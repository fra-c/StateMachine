#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stddef.h>

class State {
public:
    virtual ~State() = default;

    virtual void onEnter() {}
    virtual void onUpdate() {}
    virtual void onExit() {}

    // The "= 0" means this class cannot be instantiated.
    // Every child class MUST define this, or the code will not compile.
    virtual bool isFinished() = 0;
};

class Condition {
public:
    virtual ~Condition() = default;
    virtual bool evaluate() = 0;
};

template <typename StateFamily>
struct Transition {
    StateFamily* from;
    StateFamily* to;
    Condition* condition;
    bool requireFinished;
};

template <typename StateFamily>
class StateMachineBase : public StateFamily {
public:
    StateMachineBase(Transition<StateFamily>* transitionsArray, size_t maxTransitions)
        : currentState(nullptr), transitions(transitionsArray), maxTransitions(maxTransitions), transitionCount(0) {}

    void setState(StateFamily* state) {
        if (currentState) {
            currentState->onExit();
        }
        currentState = state;
        if (currentState) {
            currentState->onEnter();
        }
    }

    StateFamily* getState() const {
        return currentState;
    }

    void addTransition(StateFamily* from, StateFamily* to, Condition* condition, bool requireFinished) {
        if (transitionCount < maxTransitions) {
            transitions[transitionCount++] = {from, to, condition, requireFinished};
        }
    }

    bool requestTransition(StateFamily* targetState) {
        for (size_t i = 0; i < transitionCount; ++i) {
            if (transitions[i].to == targetState && isValidPath(transitions[i])) {
                if (currentState != targetState) {
                    setState(targetState);
                }
                return true;
            }
        }
        return false;
    }

    void onUpdate() override {
        if (currentState) {
            for (size_t i = 0; i < transitionCount; ++i) {
                if (isValidPath(transitions[i]) && (transitions[i].condition == nullptr || transitions[i].condition->evaluate())) {
                    // Prevent endless re-entry loops if a global condition stays true
                    if (currentState != transitions[i].to) {
                        setState(transitions[i].to);
                        return;
                    }
                }
            }
            currentState->onUpdate();
        }
    }

    void onEnter() override {
        if (currentState) {
            currentState->onEnter();
        }
    }

    void onExit() override {
        if (currentState) {
            currentState->onExit();
        }
    }

    bool isFinished() override {
        return currentState ? currentState->isFinished() : true;
    }

    bool isInState(const StateFamily* state) const {
        return currentState == state;
    }

private:
    bool isValidPath(const Transition<StateFamily>& transition) const {
        if (transition.from != currentState && transition.from != nullptr) return false;
        if (transition.requireFinished && currentState && !currentState->isFinished()) return false;
        return true;
    }

    StateFamily* currentState;
    Transition<StateFamily>* transitions;
    size_t maxTransitions;
    size_t transitionCount;
};

template <typename StateFamily, size_t MAX_TRANSITIONS>
class StateMachine : public StateMachineBase<StateFamily> {
public:
    StateMachine() : StateMachineBase<StateFamily>(storage, MAX_TRANSITIONS) {}

private:
    Transition<StateFamily> storage[MAX_TRANSITIONS];
};

#endif
