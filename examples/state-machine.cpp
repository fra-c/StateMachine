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
};

// Instantiate state machine and states
// The number <2> indicates the number of transitions, see below
StateMachine<2> stateMachine;
IdleState idleState;
ActiveState activeState;

// Declare condition functions
bool shouldActivate() {
  return digitalRead(1, HIGH);
}

bool shouldIdle() {
  return digitalRead(1, LOW);
}

void setup() {
  Serial.begin(115200);

  // Create transitions.
  // Remember to increment the number of transitions, (e.g. StateMachine<3>) when you add more transitions
  stateMachine.addTransition(&idleState, &activeState, &shouldActivate);
  stateMachine.addTransition(&activeState, &idleState, &shouldIdle);

  // Set initial state
  stateMachine.setState(&idleState);
}

void loop() {
  stateMachine.update();

  // Add delay for this example only, to prevent too many serial messages
  delay(1000);
}
