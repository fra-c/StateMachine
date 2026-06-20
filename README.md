# MSM: Minimalist State Machine for Embedded C++

**MSM** is a high-performance, strictly zero-allocation Hierarchical State Machine (HSM) library designed specifically for bare-metal embedded systems (ESP32, ARM Cortex-M, Arduino, etc.).

It maps complex logical states and concurrent hardware peripherals directly to static memory, guaranteeing `O(1)` execution speed, compile-time safety, and zero heap fragmentation.

## ✨ Core Features

* **Strict Zero-Allocation:** Uses `std::array`, templates, and C++ references. No `new`, no `malloc`, no `std::vector`. Memory is reserved entirely at compile-time on the stack or BSS segment.


* **Compile-Time Safety:** The API utilizes C++ references at the boundary to make `nullptr` transition targets mathematically impossible, preventing the most common embedded state machine crashes.


* **Hierarchical (HSM):** Natively implements the Composite Pattern. A `StateMachine` *is* a `State`, allowing infinite nesting of sub-modes.


* **Orthogonal Regions (Parallel States):** Supports running multiple independent state machines concurrently under a single master state.


* **High Performance:** `O(1)` transition routing and zero runtime branching overhead for array padding.



---

## 🏗️ Architecture Glossary

* **`State`**: The abstract base class representing a system mode. Users implement `onEnter()`, `onUpdate()`, `onExit()`, and `isFinished()`.


* **`Condition`**: Evaluates to `true` or `false` to trigger transitions. You can use hardware sensors or adapt existing functions using `FunctionAdapter` or `MethodAdapter`.


* **`Transition`**: A routing rule defining `[From State] -> [To State]` when `[Condition]` is met.


* **`StateMachine`**: The engine evaluating transitions and executing states. It inherits from `State`, meaning it can be nested.


* **`ParallelState`**: A wrapper that ticks multiple state machines concurrently, acting as a synchronization barrier that only finishes when *all* internal machines are finished.



---

## 📖 Usage Examples

### 1. Basic Usage & Wildcard Transitions

Here is a simple machine that toggles an LED based on a hardware button, and includes a global wildcard transition to handle hardware faults.

```cpp
#include "StateMachine.h"

// 1. Define your states
class LedOffState : public msm::State {
    void onEnter() override { digitalWrite(LED_PIN, LOW); }
};

class LedOnState : public msm::State {
    void onEnter() override { digitalWrite(LED_PIN, HIGH); }
};

class ErrorState : public msm::State {
    void onEnter() override { /* Sound alarm */ }
};

// 2. Define conditions (e.g., reading a GPIO pin)
class ButtonPressedCondition : public msm::Condition {
    bool evaluate() override { return digitalRead(BTN_PIN) == LOW; }
};
MockCondition faultDetected(false); // Example fault condition

// 3. Instantiate objects on the stack or globally (Zero Allocation!)
LedOffState ledOff;
LedOnState ledOn;
ErrorState errorState;
ButtonPressedCondition btnPressed;

// 4. Create the State Machine
msm::StateMachine<msm::State, 3> systemSM(
    ledOff, // Initial state
    {{
        // STANDARD TRANSITIONS: { From, To, Condition, requireFinished }
        // Note: All arguments are passed as clean C++ references!
        { ledOff, ledOn, btnPressed, false },
        { ledOn, ledOff, btnPressed, false },

        // WILDCARD TRANSITION: { To, Condition, requireFinished }
        // Omitting the 'From' state means this triggers from ANY active state.
        { errorState, faultDetected, false }
    }}
);

void loop() {
    systemSM.onUpdate(); // Evaluates conditions and ticks the active state
}

```

### 2. Hierarchical State Machines (Parenting)

Because `StateMachine` inherits from `State`, you can pass an entire state machine into another state machine as a child. When the parent enters the child machine, the child resets to its initial state.

```cpp
// ... assume idleState, brewingState, autoMode, manualMode are defined ...

// 1. The Child Machine (Handles a specific sequence)
msm::StateMachine<msm::State, 2> brewingSM(
    idleState,
    {{
        { idleState, brewingState, startButton, false },
        { brewingState, idleState, brewComplete, false }
    }}
);

// 2. The Parent Machine (Handles global operating modes)
msm::StateMachine<msm::State, 2> masterSM(
    manualMode,
    {{
        // Transitioning into the brewingSM automatically calls brewingSM.onEnter()
        { manualMode, brewingSM, startAutoCondition, false },

        // Exiting brewingSM automatically calls onExit() on whichever child state is active!
        { brewingSM, manualMode, stopAutoCondition, false }
    }}
);

```

### 3. Orthogonal Regions (Parallel States)

Use `ParallelState` when you have multiple independent hardware peripherals that must operate simultaneously. A great example is a **Smartwatch** running various background tasks simultaneously.

```cpp
// 1. Create independent peripheral state machines
msm::StateMachine<msm::State, 2> displaySM(screenOff, /* screen transitions */);
msm::StateMachine<msm::State, 2> heartRateSM(sensorIdle, /* measuring transitions */);
msm::StateMachine<msm::State, 2> bluetoothSM(bleDisconnected, /* sync transitions */);

// 2. Bundle them into a ParallelState
// This wrapper ensures all 3 machines are ticked concurrently during onUpdate()
msm::ParallelState<msm::State, 3> activeWatchMode({
    displaySM,
    heartRateSM,
    bluetoothSM
});

```

### 4. Putting It All Together (Parented Parallel States & Graceful Shutdown)

This demonstrates a master orchestrator governing concurrent systems on our Smartwatch. It uses `requireFinished = true` to ensure the watch does not instantly power off when the battery dies. Instead, it waits for the `ParallelState` to report that the display, heart-rate sensor, and Bluetooth radio have all safely saved their data and gracefully shut down.

```cpp
// ... assume displaySM, heartRateSM, and bluetoothSM are defined
// and have their own internal "Saving Data/Shutting Down" logic ...

// 1. The Concurrent Operating Mode
msm::ParallelState<msm::State, 3> activeWatchMode({ displaySM, heartRateSM, bluetoothSM });

MockState powerOffState;
MockState gracefulShutdownWait;

// 2. The Master Orchestrator
msm::StateMachine<msm::State, 3> masterSM(
    powerOffState,
    {{
        // Boot up the watch -> Starts all subsystems concurrently
        { powerOffState, activeWatchMode, powerButton, false },

        // Low Battery triggers a move to the waiting state
        { activeWatchMode, gracefulShutdownWait, lowBatteryEvent, false },

        // WAIT BARRIER: Only transition to PowerOff when activeWatchMode is FINISHED.
        // ParallelState::isFinished() only returns true when ALL internal machines are finished.
        { gracefulShutdownWait, powerOffState, msm::Unconditional(), true }
    }}
);

```

## 📡 Handling External Commands (WebSockets/Serial)

Unlike physical sensors, network commands are ephemeral events. Do not inject strings or JSON directly into the State Machine. Instead, use a zero-allocation `CommandContext` bridged via a `Condition`.

```cpp
// 1. A static context to hold the latest network event
struct CommandContext {
    int currentCommandId = 0;
    bool hasNewCommand = false;
    void clear() { hasNewCommand = false; currentCommandId = 0; }
};

CommandContext globalCmd;

// 2. A Condition that reads the context
class CommandCondition : public msm::Condition {
    CommandContext& ctx;
    int targetCmd;
public:
    CommandCondition(CommandContext& c, int target) : ctx(c), targetCmd(target) {}
    bool evaluate() override { return ctx.hasNewCommand && ctx.currentCommandId == targetCmd; }
};

// 3. Instantiate the conditions
CommandCondition cmdSyncNow(globalCmd, 12); // Assume ID 12 = Force Sync

// 4. Feed the context in your WebSocket callback
void onWebSocketEvent(int incomingCommandId) {
    globalCmd.currentCommandId = incomingCommandId;
    globalCmd.hasNewCommand = true;
}

void loop() {
    masterSM.onUpdate(); // The state machine naturally consumes the event if it's a valid transition
    globalCmd.clear();   // Clear the context at the end of the loop
}

```
