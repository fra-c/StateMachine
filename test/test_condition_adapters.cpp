#include "Mocks.h"

class ConditionToAdapt {
public:
    bool evaluate() {
        return true;
    }
};

class MethodToAdapt {
public:
    bool check() {
        return true;
    }
};

bool functionToAdapt() {
    return true;
}

TEST(ConditionAdapter, AdapterEvaluatesTargetCondition) {
    ConditionToAdapt condition;
    ConditionAdapter<ConditionToAdapt> adapter(&condition);

    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, adapter, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(MethodAdapter, AdapterEvaluatesTargetMethod) {
    MethodToAdapt methodTarget;
    MethodAdapter<MethodToAdapt> adapter(&methodTarget, &MethodToAdapt::check);

    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, adapter, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}

TEST(FunctionAdapter, AdapterEvaluatesTargetFunction) {
    FunctionAdapter adapter(&functionToAdapt);

    MockState stateA;
    MockState stateB;

    StateMachine<State, 1> sm(
        stateA,
        {{
            { stateA, stateB, adapter, false }
        }}
    );

    sm.onUpdate();

    EXPECT_TRUE(sm.isInState(stateB));
}
