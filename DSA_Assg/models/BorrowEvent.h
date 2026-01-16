#ifndef BORROW_EVENT_H
#define BORROW_EVENT_H

#include <cstring>

// Status codes shared across services
enum Status { OK, NOT_FOUND, ALREADY_EXISTS, NOT_AVAILABLE, NOT_BORROWED, INVALID_INPUT, FULL };

// Action types for borrow/return history
enum ActionType { BORROW, RETURN };

struct BorrowEvent {
    int memberId;
    int gameId;
    ActionType action;
    char timestamp[32]; // e.g., "2024-05-01 12:00"

    BorrowEvent() : memberId(0), gameId(0), action(BORROW) {
        timestamp[0] = '\0';
    }
};

#endif // BORROW_EVENT_H
