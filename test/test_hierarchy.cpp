#include <gtest/gtest.h>
#include "StateMachine.h"
#include "Mocks.h"

TEST(BasicRouting, ParentDelegatesUpdateToChild) {
    MockState parentState;
    MockState childState;

    StateMachine<State, 1> childSM(
        &childState,
        {}
    );

    StateMachine<State, 1> parent(
        &parentState,
        {{
            { &parentState, &childSM, Unconditional(), false }
        }}
    );



    parent.onUpdate();

    EXPECT_EQ(parent.isInState(&childSM), true);
    EXPECT_EQ(childSM.isInState(&childState), true);
    EXPECT_EQ(childState.updateCount, 1);
}

TEST(StateMachineTest, ChildResetsOnReentry) {
    MockState parentState;
    MockState childStateA;
    MockState childStateB;

    // Child can manually move from A -> B
    StateMachine<State, 1> childSM(
        &childStateA,
        {{ { &childStateA, &childStateB, ManualOnly(), false } }}
    );

    // Parent can manually swap back and forth between parentState and childSM
    StateMachine<State, 2> parentSM(
        &parentState,
        {{
            { &parentState, &childSM, ManualOnly(), false },
            { &childSM, &parentState, ManualOnly(), false }
        }}
    );

    // STEP 1: Enter the child machine
    parentSM.requestTransition(&childSM);
    parentSM.onUpdate();
    EXPECT_EQ(childSM.getState(), &childStateA);

    // STEP 2: Move the child machine internally to State B
    childSM.requestTransition(&childStateB);
    parentSM.onUpdate(); // Ticking the parent ticks the child!
    EXPECT_EQ(childSM.getState(), &childStateB);

    // STEP 3: Parent exits the child machine
    parentSM.requestTransition(&parentState);
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &parentState);

    // STEP 4: Parent re-enters the child machine
    parentSM.requestTransition(&childSM);
    parentSM.onUpdate();

    // FINAL ASSERTION: The child machine MUST be back in State A!
    // If this fails and returns childStateB, the reset bug has returned.
    EXPECT_EQ(childSM.getState(), &childStateA);
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
