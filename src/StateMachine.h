#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

class State {
public:
    virtual ~State() = default;

    virtual void onEnter() {}
    virtual void onUpdate() {}
    virtual void onExit() {}
};

template <size_t MAX_TRANSITIONS>
class StateMachine {
public:
    StateMachine() : currentState(nullptr), transitionCount(0) {}

    void setState(State* state) {
        if (currentState) {
            currentState->onExit();
        }
        currentState = state;
        if (currentState) {
            currentState->onEnter();
        }
    }

    void addTransition(State* from, State* to, bool (*condition)(void)) {
        if (transitionCount < MAX_TRANSITIONS) {
            transitions[transitionCount++] = {from, to, condition};
        }
    }

    void update() {
        if (currentState) {
            for (int i = 0; i < transitionCount; ++i) {
                const Transition& transition = transitions[i];
                if (transition.from == currentState && transition.condition()) {
                    setState(transition.to);
                    return;
                }
            }
            currentState->onUpdate();
        }
    }

    bool isInState(const State* state) const {
        return currentState == state;
    }

private:
    struct Transition {
        State* from;
        State* to;
        bool (*condition)(void);
    };

    State* currentState;
    Transition transitions[MAX_TRANSITIONS];
    size_t transitionCount;
};

#endif;
