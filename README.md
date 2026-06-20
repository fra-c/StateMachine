# Embedded C++ Hierarchical State Machine

A lightweight, header-only, zero-allocation state machine library designed specifically for microcontrollers and embedded systems.

This library provides a mathematically sound routing engine that supports event-driven transitions, automatic polling sequences, deep hierarchical nesting (state machines within state machines), and hardware interrupt routing—all without ever touching the heap.

## Features

* **Zero Dynamic Allocation:** Built entirely on templates and `std::array`. Perfect for strict embedded environments.
* **Hierarchical (Nested) Machines:** A State Machine inherits from `State`, meaning a parent machine can seamlessly orchestrate child machines exactly like standard states.
* **Deterministic Routing:** Array-order evaluation guarantees predictable priority for emergency interrupts over normal flow.
* **Built-in Routing Singletons:** Includes `sm::Unconditional()` and `sm::ManualOnly()` for memory-free sequential and event-driven routing.

---

## The Transition Table

The core of the engine is the Transition array. Every route in your system is defined by four rules:

| `from` | `to` | `condition` | `requireFinished` |
| --- | --- | --- | --- |
| The state you are leaving. | The destination state. | The rule to evaluate. | `true`: Wait for `from` to finish.<br>

<br>`false`: Interrupt immediately. |

> **Note:** Setting `from` to `nullptr` creates a **Global Transition** (can trigger from *any* state).

---

## 1. Quick Start: The Bare Minimum

Here is the simplest use case: A basic Heater that turns on when a button is pressed, and turns off automatically when a target temperature is reached.

```cpp
#include "StateMachine.h"

// 1. Define your base state family
class HeaterState : public State { /* ... */ };

// 2. Create your states and conditions
class IdleState : public HeaterState { /* ... */ } idle;
class HeatingState : public HeaterState { /* ... */ } heating;

class ButtonPressedCond : public Condition {
    bool evaluate() override { return hardware_read_button(); }
} buttonPressed;

// 3. Define the State Machine
StateMachine<HeaterState, 2> heaterSM(
    &idle, // Initial state
    {{
        // If button pressed, go from Idle -> Heating immediately
        { &idle, &heating, &buttonPressed, false },

        // Go from Heating -> Idle unconditionally, BUT only when Heating is finished
        { &heating, &idle, sm::Unconditional(), true }
    }}
);

// 4. Run it in your main loop
void loop() {
    heaterSM.onUpdate();
}

```

---

## 2. Intermediate: Interrupts & Priority Routing

Transition order dictates priority. Place emergency interrupts at the top of your array, and normal operational flow at the bottom.

In this Rover Drive example, an obstacle will instantly interrupt the driving sequence, no matter what state the rover is currently in.

```cpp
StateMachine<DriveState, 4> driveSM(
    &stationary,
    {{
        // --- PRIORITY 1: EMERGENCIES ---
        // Global Transition (nullptr): Jump to EmergencyStop from ANY state instantly if an obstacle is seen.
        { nullptr, &emergencyStop, &obstacleDetected, false },

        // --- PRIORITY 2: NORMAL FLOW ---
        // 1. Start moving manually via an external request (e.g., RPC/UI)
        { &stationary, &rampingUp, sm::ManualOnly(), false },

        // 2. Automatically go to Cruise ONLY when Ramping Up is physically finished
        { &rampingUp, &cruise, sm::Unconditional(), true },

        // 3. Manually trigger braking via an external request
        { &cruise, &slowingDown, sm::ManualOnly(), false }
    }}
);

// Example of an external event triggering a manual transition
void onGoCommandReceived() {
    driveSM.requestTransition(&rampingUp);
}

```

---

## 3. Advanced: Hierarchical Parenting

Because `StateMachine` inherits from your base state, you can plug entire state machines into other state machines.

The parent acts as the orchestrator. It handles the high-level logic and blindly delegates the micro-level updates to the active child machine. When a child machine reaches a dead-end, it automatically reports as "Finished" so the parent can route it elsewhere.

```cpp
// --- THE CHILD MACHINE (Micro-level maneuver) ---
StateMachine<RobotState, 2> parallelParkSM(
    &aligning,
    {{
        // Internal child sequence
        { &aligning, &reversing, sm::Unconditional(), true },
        { &reversing, &straightening, sm::Unconditional(), true }
        // When 'straightening' finishes, this child machine has nowhere else to go.
        // It will automatically report isFinished() == true to the parent.
    }}
);

// --- THE PARENT MACHINE (Macro-level orchestrator) ---
StateMachine<RobotState, 3> roverOrchestratorSM(
    &navigating,
    {{
        // 1. Enter the parking sequence when a spot is found
        { &navigating, &parallelParkSM, &spotFoundCond, false },

        // 2. When the parking machine is completely finished, shut down the rover
        { &parallelParkSM, &shutDown, sm::Unconditional(), true },

        // 3. Global Safety: Abort everything and wait for human help if battery dies
        { nullptr, &idleAwaitingHelp, &batteryDeadCond, false }
    }}
);

// In your hardware timer loop, you only ever tick the top-level parent!
void hardware_timer_tick() {
    roverOrchestratorSM.onUpdate();
}

```
