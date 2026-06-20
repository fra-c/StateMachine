#include "Mocks.h"

TEST(BasicRouting, GlobalTransitionTriggersFromAnyState) {
    MockState stateA;
    MockState stateB;
    MockState emergencyState;
    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { emergencyState, trueCond, false },
            { emergencyState, stateB, trueCond, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(emergencyState));

    sm.requestTransition(stateB);
    EXPECT_TRUE(sm.isInState(stateB));

    sm.onUpdate();
    EXPECT_TRUE(sm.isInState(emergencyState));
}

TEST(BasicRouting, GlobalTransitionOverridesLowerSpecificTransitions) {
    MockState stateA;
    MockState stateB;
    MockState emergencyState;

    MockCondition mockCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { emergencyState, mockCond, false },
            { stateA, stateB, mockCond, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(emergencyState));
}

TEST(BasicRouting, GlobalTransitionIgnoredIfConditionFalse) {
    MockState stateA;
    MockState stateB;
    MockState emergencyState;

    MockCondition trueCond(true);
    MockCondition falseCond(false);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { emergencyState, falseCond, false },
            { stateA, stateB, trueCond, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(BasicRouting, GlobalTransitionRespectsRequireFinished) {
    MockState stateA;
    MockState fallbackState;
    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { fallbackState, trueCond, true }
        }}
    );

    stateA.setFinished(false);
    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateA));

    stateA.setFinished(true);
    sm.onUpdate();
    EXPECT_TRUE(sm.isInState(fallbackState));
}

