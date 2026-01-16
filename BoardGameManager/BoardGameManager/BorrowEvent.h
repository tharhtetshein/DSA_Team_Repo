#ifndef MODELS_BORROWEVENT_H
#define MODELS_BORROWEVENT_H

// Member B - Phase 0

enum ActionType {
    ACTION_BORROW = 0,
    ACTION_RETURN = 1
};

struct BorrowEvent {
    int memberId;
    int gameId;
    ActionType action;
    long timestamp; // Can be replaced with proper time handling later

    BorrowEvent()
        : memberId(0), gameId(0), action(ACTION_BORROW), timestamp(0) {
    }
};

#endif
