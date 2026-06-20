#include "Mocks.h"

TEST(ParallelStateTest, TickingStateMachineTicksParallelStateRegions) {
    // 1. Setup states for two independent peripherals (e.g., Wheels and Blade)
    MockState wheelsIdling, wheelsMoving;
    MockState bladeOff, bladeSpinning;

    // Wheels have a true condition, Blade has a false condition
    MockCondition turnWheelsOn(true);
    MockCondition turnBladeOn(false);

    // 2. Setup the internal state machines (Orthogonal Regions)
    StateMachine<State, 1> wheelsSM(
        wheelsIdling,
        {{ { wheelsIdling, wheelsMoving, turnWheelsOn, false } }}
    );

    StateMachine<State, 1> bladeSM(
        bladeOff,
        {{ { bladeOff, bladeSpinning, turnBladeOn, false } }}
    );

    // 3. Bundle them into the ParallelState
    ParallelState<State, 2> normalOperatingMode({ wheelsSM, bladeSM });

    StateMachine<State, 1> systemSM(
        normalOperatingMode,
        {}
    );

    systemSM.onUpdate();
    // 4. THE ASSERTION:
    // WheelsSM should have transitioned because its condition was true.
    // BladeSM should NOT have transitioned because its condition was false.
    EXPECT_TRUE(wheelsSM.isInState(wheelsMoving));
    EXPECT_TRUE(bladeSM.isInState(bladeOff));
}

TEST(ParallelStateTest, IsFinishedRequiresAllRegionsToBeFinished) {
    // 1. Setup two mock states representing two internal state machines
    MockState wheelsSM;
    MockState bladeSM;

    // Both are currently busy running their graceful shutdown maneuvers
    wheelsSM.setFinished(false);
    bladeSM.setFinished(false);

    // 2. Bundle them
    ParallelState<State, 2> shutdownMode({ wheelsSM, bladeSM });

    // 3. THE ASSERTIONS:

    // A. Neither are finished. Parallel state must be false.
    EXPECT_FALSE(shutdownMode.isFinished());

    // B. The wheels finish slowing down, but the blade is still spinning.
    // The parallel state MUST still return false.
    wheelsSM.setFinished(true);
    EXPECT_FALSE(shutdownMode.isFinished());

    // C. The blade finally stops. Now both are finished.
    // The parallel state should now return true, signaling the Master SM to proceed.
    bladeSM.setFinished(true);
    EXPECT_TRUE(shutdownMode.isFinished());
}

TEST(ParallelStateTest, TransitionsBroadcastEnterAndExitToAllRegions) {
    // 1. Setup states
    MockState wheelsIdling, bladeOff;

    // 2. Setup internal state machines
    StateMachine<State, 1> wheelsSM(wheelsIdling, {{}});
    StateMachine<State, 1> bladeSM(bladeOff, {{}});

    // 3. Bundle into ParallelState
    ParallelState<State, 2> normalOperatingMode({ wheelsSM, bladeSM });

    // 4. Setup Master states
    MockState systemBooting;
    MockState systemEmergencyStop;

    MockCondition systemReady(true);
    MockCondition emergencyTriggered(true);

    // 5. Setup Master SM
    StateMachine<State, 2> masterSM(
        systemBooting,
        {{
            // Transition INTO the parallel state
            { systemBooting, normalOperatingMode, systemReady, false },
            // Transition OUT OF the parallel state
            { normalOperatingMode, systemEmergencyStop, emergencyTriggered, false }
        }}
    );

    // STEP 1: Booting -> Normal Operation
    // This must trigger onEnter() on both wheelsSM and bladeSM.
    masterSM.onUpdate();
    EXPECT_TRUE(masterSM.isInState(normalOperatingMode));

    // Prove the children were started (isInState checks if they are active)
    EXPECT_TRUE(wheelsSM.isInState(wheelsIdling));
    EXPECT_TRUE(bladeSM.isInState(bladeOff));

    // STEP 2: Normal Operation -> Emergency Stop
    // This must trigger onExit() on normalOperatingMode, which cascades down.
    masterSM.onUpdate();
    EXPECT_TRUE(masterSM.isInState(systemEmergencyStop));
}

TEST(ParallelStateTest, ParentMachineWaitsForParallelStateToFinish) {
    // 1. Setup peripheral states
    MockState wheelsSM;
    MockState bladeSM;

    // Start them as NOT finished (running)
    wheelsSM.setFinished(false);
    bladeSM.setFinished(false);

    // 2. Bundle into ParallelState
    ParallelState<State, 2> shutdownMode({ wheelsSM, bladeSM });

    // 3. Setup a final idle state
    MockState systemIdle;

    // 4. Setup Master SM
    // We start in shutdownMode and try to unconditionally transition to systemIdle.
    // Crucially, requireFinished = TRUE.
    StateMachine<State, 1> masterSM(
        shutdownMode,
        {{
            { shutdownMode, systemIdle, Unconditional(), true }
        }}
    );

    // STEP 1: Tick the master.
    // Because the regions aren't finished, the master MUST stay in shutdownMode.
    masterSM.onUpdate();
    EXPECT_TRUE(masterSM.isInState(shutdownMode));

    // STEP 2: One region finishes (Wheels stop).
    // The master MUST still wait because the blade is spinning.
    wheelsSM.setFinished(true);
    masterSM.onUpdate();
    EXPECT_TRUE(masterSM.isInState(shutdownMode));

    // STEP 3: The final region finishes (Blade stops).
    // Now the ParallelState barrier drops. The transition should finally execute!
    bladeSM.setFinished(true);
    masterSM.onUpdate();

    EXPECT_TRUE(masterSM.isInState(systemIdle));
}
