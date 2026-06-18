#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stddef.h>
#include <array>
#include <initializer_list>

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
    StateMachineBase(size_t maxTransitions)
        : currentState(nullptr), terminalState(nullptr), transitions(nullptr), maxTransitions(maxTransitions) {}

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
        if (currentState) {
            for (size_t i = 0; i < maxTransitions; ++i) {
                if (transitions[i].to == nullptr) continue;
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

    void setTerminalState(StateFamily* state) {
        terminalState = state;
    }

    bool isFinished() override {
        // If we have a terminal state defined, we are finished when we reach it
        if (terminalState != nullptr) {
            return currentState == terminalState;
        }

        // Fallback: If no terminal state is defined, the machine never finishes natively.
        // It relies on external Orchestrator interruptions.
        return false;
    }

    bool isInState(const StateFamily* state) const {
        return currentState == state;
    }

    StateFamily* getCurrentState() const {
        return currentState;
    }

private:
    bool isValidPath(const Transition<StateFamily>& transition) const {
        if (transition.from != currentState && transition.from != nullptr) return false;
        if (transition.requireFinished && currentState && !currentState->isFinished()) return false;
        return true;
    }

protected:
    StateFamily* currentState;
    StateFamily* terminalState;
    const Transition<StateFamily>* transitions;
    size_t maxTransitions;
};

template <typename StateFamily, size_t MAX_TRANSITIONS>
class StateMachine : public StateMachineBase<StateFamily> {
public:
    // 1. NEW: The Default Constructor (For creating empty machines in your tests)
    StateMachine() : StateMachineBase<StateFamily>(MAX_TRANSITIONS) {
        this->transitions = this->_transitions.data();
    }

    // 2. The Strict Array Constructor (For when you actually have rules)
    StateMachine(const std::array<Transition<StateFamily>, MAX_TRANSITIONS>& init)
        : StateMachineBase<StateFamily>(MAX_TRANSITIONS),
          _transitions(init)
    {
        this->transitions = this->_transitions.data();
    }

private:
    std::array<Transition<StateFamily>, MAX_TRANSITIONS> _transitions{};
};

#endif
