#pragma once
#include "StateMachine.h"
#include <gtest/gtest.h>

using namespace msm;

class MockState : public State {
public:
    int enterCount = 0;
    int updateCount = 0;
    int exitCount = 0;

    void onEnter() override { enterCount++; }
    void onUpdate(float = 0.0f) override { updateCount++; }
    void onExit() override { exitCount++; }
};

class MockCondition : public Condition {
public:
    bool result;
    MockCondition(bool r = true) : result(r) {}
    bool evaluate() override { return result; }
};

