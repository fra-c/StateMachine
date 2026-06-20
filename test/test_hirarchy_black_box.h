#include "Mocks.h"

TEST(StateMachineTest, MachineNotFinishedIfChildNotFinished) {
    // 1. Set up the active state and explicitly mark it as NOT finished
    MockState childStateA;
    childStateA.finished = false;

    // 2. Set up the state machine.
    // We add a dummy ManualOnly transition so the compiler satisfies MAX_TRANSITIONS > 0
    StateMachine<State, 1> childSM(
        &childStateA,
        {{
            { &childStateA, nullptr, ManualOnly(), false }
        }}
    );

    // 3. The assertion: Because the active state (childStateA) is not finished,
    // the machine itself must report that it is not finished.
    EXPECT_EQ(childSM.isFinished(), false);

    // --- Bonus Validation (Optional but highly recommended) ---
    // If we suddenly mark the child state as finished, the machine should
    // now report as finished because it has no automatic paths out of State A.
    childStateA.finished = true;
    EXPECT_EQ(childSM.isFinished(), true);
}

TEST(StateMachineTest, MachineNotFinishedIfUnconditionalPathExists) {
    // 1. Setup the active state and explicitly mark it as finished.
    // Its physical work is done.
    MockState childStateA;
    childStateA.finished = true;

    // 2. Setup the destination state.
    MockState childStateB;

    // 3. Setup the machine.
    // We give it an Unconditional path out of State A. Because requireFinished
    // is true, this path is only valid when State A is finished (which it is).
    StateMachine<State, 1> childSM(
        &childStateA,
        {{
            { &childStateA, &childStateB, Unconditional(), true }
        }}
    );

    // 4. The assertion: Even though the active child state is finished,
    // the machine itself must report FALSE because it sees a valid,
    // automatic path out of the current state that it intends to take.
    EXPECT_EQ(childSM.isFinished(), false);
}

TEST(StateMachineTest, MachineNotFinishedIfConditionIsTrue) {
    // 1. Setup the active state and explicitly mark its physical work as done
    MockState childStateA;
    childStateA.finished = true;

    // 2. Setup the destination state
    MockState childStateB;

    // 3. Setup a condition that evaluates to true (from your original test suite)
    MockCondition condTrue(true);

    // 4. Setup the machine.
    // We give it a conditional path out of State A. Because requireFinished
    // is true, this path is only evaluated when State A is finished (which it is).
    // The condition also evaluates to true.
    StateMachine<State, 1> childSM(
        &childStateA,
        {{
            { &childStateA, &childStateB, &condTrue, true }
        }}
    );

    // 5. The assertion: Even though the active child state is finished,
    // the machine itself must report FALSE because it has an internal
    // conditional route that is completely valid and ready to be taken.
    EXPECT_EQ(childSM.isFinished(), false);
}

TEST(StateMachineTest, MachineIsFinishedWhenDeadEnded) {
    // 1. Setup the active state and explicitly mark its physical work as done
    MockState childStateA;
    childStateA.finished = true;

    // 2. Setup a dummy destination state
    MockState childStateB;

    // 3. Setup the machine.
    // We give it a path out, BUT we use ManualOnly(). Because ManualOnly
    // evaluates to false during the polling loop, there are NO automatic
    // transitions available. This state is a complete dead end.
    StateMachine<State, 1> childSM(
        &childStateA,
        {{
            { &childStateA, &childStateB, ManualOnly(), false }
        }}
    );

    // 4. The assertion: The active child state is finished, and the machine
    // scans its internal array and realizes it has absolutely nowhere else
    // to go automatically. Therefore, the machine itself is finished.
    EXPECT_EQ(childSM.isFinished(), true);
}

TEST(StateMachineTest, ParentTransitionsOutWhenChildFinished) {
    MockState parentStateA;
    MockState parentStateB;

    MockState childStateA;
    MockState childStateB;

    StateMachine<State, 1> childSM(
        &childStateA,
        {{ { &childStateA, &childStateB, ManualOnly(), false } }}
    );

    StateMachine<State, 2> parentSM(
        &parentStateA,
        {{
            { &parentStateA, &childSM, ManualOnly(), false },
            { &childSM, &parentStateB, Unconditional(), true }
        }}
    );

    // STEP 1: Move parent into the child machine
    parentSM.requestTransition(&childSM);
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &childSM);
    EXPECT_EQ(childSM.getState(), &childStateA);

    // STEP 2: Tick parent. It should NOT transition to parentStateB yet.
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &childSM); // Still inside child

    // STEP 3: Move child internally to State B
    childSM.requestTransition(&childStateB);
    parentSM.onUpdate();
    EXPECT_EQ(childSM.getState(), &childStateB);

    // STEP 4: Tick parent again. childStateB isn't finished, so it holds.
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &childSM);

    // STEP 5: The Magic Moment - Mark the child state as finished.
    // Because childStateB has nowhere else to go (dead-end),
    // childSM will now report itself as finished too!
    childStateB.finished = true;

    // STEP 6: Tick the parent. It should see childSM is finished,
    // evaluate the Unconditional transition, and route to parentStateB.
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &parentStateB);
}
