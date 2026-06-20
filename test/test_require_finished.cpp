#include <gtest/gtest.h>
#include "StateMachine.h"
#include "Mocks.h"

TEST(BasicRouting, BlocksIfRequireFinishedAndStateNotFinished) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { &stateA, &stateB, new MockCondition(true), true }
        }}
    );

    stateA.finished = false;
    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateA), true);
}

TEST(BasicRouting, AllowsIfRequireFinishedAndStateIsFinished) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { &stateA, &stateB, new MockCondition(true), true }
        }}
    );

    stateA.finished = true;
    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}

TEST(BasicRouting, InterruptsIfRequireFinishedIsFalse) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { &stateA, &stateB, new MockCondition(true), false }
        }}
    );

    stateA.finished = false;
    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}

