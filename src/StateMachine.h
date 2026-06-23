#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stddef.h>
#include <array>
#include <initializer_list>
#include <type_traits>

namespace msm {

/**
 * State is an abstract base class that defines the interface for states in the state
 * machine. Users of the library should derive their own state classes from this base class.
 */
class State {
    // Friend declarations to allow engine classes to invoke hidden lifecycle wrappers
    template <typename StateFamily> friend class StateMachineBase;
    template <typename StateFamily, size_t MAX_TRANSITIONS> friend class StateMachine;
    template <typename StateFamily, size_t NUM_REGIONS> friend class ParallelState;
    template <typename ParentFamily, typename ChildFamily> friend class ChildState;

public:
    virtual ~State() = default;

    virtual void onEnter() {}
    virtual void onUpdate(float delta_time = 0.0f) {}
    virtual void onExit() {}
    virtual bool isFinished() { return _isFinished; }

    void setFinished(bool finished) { _isFinished = finished; }

protected:
    // Hidden virtual wrapper methods
    virtual void enter() {
        onEnter();
    }
    virtual void update(float delta_time = 0.0f) {
        onUpdate(delta_time);
    }
    virtual void exit() {
        onExit();
    }

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
            _regions[i].get().enter();
        }
    }

    void onUpdate(float delta_time = 0.0f) override {
        for (size_t i = 0; i < NUM_REGIONS; ++i) {
            _regions[i].get().update(delta_time);
        }
    }

    void onExit() override {
        for (size_t i = 0; i < NUM_REGIONS; ++i) {
            _regions[i].get().exit();
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

    void onUpdate(float delta_time = 0.0f) override {
        for (size_t i = 0; i < maxTransitions; ++i) {
            if (transitions[i].to == nullptr) {
                continue;
            }

            if (isValidPath(transitions[i]) && transitions[i].condition->evaluate()) {
                // Prevent endless re-entry loops if a global condition stays true
                if (currentState != transitions[i].to) {
                    setState(*transitions[i].to);
                    currentState->update(delta_time);
                    return;
                }
            }
        }
        currentState->update(delta_time);
    }

    void onEnter() override {
        currentState = initialState;
        currentState->setFinished(false);
        currentState->enter();
    }

    void onExit() override {
        currentState->exit();
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
        currentState->exit();
        currentState = &state;
        currentState->setFinished(false);
        currentState->enter();
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
        this->currentState->enter();
    }

private:
    std::array<Transition<StateFamily>, MAX_TRANSITIONS> _transitions{};
};

/**
 * ConditionMethodAdapter is a template class that adapts a member function of an existing class to the Condition interface.
 * It allows users to use member functions as conditions in the state machine without having to derive from
 * the Condition base class.
 */
template <typename T, auto MethodPtr>
class ConditionMethodAdapter : public Condition {
private:
    T* _instance;

public:
    // Constructor only needs the instance pointer!
    ConditionMethodAdapter(T* instance) : _instance(instance) {}

    bool evaluate() override {
        // Invokes the compile-time bound method pointer
        return (_instance->*MethodPtr)();
    }
};

/**
 * ConditionFunctionAdapter is a class that adapts a raw C function pointer to the Condition interface.
 * It allows users to use standalone functions as conditions in the state machine without having to derive from
 * the Condition base class.
 */
template <auto FuncPtr>
class ConditionFunctionAdapter : public Condition {
public:
    bool evaluate() override {
        return FuncPtr(); // Resolved entirely at compile time!
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

/**
 * ChildState is an adapter class that allows a StateMachine belonging to one StateFamily
 * to be nested as a state inside another StateMachine of a different StateFamily.
 * This completely decouples parent and child state machines, ensuring type safety.
 */
template <typename ParentFamily, typename ChildFamily>
class ChildState : public ParentFamily {
    static_assert(std::is_base_of<State, ParentFamily>::value, "ParentFamily must derive from msm::State");
    static_assert(std::is_base_of<State, ChildFamily>::value, "ChildFamily must derive from msm::State");

public:
    constexpr ChildState(StateMachineBase<ChildFamily>& childSM)
        : _childSM(childSM) {}

    bool isFinished() override {
        return _childSM.isFinished();
    }

    constexpr StateMachineBase<ChildFamily>& getChildStateMachine() const {
        return _childSM;
    }

protected:
    // Hidden virtual wrapper overrides. Subclasses are prevented from overriding these.
    void enter() override final {
        this->onEnter();
        _childSM.enter();
    }

    void update(float delta_time = 0.0f) override final {
        this->onUpdate(delta_time);
        _childSM.update(delta_time);
    }

    void exit() override final {
        this->onExit();
        _childSM.exit();
    }

private:
    StateMachineBase<ChildFamily>& _childSM;
};

}// namespace msm

#endif
