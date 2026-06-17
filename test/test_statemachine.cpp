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
    bool isFinished() override { return false; }
};

class MockCondition : public Condition {
public:
    bool result;
    MockCondition(bool r = true) : result(r) {}
    bool evaluate() override { return result; }
};

TEST(StateMachineTest, InitialState) {
    StateMachine<State, 2> sm;
    EXPECT_EQ(sm.getState(), nullptr);
}

TEST(StateMachineTest, SetState) {
    StateMachine<State, 2> sm;
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
    StateMachine<State, 1> sm;
    MockState state;

    sm.setState(&state);
    sm.onUpdate();
    sm.onUpdate();

    EXPECT_EQ(state.updateCount, 2);
}

TEST(StateMachineTest, DoesNotTransitionOnFalseCondition) {
    StateMachine<State, 2> sm;
    MockState state1;
    MockState state2;
    MockCondition condFalse(false);

    sm.addTransition(&state1, &state2, &condFalse, false);
    sm.setState(&state1);
    sm.onUpdate();

    EXPECT_EQ(sm.getState(), &state1);
    EXPECT_EQ(state2.enterCount, 0);
}

TEST(StateMachineTest, TransitionsOnTrueCondition) {
    StateMachine<State, 2> sm;
    MockState state1;
    MockState state2;
    MockCondition condTrue(true);

    sm.addTransition(&state1, &state2, &condTrue, false);
    sm.setState(&state1);
    sm.onUpdate(); // Should transition to state2

    EXPECT_EQ(sm.getState(), &state2);
    EXPECT_EQ(state1.exitCount, 1);
    EXPECT_EQ(state2.enterCount, 1);
    EXPECT_EQ(state1.updateCount, 0); // Exited before update called
}

TEST(StateMachineTest, MaxTransitionsLimit) {
    StateMachine<State, 1> sm;
    MockState state1;
    MockState state2;
    MockState state3;
    MockCondition condTrue(true);
    MockCondition condFalse(false);

    // We can only add 1 transition
    sm.addTransition(&state1, &state2, &condFalse, false);
    // This second one should be ignored
    sm.addTransition(&state2, &state3, &condTrue, false);

    sm.setState(&state2);
    sm.onUpdate();

    // Since the second transition wasn't added, it should remain in state2
    EXPECT_EQ(sm.getState(), &state2);
}

// A condition that evaluates if a given state is finished
class IsFinishedCondition : public Condition {
    State* state;
public:
    IsFinishedCondition(State* s) : state(s) {}
    bool evaluate() override { return state->isFinished(); }
};

class MockFinishingState : public MockState {
    bool finished = false;
public:
    void setFinished(bool f) { finished = f; }
    bool isFinished() override { return finished; }
};

TEST(StateMachineTest, HierarchicalStateMachine) {
    StateMachine<State, 2> parentSM;
    StateMachine<State, 1> childSM;

    MockState state1;
    MockState state2; // Target state for parent
    MockFinishingState childState1;
    MockFinishingState childState2;

    MockCondition condTrue(true);
    IsFinishedCondition childFinished(&childSM);

    // Child SM setup
    childSM.addTransition(&childState1, &childState2, &condTrue, false);
    childSM.setState(&childState1);

    // Parent SM setup: transition from state1 -> childSM -> state2
    parentSM.addTransition(&state1, &childSM, &condTrue, false);
    parentSM.addTransition(&childSM, &state2, &childFinished, false);

    parentSM.setState(&state1);

    // Initial state: state1
    EXPECT_EQ(parentSM.getState(), &state1);

    // Update 1: transitions parent from state1 -> childSM
    // Because childSM is a State, onEnter will be called, propagating to childState1
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &childSM);

    // Update 2: Inside childSM, transitions from childState1 -> childState2
    parentSM.onUpdate();
    EXPECT_EQ(childSM.getState(), &childState2);
    // At this point, childSM is NOT finished yet
    EXPECT_EQ(parentSM.getState(), &childSM);

    // Report finishing from childState2
    childState2.setFinished(true);

    // Update 3: parentSM evaluates transitions, childSM is finished -> transitions to state2
    parentSM.onUpdate();
    EXPECT_EQ(parentSM.getState(), &state2);
}

TEST(StateMachineTest, GlobalAnyStateTransition) {
    StateMachine<State, 2> sm;
    MockState state1;
    MockState state2;
    MockState panicState;
    MockCondition panicCondition(true);

    // Normal transition
    sm.addTransition(&state1, &state2, new MockCondition(false), false);

    // Global transition (from == nullptr)
    sm.addTransition(nullptr, &panicState, &panicCondition, false);

    sm.setState(&state1);

    // The panic condition is true, so it should jump to panicState ignoring state1->state2
    sm.onUpdate();
    EXPECT_EQ(sm.getState(), &panicState);
    EXPECT_EQ(state1.exitCount, 1);
    EXPECT_EQ(panicState.enterCount, 1);

    // Evaluate again, panic condition still true.
    // It should NOT re-enter panicState (preventing endless self-transition loop).
    sm.onUpdate();
    EXPECT_EQ(panicState.exitCount, 0);
    EXPECT_EQ(panicState.enterCount, 1);
}

TEST(StateMachineTest, RequireFinishedTransition) {
    StateMachine<State, 2> sm;
    MockFinishingState state1;
    MockState state2;
    MockCondition condTrue(true);

    // This transition requires state1 to be finished
    sm.addTransition(&state1, &state2, &condTrue, true);
    sm.setState(&state1);

    // Initial update: condition is true but state1 is NOT finished, so it shouldn't transition
    sm.onUpdate();
    EXPECT_EQ(sm.getState(), &state1);
    EXPECT_EQ(state2.enterCount, 0);

    // Mark it as finished
    state1.setFinished(true);

    // Update again: condition is true and state1 IS finished, so it should transition
    sm.onUpdate();
    EXPECT_EQ(sm.getState(), &state2);
    EXPECT_EQ(state1.exitCount, 1);
    EXPECT_EQ(state2.enterCount, 1);
}

TEST(StateMachineTest, RequestTransitionValidPath) {
    StateMachine<State, 2> sm;
    MockState state1;
    MockState state2;
    MockCondition condFalse(false); // Condition is false, so update() wouldn't naturally transition

    sm.addTransition(&state1, &state2, &condFalse, false);
    sm.setState(&state1);

    // Requesting bypassing the condition check
    bool result = sm.requestTransition(&state2);
    EXPECT_TRUE(result);
    EXPECT_EQ(sm.getState(), &state2);
}

TEST(StateMachineTest, RequestTransitionInvalidPath) {
    StateMachine<State, 2> sm;
    MockState state1;
    MockState state2;
    MockState state3;
    MockCondition condFalse(false);

    sm.addTransition(&state1, &state2, &condFalse, false);
    sm.setState(&state1);

    // No valid path exists from state1 to state3
    bool result = sm.requestTransition(&state3);
    EXPECT_FALSE(result);
    EXPECT_EQ(sm.getState(), &state1);
}

TEST(StateMachineTest, RequestTransitionRequireFinished) {
    StateMachine<State, 2> sm;
    MockFinishingState state1;
    MockState state2;
    MockCondition condFalse(false);

    sm.addTransition(&state1, &state2, &condFalse, true); // Requires state1 to finish
    sm.setState(&state1);

    // Path exists, but state1 is NOT finished yet
    bool result = sm.requestTransition(&state2);
    EXPECT_FALSE(result);
    EXPECT_EQ(sm.getState(), &state1);

    // Mark as finished and try again
    state1.setFinished(true);
    result = sm.requestTransition(&state2);
    EXPECT_TRUE(result);
    EXPECT_EQ(sm.getState(), &state2);
}

TEST(StateMachineTest, NullConditionEvaluatesTrue) {
    StateMachine<State, 2> sm;
    MockState state1;
    MockState state2;

    // Passing nullptr as the condition
    sm.addTransition(&state1, &state2, nullptr, false);
    sm.setState(&state1);
    sm.onUpdate();

    // Since condition is nullptr, it evaluates to true and transitions immediately
    EXPECT_EQ(sm.getState(), &state2);
    EXPECT_EQ(state1.exitCount, 1);
    EXPECT_EQ(state2.enterCount, 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Hack for PIO native: manually include the cpp implementation to avoid linking errors natively
#include "../src/StateMachine.cpp"
