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

TransactionService::TransactionService(MemberService* ms, GameService* gs, int eventCapacity)
    : memberService(ms), gameService(gs), eventLog(eventCapacity) {
}

Status TransactionService::borrowGame(int memberId, int gameId) {
    // Phase 0: Validate inputs and member existence; real game checks in Phase 1
    if (memberService == nullptr || gameService == nullptr || memberId <= 0 || gameId <= 0) {
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

    Game* g = gameService->findById(gameId);
    if (g == nullptr) {
        return NOT_FOUND;
    }
    if (g->copiesAvailable <= 0) {
        return NOT_AVAILABLE;
    }

    g->copiesAvailable -= 1;
    if (g->copiesAvailable <= 0) {
        g->status = GameStatus::BORROWED;
    }

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
    if (memberService == nullptr || gameService == nullptr || memberId <= 0 || gameId <= 0) {
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

    Game* g = gameService->findById(gameId);
    if (g == nullptr) {
        return NOT_FOUND;
    }

    g->copiesAvailable += 1;
    g->status = GameStatus::AVAILABLE;

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

    std::cout << "[Admin Summary]\n";
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

    std::cout << "[Member Summary] MemberId: " << memberId << "\n";
    std::cout << "BORROW events: " << borrowCount << "\n";
    std::cout << "RETURN events: " << returnCount << "\n";
}

void TransactionService::adminBorrowReturnSummaryByGame() {
    if (gameService == nullptr) {
        return;
    }

    Game** all = nullptr;
    int total = gameService->getAllGames(all);
    if (total <= 0 || all == nullptr) {
        std::cout << "No games loaded.\n";
        return;
    }

    std::cout << "[Admin Summary - By Game]\n";
    for (int i = 0; i < total; i++) {
        int borrowCount = 0;
        int returnCount = 0;

        for (int j = 0; j < eventLog.getCount(); j++) {
            BorrowEvent e;
            if (eventLog.getAt(j, e)) {
                if (e.gameId == all[i]->gameID) {
                    if (e.action == ACTION_BORROW) borrowCount++;
                    else if (e.action == ACTION_RETURN) returnCount++;
                }
            }
        }

        std::cout << "GameID " << all[i]->gameID << " - " << all[i]->title
            << " | Borrow: " << borrowCount << " Return: " << returnCount << "\n";
    }

    delete[] all;
}

CircularQueue& TransactionService::getEventLog() {
    return eventLog;
}
