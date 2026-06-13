#include <Arduino.h>
#include <StateMachine.h>

// A practical Condition implementing a simple timed delay
class TimeoutCondition : public Condition {
    unsigned long duration;
    unsigned long startTime;
public:
    TimeoutCondition(unsigned long ms) : duration(ms), startTime(0) {}

    void reset() { startTime = millis(); }

    bool evaluate() override {
        return (millis() - startTime) >= duration;
    }
};

// Sub-States for the driving maneuver
class AccelerateState : public State {
public:
    void onEnter() override { Serial.println("  [Maneuver] Accelerating..."); }
    void onUpdate() override { /* ramp up PWM steadily over time */ }
    void onExit() override { Serial.println("  [Maneuver] Reached top speed."); }
};

class CruiseState : public State {
public:
    void onEnter() override { Serial.println("  [Maneuver] Cruising..."); }
};

class DecelerateState : public State {
    bool stopped = false;
public:
    void onEnter() override {
        Serial.println("  [Maneuver] Decelerating...");
        stopped = false;
    }
    void onUpdate() override {
        /* decrease PWM down to 0 */
        stopped = true; // Simulating we quickly hit 0 speed
    }
    void onExit() override { Serial.println("  [Maneuver] Fully stopped."); }

    // Once speed is 0, this state reports it's finished!
    bool isFinished() override { return stopped; }
};

// Master States
class IdleState : public State {
public:
    void onEnter() override { Serial.println("Robot Idle."); }
    void onUpdate() override { /* Waiting for command */ }
};

// Condition to check if a specific state reports it is finished
class IsFinishedCondition : public Condition {
    State* state;
public:
    IsFinishedCondition(State* s) : state(s) {}
    bool evaluate() override { return state->isFinished(); }
};

// Simulated flag for triggering movement
bool userPressedGo = true;

class SignalCondition : public Condition {
public:
    bool evaluate() override {
        if (userPressedGo) {
            userPressedGo = false; // Reset flag
            return true;
        }
        return false;
    }
};


// 1. Declare State Machines
StateMachine<2> mainSM;
StateMachine<2> maneuverSM; // Holds 2 internal transitions (Accel->Cruise, Cruise->Decel)

// 2. Declare States
IdleState idleState;
AccelerateState accelState;
CruiseState cruiseState;
DecelerateState decelState;

// 3. Declare Conditions
TimeoutCondition accelTime(2000);   // Accelerate for 2 seconds
TimeoutCondition cruiseTime(3000);  // Cruise for 3 seconds
SignalCondition goSignal;
IsFinishedCondition maneuverDone(&maneuverSM); // Trigger when the entire nested SM finishes

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Hierarchical State Machine Example");

    // -------------------------------------------------------------
    // Set up the Nested Maneuver State Machine
    // -------------------------------------------------------------
    maneuverSM.addTransition(&accelState, &cruiseState, &accelTime);
    maneuverSM.addTransition(&cruiseState, &decelState, &cruiseTime);

    // We don't add a transition OUT of Decelerate inside maneuverSM.
    // Instead, Decelerate overrides `isFinished()` returning true when robot stops.
    // maneuverSM forwards `isFinished` automatically if its active state is finished.

    // -------------------------------------------------------------
    // Set up the Main Application State Machine
    // -------------------------------------------------------------
    // Idle -> maneuverSM
    mainSM.addTransition(&idleState, &maneuverSM, &goSignal);

    // maneuverSM -> Idle  (Triggered when maneuverSM completes)
    mainSM.addTransition(&maneuverSM, &idleState, &maneuverDone);

    // Initial states
    maneuverSM.setState(&accelState); // Sub-machine's default state
    mainSM.setState(&idleState);      // Main machine's default state
}

void loop() {
    // When mainSM is in 'maneuverSM', this will automatically call onUpdate() on maneuverSM,
    // which then calls onUpdate() on accelState/cruiseState/decelState.
    mainSM.onUpdate();

    // Resetting timing for our mocked example conditions
    if (mainSM.isInState(&maneuverSM)) {
        if (accelTime.evaluate() && maneuverSM.getState() == &accelState) accelTime.reset();
        if (cruiseTime.evaluate() && maneuverSM.getState() == &cruiseState) cruiseTime.reset();
    }

    delay(50);
}
