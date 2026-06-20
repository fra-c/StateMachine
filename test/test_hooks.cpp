#include "Mocks.h"

TEST(BasicRouting, UpdateLoopCallsHooksProperly) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
            { &stateA, &stateB, Unconditional(), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(stateA.enterCount, 1);
    EXPECT_EQ(stateA.updateCount, 0);
    EXPECT_EQ(stateA.exitCount, 1);
    EXPECT_EQ(stateB.enterCount, 1);
    EXPECT_EQ(stateB.updateCount, 1);
    EXPECT_EQ(stateB.exitCount, 0);
}

TEST(BasicRouting, RequestTransitionDoesNotTick) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
            { &stateA, &stateB, Unconditional(), false }
        }}
    );

    sm.requestTransition(&stateB);

    EXPECT_EQ(stateA.exitCount, 1);
    EXPECT_EQ(stateB.enterCount, 1);
    EXPECT_EQ(stateB.updateCount, 0);
}

TEST(BasicRouting, NoUpdateIfNoTransition) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {}
    );

    sm.onUpdate();

    EXPECT_EQ(stateA.updateCount, 1);
}
