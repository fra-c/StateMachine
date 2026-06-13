#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stddef.h>

class State {
public:
    virtual ~State() = default;

    virtual void onEnter() {}
    virtual void onUpdate() {}
    virtual void onExit() {}
};

struct Transition {
    State* from;
    State* to;
    bool (*condition)(void);
};

class StateMachineBase {
public:
    StateMachineBase(Transition* transitionsArray, size_t maxTransitions);

    void setState(State* state);
    State* getState() const;
    void addTransition(State* from, State* to, bool (*condition)(void));
    void update();
    bool isInState(const State* state) const;

private:
    State* currentState;
    Transition* transitions;
    size_t maxTransitions;
    size_t transitionCount;
};

template <size_t MAX_TRANSITIONS>
class StateMachine : public StateMachineBase {
public:
    StateMachine() : StateMachineBase(storage, MAX_TRANSITIONS) {}

private:
    Transition storage[MAX_TRANSITIONS];
};

#endif
