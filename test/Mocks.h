#pragma once
#include "StateMachine.h"

using namespace msm;

class MockState : public State {
public:
    int enterCount = 0;
    int updateCount = 0;
    int exitCount = 0;
    bool finished = false;

    void onEnter() override { enterCount++; }
    void onUpdate() override { updateCount++; }
    void onExit() override { exitCount++; }
    bool isFinished() override { return finished; }
};

class MockCondition : public Condition {
public:
    bool result;
    MockCondition(bool r = true) : result(r) {}
    bool evaluate() override { return result; }
};
