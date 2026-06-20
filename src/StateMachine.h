#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stddef.h>
#include <array>
#include <initializer_list>

namespace msm {

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
    StateMachineBase(size_t maxTransitions, StateFamily* initialState)
        : currentState(initialState), initialState(initialState), transitions(nullptr), maxTransitions(maxTransitions) {}

    StateFamily* getState() const {
        return currentState;
    }

    bool requestTransition(StateFamily* targetState) {
        for (size_t i = 0; i < maxTransitions; ++i) {
            if (transitions[i].to == nullptr) continue;
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
        for (size_t i = 0; i < maxTransitions; ++i) {
            if (transitions[i].to == nullptr) continue;
            if (isValidPath(transitions[i]) && transitions[i].condition->evaluate()) {
                // Prevent endless re-entry loops if a global condition stays true
                if (currentState != transitions[i].to) {
                    setState(transitions[i].to);
                    currentState->onUpdate();
                    return;
                }
            }
        }
        currentState->onUpdate();
    }

    void onEnter() override {
        currentState = initialState;
        currentState->onEnter();
    }

    void onExit() override {
        currentState->onExit();
    }

    bool isFinished() override {
        return currentState->isFinished();
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

    void setState(StateFamily* state) {
        currentState->onExit();
        currentState = state;
        currentState->onEnter();
    }

protected:
    StateFamily* currentState;
    StateFamily* initialState;
    const Transition<StateFamily>* transitions;
    size_t maxTransitions;
};

template <typename StateFamily, size_t MAX_TRANSITIONS>
class StateMachine : public StateMachineBase<StateFamily> {
    static_assert(MAX_TRANSITIONS > 0, "A StateMachine MUST have at least one transition defined.");

public:
    StateMachine(StateFamily* initialState,
                 const std::array<Transition<StateFamily>, MAX_TRANSITIONS>& init)
        : StateMachineBase<StateFamily>(MAX_TRANSITIONS, initialState),
          _transitions(init)
    {
        this->transitions = this->_transitions.data();
        this->currentState->onEnter();
    }

private:
    std::array<Transition<StateFamily>, MAX_TRANSITIONS> _transitions{};
};


class UnconditionalTransition : public Condition {
public:
    bool evaluate() override { return true; }
};

inline Condition* Unconditional() {
    static UnconditionalTransition instance;
    return &instance;
}

class ManualOnlyTransition : public Condition {
public:
    bool evaluate() override { return false; }
};

inline Condition* ManualOnly() {
    static ManualOnlyTransition instance;
    return &instance;
}

}// namespace msm

#endif
