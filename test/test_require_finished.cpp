#include "Mocks.h"

TEST(BasicRouting, BlocksIfRequireFinishedAndStateNotFinished) {
    MockState stateA;
    MockState stateB;

    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { stateA, stateB, trueCond, true }
        }}
    );

    stateA.setFinished(false);
    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateA));
}

TEST(BasicRouting, AllowsIfRequireFinishedAndStateIsFinished) {
    MockState stateA;
    MockState stateB;

    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { stateA, stateB, trueCond, true }
        }}
    );

    stateA.setFinished(true);
    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(BasicRouting, InterruptsIfRequireFinishedIsFalse) {
    MockState stateA;
    MockState stateB;

    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { stateA, stateB, trueCond, false }
        }}
    );

    stateA.setFinished(false);
    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

