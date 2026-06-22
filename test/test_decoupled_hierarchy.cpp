#include "Mocks.h"

// Define completely decoupled state families
class ParentStateFamily : public msm::State {
public:
    virtual void setOperatingMode(const char* mode) {}
};

class ChildStateFamily : public msm::State {};

// A mock parent state that overrides setOperatingMode
class MockParentState : public ParentStateFamily {
public:
    int enterCount = 0;
    int updateCount = 0;
    int exitCount = 0;
    const char* operatingMode = nullptr;

    void onEnter() override { enterCount++; }
    void onUpdate() override { updateCount++; }
    void onExit() override { exitCount++; }
    void setOperatingMode(const char* mode) override { operatingMode = mode; }
};

// A mock child state
class MockChildState : public ChildStateFamily {
public:
    int enterCount = 0;
    int updateCount = 0;
    int exitCount = 0;

    void onEnter() override { enterCount++; }
    void onUpdate() override { updateCount++; }
    void onExit() override { exitCount++; }
};

// A custom child state that inherits from msm::ChildState to execute parent actions on enter/exit
class CustomParentChildState : public msm::ChildState<ParentStateFamily, ChildStateFamily> {
public:
    MockParentState& parentTracker;
    int customEnterCount = 0;

    CustomParentChildState(MockParentState& tracker, msm::StateMachineBase<ChildStateFamily>& childSM)
        : msm::ChildState<ParentStateFamily, ChildStateFamily>(childSM), parentTracker(tracker) {}

    void onEnter() override {
        // Parent-specific action
        parentTracker.setOperatingMode("DRIVE_MANUAL");
        customEnterCount++;
        // Delegate to child SM entry
        msm::ChildState<ParentStateFamily, ChildStateFamily>::onEnter();
    }
};

TEST(DecoupledHierarchyTest, DecoupledCompilationAndRouting) {
    MockParentState parentIdleState;
    MockChildState childStateA;
    MockChildState childStateB;

    // Child state machine (only depends on ChildStateFamily)
    StateMachine<ChildStateFamily, 1> childSM(
        childStateA,
        {{ { childStateA, childStateB, ManualOnly(), false } }}
    );

    // Parent-specific state wrapping the child SM
    CustomParentChildState driveManualState(parentIdleState, childSM);

    // Parent state machine (only depends on ParentStateFamily)
    StateMachine<ParentStateFamily, 2> parentSM(
        parentIdleState,
        {{
            { parentIdleState, driveManualState, ManualOnly(), false },
            { driveManualState, parentIdleState, Unconditional(), true }
        }}
    );

    // 1. Initial State checks
    EXPECT_TRUE(parentSM.isInState(parentIdleState));
    EXPECT_EQ(parentIdleState.operatingMode, nullptr);

    // 2. Transition parent into the decoupled child state
    parentSM.requestTransition(driveManualState);
    parentSM.onUpdate();

    EXPECT_TRUE(parentSM.isInState(driveManualState));
    EXPECT_TRUE(childSM.isInState(childStateA));
    EXPECT_STREQ(parentIdleState.operatingMode, "DRIVE_MANUAL");
    EXPECT_EQ(driveManualState.customEnterCount, 1);
    EXPECT_EQ(childStateA.enterCount, 2);

    // 3. Tick parent SM should update child SM again
    parentSM.onUpdate();
    EXPECT_EQ(childStateA.updateCount, 2);

    // 4. Move child internally
    childSM.requestTransition(childStateB);
    parentSM.onUpdate();
    EXPECT_TRUE(childSM.isInState(childStateB));
    EXPECT_EQ(childStateB.enterCount, 1);

    // 5. Test requireFinished transition out of the child machine
    childStateB.setFinished(true);
    // Since childStateB is finished and has no further outgoing transitions,
    // childSM.isFinished() returns true, and parentSM transitions back to parentIdleState.
    parentSM.onUpdate();
    EXPECT_TRUE(parentSM.isInState(parentIdleState));
}
