#include "StateMachine.h"

StateMachineBase::StateMachineBase(Transition* transitionsArray, size_t maxTransitions)
    : currentState(nullptr), transitions(transitionsArray), maxTransitions(maxTransitions), transitionCount(0) {}

void StateMachineBase::setState(State* state) {
    if (currentState) {
        currentState->onExit();
    }
    currentState = state;
    if (currentState) {
        currentState->onEnter();
    }
}

State* StateMachineBase::getState() const {
    return currentState;
}

void StateMachineBase::addTransition(State* from, State* to, bool (*condition)(void)) {
    if (transitionCount < maxTransitions) {
        transitions[transitionCount++] = {from, to, condition};
    }
}

void StateMachineBase::update() {
    if (currentState) {
        for (size_t i = 0; i < transitionCount; ++i) {
            const Transition& transition = transitions[i];
            if (transition.from == currentState && transition.condition()) {
                setState(transition.to);
                return;
            }
        }
        currentState->onUpdate();
    }
}

bool StateMachineBase::isInState(const State* state) const {
    return currentState == state;
}
