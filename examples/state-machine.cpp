#include <Arduino.h>
#include <StateMachine.h>

// Define 2 state classes, Ideally in a separate file
class IdleState : public State {
public:
    void onEnter() override {
        Serial.println("ENTER: IdleState");
    }
    void onUpdate() override {
        Serial.println("UPDATE: IdleState");
    }
    void onExit() override {
        Serial.println("EXIT: IdleState");
    }
    bool isFinished() override { return true; }
};

class ActiveState : public State {
public:
    void onEnter() override {
        Serial.println("ENTER: ActiveState");
    }
    void onUpdate() override {
        Serial.println("UPDATE: ActiveState");
    }
    void onExit() override {
        Serial.println("EXIT: ActiveState");
    }
    bool isFinished() override { return true; }
};

// Instantiate state machine and states
// The number <State, 2> indicates the state family, and the number of transitions, see below
StateMachine<State, 2> stateMachine;
IdleState idleState;
ActiveState activeState;

// Define a condition class that checks a pin state
class PinCondition : public Condition {
    int _pin;
    int _targetState;
public:
    PinCondition(int pin, int targetState) : _pin(pin), _targetState(targetState) {}

    bool evaluate() override {
        return digitalRead(_pin) == _targetState;
    }
};

// Instantiate the conditions
PinCondition shouldActivate(1, HIGH);
PinCondition shouldIdle(1, LOW);

void setup() {
  Serial.begin(115200);

  // Create transitions.
  // Remember to increment the number of transitions, (e.g. StateMachine<State, 3>) when you add more transitions
  stateMachine.addTransition(&idleState, &activeState, &shouldActivate, false);
  stateMachine.addTransition(&activeState, &idleState, &shouldIdle, false);

  // Set initial state
  stateMachine.setState(&idleState);
}

void loop() {
  stateMachine.onUpdate();

  // Add delay for this example only, to prevent too many serial messages
  delay(1000);
}
