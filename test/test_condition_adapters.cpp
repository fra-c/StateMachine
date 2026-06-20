#include "Mocks.h"

class ClassToAdapt {
public:
    bool check() const {
        return true;
    }
};

bool functionToAdapt() {
    return true;
}

TEST(ConditionMethodAdapter, AdapterEvaluatesTargetMethod) {
    ClassToAdapt methodTarget;
    ConditionMethodAdapter<ClassToAdapt, &ClassToAdapt::check> adapter(&methodTarget);

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

TEST(ConditionFunctionAdapter, AdapterEvaluatesTargetFunction) {
    ConditionFunctionAdapter<&functionToAdapt> adapter;

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
