# State Machine

A lightweight, zero-allocation, and fully hierarchical state machine library designed for embedded systems (Arduino, ESP32, etc.).

## Features
- **Zero Dynamic Allocation:** Predictable memory footprint. Transitions are pre-allocated via C++ templates.
- **Class-Based Conditions:** Conditions are extensible classes, allowing you to easily store state (like pin numbers or timers).
- **Hierarchical (Nested) State Machines:** A `StateMachine` inherits from `State`, meaning you can nest sub-state machines inside others to cleanly organize complex behavior.
- **Global/Any-State Transitions:** Easily handle emergency stops or system-wide events without duplicating transitions for every state.

## Basic Usage

See the full code in [examples/state-machine.cpp](examples/state-machine.cpp).

```cpp
#include <StateMachine.h>

// 1. Define States
class IdleState : public State {
    void onEnter() override { Serial.println("Idling..."); }
};
class ActiveState : public State {
    void onEnter() override { Serial.println("Active!"); }
};

// 2. Define Conditions
class PinCondition : public Condition {
    int pin, targetState;
public:
    PinCondition(int p, int t) : pin(p), targetState(t) {}
    bool evaluate() override { return digitalRead(pin) == targetState; }
};

// 3. Setup State Machine
StateMachine<2> sm; // Pre-allocates space for exactly 2 transitions
IdleState idle;
ActiveState active;
PinCondition buttonPressed(1, HIGH);
PinCondition buttonReleased(1, LOW);

void setup() {
    sm.addTransition(&idle, &active, &buttonPressed);
    sm.addTransition(&active, &idle, &buttonReleased);
    sm.setState(&idle);
}

void loop() {
    sm.onUpdate();
}
```

## Advanced Features

### Global (Any-State) Transitions
Pass `nullptr` as the `from` state to create a transition that can trigger from *any* state. Perfect for error handling or hardware interrupts.

```cpp
// Jumps to 'errorState' from anywhere if 'wifiLostCondition' evaluates to true
sm.addTransition(nullptr, &errorState, &wifiLostCondition);
```

### Hierarchical (Nested) State Machines
Because the framework treats a `StateMachine` exactly like a `State`, you can nest them directly. The `isFinished()` capability automatically bubbles up out of child machines when a sub-sequence is complete!

See the complete complex robotics maneuver example in [examples/nested-state-machine.cpp](examples/nested-state-machine.cpp).

```cpp
StateMachine<2> mainSM;
StateMachine<2> maneuverSM; // Sub-machine

IsFinishedCondition maneuverDone(&maneuverSM);

void setup() {
    // ... setup maneuverSM states and transitions ...

    // Jump from Idle to Maneuver sub-machine
    mainSM.addTransition(&idleState, &maneuverSM, &goSignal);
    
    // Return to Idle only when the maneuverSM signals it is finished
    mainSM.addTransition(&maneuverSM, &idleState, &maneuverDone);
    
    mainSM.setState(&idleState);
}

void loop() {
    // Automatically ticks down into maneuverSM if active
    mainSM.onUpdate();
}
```
