#include <gtest/gtest.h>
#include "StateMachine.h"

using namespace msm;

class DeltaTimeState : public State {
public:
    float lastDeltaTime = -1.0f;
    void onUpdate(float delta_time = 0.0f) override {
        lastDeltaTime = delta_time;
    }
};

TEST(DeltaTimeTest, PassesDeltaTimeDown) {
    DeltaTimeState stateA;
    std::array<Transition<State>, 1> transitions = {
        Transition<State>()
    };
    StateMachine<State, 1> sm(stateA, transitions);

    sm.onUpdate(1.5f);
    EXPECT_FLOAT_EQ(stateA.lastDeltaTime, 1.5f);

    sm.onUpdate(0.2f);
    EXPECT_FLOAT_EQ(stateA.lastDeltaTime, 0.2f);

    sm.onUpdate();
    EXPECT_FLOAT_EQ(stateA.lastDeltaTime, 0.0f);
}
