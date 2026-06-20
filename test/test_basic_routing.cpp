#include <gtest/gtest.h>
#include "StateMachine.h"
#include "Mocks.h"

TEST(BasicRouting, TransitionsWhenConditionIsTrue) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
        { &stateA, &stateB, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}

TEST(BasicRouting, IgnoreWhenConditionIsFalse) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        &stateA,
        {{
        { &stateA, &stateB, new MockCondition(false), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateA), true);
}

TEST(BasicRouting, RespectsArrayPriority) {
    MockState stateA;
    MockState stateB;
    MockState stateC;

    StateMachine<State, 2> sm(
        &stateA,
        {{
        { &stateA, &stateB, new MockCondition(true), false },
        { &stateA, &stateC, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}

TEST(BasicRouting, IgnoresUnmatchedFromState) {
    MockState stateA;
    MockState stateB;
    MockState stateC;

    StateMachine<State, 2> sm(
        &stateC,
        {{
        { &stateA, &stateB, new MockCondition(true), false },
        { &stateA, &stateC, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateC), true);
}

