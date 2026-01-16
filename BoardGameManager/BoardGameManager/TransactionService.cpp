#include "TransactionService.h"
#include <iostream>

// Helper functions for borrowed list (Member owns list)
static bool hasBorrowedGame(Member* member, int gameId) {
    BorrowedNode* cur = member->borrowedHead;
    while (cur != nullptr) {
        if (cur->gameId == gameId) return true;
        cur = cur->next;
    }
    return false;
}

static void addBorrowedGame(Member* member, int gameId) {
    BorrowedNode* node = new BorrowedNode();
    node->gameId = gameId;
    node->next = member->borrowedHead;
    member->borrowedHead = node;
}

static bool removeBorrowedGame(Member* member, int gameId) {
    BorrowedNode* cur = member->borrowedHead;
    BorrowedNode* prev = nullptr;

    while (cur != nullptr) {
        if (cur->gameId == gameId) {
            if (prev == nullptr) {
                member->borrowedHead = cur->next;
            }
            else {
                prev->next = cur->next;
            }
            delete cur;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

TransactionService::TransactionService(MemberService* ms, int eventCapacity)
    : memberService(ms), eventLog(eventCapacity) {
}

Status TransactionService::borrowGame(int memberId, int gameId) {
    // Phase 0: Validate inputs and member existence; real game checks in Phase 1
    if (memberService == nullptr || memberId <= 0 || gameId <= 0) {
        return INVALID_INPUT;
    }

    Member* m = memberService->getMemberById(memberId);
    if (m == nullptr) {
        return NOT_FOUND; // member not found
    }

    // Duplicate borrow check (rule frozen in Phase 0)
    if (hasBorrowedGame(m, gameId)) {
        return ALREADY_EXISTS;
    }

    // TODO (Phase 1): Integrate with Member A's GameService:
    // - Game* g = getGameById(gameId)
    // - if g == nullptr -> NOT_FOUND
    // - if g->availableCopies <= 0 -> NOT_AVAILABLE
    // - g->availableCopies--

    // For Phase 0, we simulate success by adding to borrowed list and logging event.
    addBorrowedGame(m, gameId);

    BorrowEvent e;
    e.memberId = memberId;
    e.gameId = gameId;
    e.action = ACTION_BORROW;
    e.timestamp = 0; // TODO: set real timestamp in Phase 1
    eventLog.enqueue(e);

    return OK;
}

Status TransactionService::returnGame(int memberId, int gameId) {
    if (memberService == nullptr || memberId <= 0 || gameId <= 0) {
        return INVALID_INPUT;
    }

    Member* m = memberService->getMemberById(memberId);
    if (m == nullptr) {
        return NOT_FOUND; // member not found
    }

    // Must have borrowed the game to return it
    if (!hasBorrowedGame(m, gameId)) {
        return NOT_BORROWED;
    }

    // TODO (Phase 1): Integrate with Member A's GameService:
    // - Game* g = getGameById(gameId)
    // - if g == nullptr -> NOT_FOUND
    // - g->availableCopies++

    // Phase 0: remove from borrowed list and log event.
    (void)removeBorrowedGame(m, gameId);

    BorrowEvent e;
    e.memberId = memberId;
    e.gameId = gameId;
    e.action = ACTION_RETURN;
    e.timestamp = 0; // TODO: set real timestamp in Phase 1
    eventLog.enqueue(e);

    return OK;
}

void TransactionService::adminBorrowReturnSummary() {
    // Phase 0: simple counts from event log
    int borrowCount = 0;
    int returnCount = 0;

    for (int i = 0; i < eventLog.getCount(); i++) {
        BorrowEvent e;
        if (eventLog.getAt(i, e)) {
            if (e.action == ACTION_BORROW) borrowCount++;
            else if (e.action == ACTION_RETURN) returnCount++;
        }
    }

    std::cout << "[Admin Summary - Phase 0]\n";
    std::cout << "Total BORROW events: " << borrowCount << "\n";
    std::cout << "Total RETURN events: " << returnCount << "\n";
    std::cout << "Total events logged: " << eventLog.getCount() << "\n";
}

void TransactionService::memberBorrowReturnSummary(int memberId) {
    // Phase 0: filter by memberId from event log
    int borrowCount = 0;
    int returnCount = 0;

    for (int i = 0; i < eventLog.getCount(); i++) {
        BorrowEvent e;
        if (eventLog.getAt(i, e)) {
            if (e.memberId == memberId) {
                if (e.action == ACTION_BORROW) borrowCount++;
                else if (e.action == ACTION_RETURN) returnCount++;
            }
        }
    }

    std::cout << "[Member Summary - Phase 0] MemberId: " << memberId << "\n";
    std::cout << "BORROW events: " << borrowCount << "\n";
    std::cout << "RETURN events: " << returnCount << "\n";
}

CircularQueue& TransactionService::getEventLog() {
    return eventLog;
}
