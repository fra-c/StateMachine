#include <gtest/gtest.h>
#include "StateMachine.h"

class MockState : public State {
public:
    int enterCount = 0;
    int updateCount = 0;
    int exitCount = 0;

    void onEnter() override { enterCount++; }
    void onUpdate() override { updateCount++; }
    void onExit() override { exitCount++; }
};

bool conditionTrue() { return true; }
bool conditionFalse() { return false; }

TEST(StateMachineTest, InitialState) {
    StateMachine<2> sm;
    EXPECT_EQ(sm.getState(), nullptr);
}

TEST(StateMachineTest, SetState) {
    StateMachine<2> sm;
    MockState state1;
    MockState state2;

    sm.setState(&state1);
    EXPECT_EQ(sm.getState(), &state1);
    EXPECT_TRUE(sm.isInState(&state1));
    EXPECT_EQ(state1.enterCount, 1);
    EXPECT_EQ(state1.exitCount, 0);

    sm.setState(&state2);
    EXPECT_EQ(sm.getState(), &state2);
    EXPECT_EQ(state1.exitCount, 1);
    EXPECT_EQ(state2.enterCount, 1);
}

TEST(StateMachineTest, UpdateCallsOnUpdate) {
    StateMachine<1> sm;
    MockState state;

    sm.setState(&state);
    sm.update();
    sm.update();

    EXPECT_EQ(state.updateCount, 2);
}

TEST(StateMachineTest, DoesNotTransitionOnFalseCondition) {
    StateMachine<2> sm;
    MockState state1;
    MockState state2;

    sm.addTransition(&state1, &state2, conditionFalse);
    sm.setState(&state1);
    sm.update();

    EXPECT_EQ(sm.getState(), &state1);
    EXPECT_EQ(state2.enterCount, 0);
}

TEST(StateMachineTest, TransitionsOnTrueCondition) {
    StateMachine<2> sm;
    MockState state1;
    MockState state2;

    sm.addTransition(&state1, &state2, conditionTrue);
    sm.setState(&state1);
    sm.update(); // Should transition to state2

    EXPECT_EQ(sm.getState(), &state2);
    EXPECT_EQ(state1.exitCount, 1);
    EXPECT_EQ(state2.enterCount, 1);
    EXPECT_EQ(state1.updateCount, 0); // Exited before update called
}

TEST(StateMachineTest, MaxTransitionsLimit) {
    StateMachine<1> sm;
    MockState state1;
    MockState state2;
    MockState state3;

    // We can only add 1 transition
    sm.addTransition(&state1, &state2, conditionFalse);
    // This second one should be ignored
    sm.addTransition(&state2, &state3, conditionTrue);

    sm.setState(&state2);
    sm.update();

    // Since the second transition wasn't added, it should remain in state2
    EXPECT_EQ(sm.getState(), &state2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Hack for PIO native: manually include the cpp implementation to avoid linking errors natively
#include "../src/StateMachine.cpp"
