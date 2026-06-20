#include "Mocks.h"

TEST(BasicRouting, UnconditionalTransitionsAutomatically) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
            { &stateA, &stateB, Unconditional(), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}

TEST(BasicRouting, ManualOnlyIsIgnoredByUpdateLoop) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
            { &stateA, &stateB, ManualOnly(), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateA), true);
}

TEST(BasicRouting, RequestTransitionSucceedsWithManualOnly) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
            { &stateA, &stateB, ManualOnly(), false }
        }}
    );

    EXPECT_EQ(sm.requestTransition(&stateB), true);
    EXPECT_EQ(sm.isInState(&stateB), true);
}

TEST(BasicRouting, RequestTransitionFailsIfNoValidPath) {
    MockState stateA;
    MockState stateB;
    MockState stateC;

    StateMachine<State, 1> sm(
        &stateA,
        {{
            { &stateA, &stateB, ManualOnly(), false }
        }}
    );

    EXPECT_EQ(sm.requestTransition(&stateC), false);
    EXPECT_EQ(sm.isInState(&stateA), true);
}
