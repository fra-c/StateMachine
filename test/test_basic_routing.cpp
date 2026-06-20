#include "Mocks.h"

TEST(BasicRouting, TransitionsWhenConditionIsTrue) {
    MockState stateA;
    MockState stateB;

    MockCondition trueCond(true);

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, trueCond, false }
        }}
    );

    sm.onUpdate();
    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(BasicRouting, IgnoreWhenConditionIsFalse) {
    MockState stateA;
    MockState stateB;

    MockCondition falseCond(false);

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, falseCond, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateA));
}

TEST(BasicRouting, RespectsArrayPriority) {
    MockState stateA;
    MockState stateB;
    MockState stateC;

    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateA,
        {{
            { stateA, stateB, trueCond, false },
            { stateA, stateC, trueCond, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(BasicRouting, IgnoresUnmatchedFromState) {
    MockState stateA;
    MockState stateB;
    MockState stateC;

    MockCondition trueCond(true);

    StateMachine<State, 2> sm(
        stateC,
        {{
            { stateA, stateB, trueCond, false },
            { stateA, stateC, trueCond, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateC));
}
