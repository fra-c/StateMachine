#include "Mocks.h"

TEST(StateMachineTest, MachineNotFinishedIfChildNotFinished) {
    // 1. Set up the active state and explicitly mark it as NOT finished
    MockState childStateA;
    MockState childStateB;
    childStateA.setFinished(false);

    // 2. Set up the state machine.
    StateMachine<State, 1> childSM(
        childStateA,
        {{
            { childStateA, childStateB, ManualOnly(), false }
        }}
    );

    // 3. The assertion: Because the active state (childStateA) is not finished,
    // the machine itself must report that it is not finished.
    EXPECT_FALSE(childSM.isFinished());
}

TEST(StateMachineTest, MachineNotFinishedIfUnconditionalPathExists) {
    // 1. Setup the active state and explicitly mark it as finished.
    // Its physical work is done.
    MockState childStateA;
    childStateA.setFinished(true);

    // 2. Setup the destination state.
    MockState childStateB;

    // 3. Setup the machine.
    // We give it an Unconditional path out of State A. Because requireFinished
    // is true, this path is only valid when State A is finished (which it is).
    StateMachine<State, 1> childSM(
        childStateA,
        {{
            { childStateA, childStateB, Unconditional(), true }
        }}
    );

    // 4. The assertion: Even though the active child state is finished,
    // the machine itself must report FALSE because it sees a valid,
    // automatic path out of the current state that it intends to take.
    EXPECT_FALSE(childSM.isFinished());
}

TEST(StateMachineTest, MachineNotFinishedIfConditionIsTrue) {
    // 1. Setup the active state and explicitly mark its physical work as done
    MockState childStateA;
    childStateA.setFinished(true);

    // 2. Setup the destination state
    MockState childStateB;

    // 3. Setup a condition that evaluates to true
    MockCondition condTrue(true);

    // 4. Setup the machine.
    // We give it a conditional path out of State A. Because requireFinished
    // is true, this path is only evaluated when State A is finished (which it is).
    StateMachine<State, 1> childSM(
        childStateA,
        {{
            { childStateA, childStateB, condTrue, true }
        }}
    );

    // 5. The assertion: Even though the active child state is finished,
    // the machine itself must report FALSE because it has an internal
    // conditional route that is completely valid and ready to be taken.
    EXPECT_FALSE(childSM.isFinished());
}

// --- NEW & CORRECTED TESTS BELOW ---

TEST(StateMachineTest, MachineWaitsForManualCommands) {
    // 1. Setup the active state and explicitly mark its physical work as done
    MockState childStateA;
    childStateA.setFinished(true);

    // 2. Setup the destination state
    MockState childStateB;

    // 3. Setup the machine with a ManualOnly path out.
    StateMachine<State, 1> childSM(
        childStateA,
        {{
            // This path is entirely valid, it just awaits a manual requestTransition() call.
            { childStateA, childStateB, ManualOnly(), false }
        }}
    );

    // 4. The assertion: The active state is finished, but the machine sees a valid
    // route forward to State B. Because it is actively waiting for a command,
    // the machine itself is NOT finished.
    EXPECT_FALSE(childSM.isFinished());
}

TEST(StateMachineTest, MachineWaitsForConditionToBecomeTrue) {
    // 1. Setup the active state and mark its physical work as done.
    MockState childStateA;
    childStateA.setFinished(true);

    // 2. Setup the destination state.
    MockState childStateB;

    // 3. Setup a condition that is currently FALSE (e.g., waiting for a heater to reach 50C).
    MockCondition sensorCondition(false);

    // 4. Setup the machine.
    StateMachine<State, 1> childSM(
        childStateA,
        {{
            // A valid path forward exists, but its condition is currently unfulfilled.
            { childStateA, childStateB, sensorCondition, false }
        }}
    );

    // 5. The assertion: The machine is sitting in State A waiting for the condition
    // to become true. Because it is patiently waiting to transition forward,
    // it must NOT declare itself finished.
    EXPECT_FALSE(childSM.isFinished());
}

TEST(StateMachineTest, MachineIsFinishedWhenTrulyDeadEnded) {
    // 1. Setup the active state and mark its physical work as done.
    MockState childStateA;
    childStateA.setFinished(true);

    MockState childStateB;
    MockState childStateC;

    // 2. Setup the machine with ZERO outgoing paths from State A.
    // We define a transition table where paths only originate from State B.
    StateMachine<State, 1> childSM(
        childStateA,
        {{
            { childStateB, childStateC, Unconditional(), false }
        }}
    );

    // 3. The assertion: State A is finished, and the machine scans its transition table
    // finding absolutely no routes originating from State A. It is truly at a dead end.
    EXPECT_TRUE(childSM.isFinished());
}
