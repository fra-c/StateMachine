#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stddef.h>
#include <array>
#include <initializer_list>

namespace msm {

/**
 * State is an abstract base class that defines the interface for states in the state
 * machine. Users of the library should derive their own state classes from this base class.
 */
class State {
public:
    virtual ~State() = default;

    virtual void onEnter() {}
    virtual void onUpdate() {}
    virtual void onExit() {}
    virtual bool isFinished() { return _isFinished; }

    void setFinished(bool finished) { _isFinished = finished; }
protected:
    bool _isFinished = false;
};

/**
 * ParallelState implements Orthogonal Regions (concurrent state machines).
 * It acts as a single composite State that manages multiple independent sub-machines
 * running in parallel without relying on the heap.
 */
template <typename StateFamily, size_t NUM_REGIONS>
class ParallelState : public StateFamily {
    static_assert(NUM_REGIONS > 0, "A ParallelState MUST manage at least one orthogonal region.");

public:
    // API requires an array of reference_wrappers
    constexpr ParallelState(const std::array<std::reference_wrapper<StateFamily>, NUM_REGIONS>& regions)
        : _regions(regions) {}

    void onEnter() override {
        for (size_t i = 0; i < NUM_REGIONS; ++i) {
            // We use .get() to unwrap the reference safely. No null checks needed!
            _regions[i].get().onEnter();
        }
    }

    void onUpdate() override {
        for (size_t i = 0; i < NUM_REGIONS; ++i) {
            _regions[i].get().onUpdate();
        }
    }

    void onExit() override {
        for (size_t i = 0; i < NUM_REGIONS; ++i) {
            _regions[i].get().onExit();
        }
    }

    bool isFinished() override {
        for (size_t i = 0; i < NUM_REGIONS; ++i) {
            if (!_regions[i].get().isFinished()) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<std::reference_wrapper<StateFamily>, NUM_REGIONS> _regions;
};

/**
 * Condition is an abstract base class that defines the interface for conditions in the state
 * machine. Users of the library could derive their own condition classes from this base class or use the provided
 * adapters to adapt existing classes or functions.
 */
class Condition {
public:
    virtual ~Condition() = default;
    virtual bool evaluate() = 0;
};

/**
 * Transition represents a state transition in the state machine.
 * It contains the source state, the target state, the condition that must be met for the transition to occur,
 * and a flag indicating whether the source state must be finished before the transition can occur.
 */
template <typename StateFamily>
struct Transition {
    StateFamily* from;
    StateFamily* to;
    Condition* condition;
    bool requireFinished;

    constexpr Transition(StateFamily& f, StateFamily& t, Condition& c, bool r = false)
        : from(&f), to(&t), condition(&c), requireFinished(r) {}

    // 2. WILDCARD CONSTRUCTOR: Omits 'from' entirely.
    // Automatically sets 'from' to nullptr internally.
    constexpr Transition(StateFamily& t, Condition& c, bool r = false)
        : from(nullptr), to(&t), condition(&c), requireFinished(r) {}

    // 3. PADDING CONSTRUCTOR: Kept strictly for std::array zero-initialization
    constexpr Transition()
        : from(nullptr), to(nullptr), condition(nullptr), requireFinished(false) {}
};

/**
 * StateMachineBase is a base class for the state machine that manages state transitions and updates.
 * It is templated on the StateFamily type, which should be derived from the State class
 */
template <typename StateFamily>
class StateMachineBase : public StateFamily {
public:
    // FIX: Require a strict reference
    StateMachineBase(size_t maxTransitions, StateFamily& initialState)
        : currentState(&initialState), initialState(&initialState), transitions(nullptr), maxTransitions(maxTransitions) {}

    StateFamily* getState() const {
        return currentState;
    }

    // FIX: Require a strict reference to match the new No-Null API
    bool requestTransition(StateFamily& targetState) {
        for (size_t i = 0; i < maxTransitions; ++i) {
            if (transitions[i].to == nullptr) continue;

            if (transitions[i].to == &targetState && isValidPath(transitions[i])) {
                if (currentState != &targetState) {
                    setState(targetState); // Safely passes the reference
                }
                return true;
            }
        }
        return false;
    }

    void onUpdate() override {
        for (size_t i = 0; i < maxTransitions; ++i) {
            if (transitions[i].to == nullptr) {
                continue;
            }

            if (isValidPath(transitions[i]) && transitions[i].condition->evaluate()) {
                // Prevent endless re-entry loops if a global condition stays true
                if (currentState != transitions[i].to) {
                    setState(*transitions[i].to);
                    currentState->onUpdate();
                    return;
                }
            }
        }
        currentState->onUpdate();
    }

    void onEnter() override {
        currentState = initialState;
        currentState->setFinished(false);
        currentState->onEnter();
    }

    void onExit() override {
        currentState->onExit();
    }

    bool isFinished() override {
        if (!currentState->isFinished()) {
            return false;
        }
        for (size_t i = 0; i < maxTransitions; ++i) {
            if (transitions[i].to == nullptr) {
                continue;
            }
            if (isValidPath(transitions[i])) {
                return false;
            }
        }
        return true;
    }

    bool isInState(const StateFamily& state) const {
        return currentState == &state;
    }

private:
    bool isValidPath(const Transition<StateFamily>& transition) const {
        if (transition.from != currentState && transition.from != nullptr) {
            return false;
        }
        if (transition.requireFinished && !currentState->isFinished()) {
            return false;
        }
        return true;
    }

    void setState(StateFamily& state) {
        currentState->onExit();
        currentState = &state;
        currentState->setFinished(false);
        currentState->onEnter();
    }

protected:
    StateFamily* currentState;
    StateFamily* initialState;
    const Transition<StateFamily>* transitions;
    size_t maxTransitions;
};

/**
 * StateMachine is a concrete implementation of the state machine that manages state transitions and updates.
 * It is templated on the StateFamily type, which should be derived from the State class, and the maximum number of
 * transitions that can be defined in the state machine.
 */
template <typename StateFamily, size_t MAX_TRANSITIONS>
class StateMachine : public StateMachineBase<StateFamily> {
    static_assert(MAX_TRANSITIONS > 0, "A StateMachine MUST have at least one transition defined.");

public:
    StateMachine(StateFamily& initialState,
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

/**
 * ConditionAdapter is a template class that adapts an existing class with an evaluate() method to the Condition
 * interface. It allows users to use their own classes as conditions in the state machine without having to derive from
 * the Condition base class.
 */
template <typename T>
class ConditionAdapter : public Condition {
private:
    T* _target;
public:
    ConditionAdapter(T* target) : _target(target) {}

    bool evaluate() override {
        return _target->evaluate();
    }
};

/**
 * MethodAdapter is a template class that adapts a member function of an existing class to the Condition interface.
 * It allows users to use member functions as conditions in the state machine without having to derive from
 * the Condition base class.
 */
template <typename T>
class MethodAdapter : public Condition {
private:
    T* _instance;
    bool (T::*_method)();

public:
    MethodAdapter(T* instance, bool (T::*method)())
        : _instance(instance), _method(method) {}

    bool evaluate() override {
        return (_instance->*_method)();
    }
};

/**
 * FunctionAdapter is a class that adapts a raw C function pointer to the Condition interface.
 * It allows users to use standalone functions as conditions in the state machine without having to derive from
 * the Condition base class.
 */
class FunctionAdapter : public Condition {
private:
    bool (*_func)(); // Raw C function pointer

public:
    FunctionAdapter(bool (*func)()) : _func(func) {}

    bool evaluate() override {
        return _func();
    }
};

/**
 * UnconditionalTransition is a class that represents a transition that is always valid.
 * It always evaluates to true, allowing the state machine to transition unconditionally.
 */
class UnconditionalTransition : public Condition {
public:
    bool evaluate() override { return true; }
};

// FIX: Return a reference to match the new Transition constructor
inline Condition& Unconditional() {
    static UnconditionalTransition instance;
    return instance;
}

/**
 * ManualOnlyTransition is a class that represents a transition that can only be triggered manually.
 * It always evaluates to false, preventing automatic transitions in the state machine.
 */
class ManualOnlyTransition : public Condition {
public:
    bool evaluate() override { return false; }
};

// FIX: Return a reference to match the new Transition constructor
inline Condition& ManualOnly() {
    static ManualOnlyTransition instance;
    return instance;
}

}// namespace msm

#endif
