#include <gtest/gtest.h>
#include "StateMachine.h"
#include "Mocks.h"

TEST(BasicRouting, GlobalTransitionTriggersFromAnyState) {
    MockState stateA;
    MockState stateB;
    MockState emergencyState;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { nullptr, &emergencyState, new MockCondition(true), false },
            { &emergencyState, &stateB, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&emergencyState), true);

    sm.requestTransition(&stateB);
    EXPECT_EQ(sm.isInState(&stateB), true);

    sm.onUpdate();
    EXPECT_EQ(sm.isInState(&emergencyState), true);
}

TEST(BasicRouting, GlobalTransitionOverridesLowerSpecificTransitions) {
    MockState stateA;
    MockState stateB;
    MockState emergencyState;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { nullptr, &emergencyState, new MockCondition(true), false },
            { &stateA, &stateB, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&emergencyState), true);
}

TEST(BasicRouting, GlobalTransitionIgnoredIfConditionFalse) {
    MockState stateA;
    MockState stateB;
    MockState emergencyState;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { nullptr, &emergencyState, new MockCondition(false), false },
            { &stateA, &stateB, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}

TEST(BasicRouting, GlobalTransitionRespectsRequireFinished) {
    MockState stateA;
    MockState fallbackState;

    StateMachine<State, 2> sm(
        &stateA,
        {{
            { nullptr, &fallbackState, new MockCondition(true), true }
        }}
    );

    stateA.finished = false;
    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateA), true);

    stateA.finished = true;
    sm.onUpdate();
    EXPECT_EQ(sm.isInState(&fallbackState), true);
}

TEST(BasicRouting, RejectsToAnywhereWildcard) {
    MockState stateA;
    MockState stateB;

    StateMachine<State, 2> sm(
        &stateA,
        {{
        { &stateA, nullptr, new MockCondition(true), false },
        { &stateA, &stateB, new MockCondition(true), false }
        }}
    );

    sm.onUpdate();

    EXPECT_EQ(sm.isInState(&stateB), true);
}
