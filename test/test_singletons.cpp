#include "Mocks.h"

TEST(BasicRouting, UnconditionalTransitionsAutomatically) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, Unconditional(), false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(BasicRouting, ManualOnlyIsIgnoredByUpdateLoop) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, ManualOnly(), false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateA));
}

TEST(BasicRouting, RequestTransitionSucceedsWithManualOnly) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, ManualOnly(), false }
        }}
    );

    EXPECT_TRUE(sm.requestTransition(stateB));
    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(BasicRouting, RequestTransitionFailsIfNoValidPath) {
    MockState stateA;
    MockState stateB;
    MockState stateC;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, ManualOnly(), false }
        }}
    );

    EXPECT_FALSE(sm.requestTransition(stateC));
    EXPECT_TRUE(sm.isInState(stateA));
}
