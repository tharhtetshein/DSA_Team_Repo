#include "TransactionService.h"
#include <iostream>
#include <ctime>

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

static int countBorrowedGames(Member* member) {
    if (member == nullptr) {
        return 0;
    }

    int count = 0;
    BorrowedNode* cur = member->borrowedHead;
    while (cur != nullptr) {
        count++;
        cur = cur->next;
    }
    return count;
}

static int countBorrowedGamesAll(MemberService* ms, int& outBorrowers) {
    outBorrowers = 0;
    if (ms == nullptr) {
        return 0;
    }

    MemberList& list = ms->getMemberList();
    MemberNode* cur = list.getHead();
    int total = 0;
    while (cur != nullptr) {
        int memberCount = countBorrowedGames(&(cur->data));
        if (memberCount > 0) {
            outBorrowers++;
        }
        total += memberCount;
        cur = cur->next;
    }
    return total;
}

static void printBorrowedGames(Member* member, GameService* gs) {
    if (member == nullptr) {
        std::cout << "None\n";
        return;
    }

    BorrowedNode* cur = member->borrowedHead;
    if (cur == nullptr) {
        std::cout << "None\n";
        return;
    }

    bool first = true;
    while (cur != nullptr) {
        if (!first) {
            std::cout << ", ";
        }
        Game* g = (gs != nullptr) ? gs->findById(cur->gameId) : nullptr;
        if (g != nullptr && g->title[0] != '\0') {
            std::cout << cur->gameId << " - " << g->title;
        }
        else {
            std::cout << cur->gameId;
        }
        first = false;
        cur = cur->next;
    }
    std::cout << "\n";
}

static void printBorrowedByMember(MemberService* ms, GameService* gs) {
    if (ms == nullptr) {
        return;
    }

    MemberList& list = ms->getMemberList();
    MemberNode* cur = list.getHead();
    bool any = false;
    while (cur != nullptr) {
        int borrowedCount = countBorrowedGames(&(cur->data));
        if (borrowedCount > 0) {
            any = true;
            std::cout << cur->data.name << " (ID " << cur->data.memberId << "): ";
            printBorrowedGames(&(cur->data), gs);
        }
        cur = cur->next;
    }

    if (!any) {
        std::cout << "No active borrowed games.\n";
    }
}

static void formatTimestamp(long ts, char* out, int size) {
    if (out == nullptr || size <= 0) {
        return;
    }

    if (ts <= 0) {
        strncpy_s(out, size, "Unknown (backfilled)", _TRUNCATE);
        out[size - 1] = '\0';
        return;
    }

    std::time_t t = (std::time_t)ts;
    std::tm localTime;
    if (localtime_s(&localTime, &t) != 0) {
        strncpy_s(out, size, "Unknown", _TRUNCATE);
        out[size - 1] = '\0';
        return;
    }

    if (std::strftime(out, size, "%Y-%m-%d %H:%M", &localTime) == 0) {
        strncpy_s(out, size, "Unknown", _TRUNCATE);
        out[size - 1] = '\0';
    }
}

static void printEventLine(const BorrowEvent& e, MemberService* ms, GameService* gs) {
    char timeBuf[32];
    formatTimestamp(e.timestamp, timeBuf, sizeof(timeBuf));

    const char* action = (e.action == ACTION_BORROW) ? "BORROW" : "RETURN";
    Member* m = (ms != nullptr) ? ms->getMemberById(e.memberId) : nullptr;
    Game* g = (gs != nullptr) ? gs->findById(e.gameId) : nullptr;

    std::cout << timeBuf << " | " << action << " | ";
    if (m != nullptr && m->name[0] != '\0') {
        std::cout << m->name << " (ID " << m->memberId << ")";
    } else {
        std::cout << "Member " << e.memberId;
    }
    std::cout << " | ";
    if (g != nullptr && g->title[0] != '\0') {
        std::cout << g->title << " (ID " << g->gameID << ")";
    } else {
        std::cout << "Game " << e.gameId;
    }
    std::cout << "\n";
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
    e.timestamp = (long)std::time(nullptr);
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
    e.timestamp = (long)std::time(nullptr);
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

    std::cout << "\n[Admin Summary]\n";
    std::cout << "Note: event counts reflect saved history (up to log capacity).\n";
    if (eventLog.getCount() == 0) {
        std::cout << "No borrow/return events recorded yet.\n";
    }
    std::cout << "Borrow events: " << borrowCount << "\n";
    std::cout << "Return events: " << returnCount << "\n";
    std::cout << "Net outstanding (borrow - return): " << (borrowCount - returnCount) << "\n";
    std::cout << "Total events logged: " << eventLog.getCount() << "\n";

    int borrowerCount = 0;
    int currentBorrowed = countBorrowedGamesAll(memberService, borrowerCount);
    std::cout << "Currently borrowed items: " << currentBorrowed
        << " across " << borrowerCount << " member(s)\n";

    std::cout << "Borrowed items by member:\n";
    printBorrowedByMember(memberService, gameService);

    std::cout << "Recent events (last 10):\n";
    if (eventLog.getCount() == 0) {
        std::cout << "None\n";
    } else {
        int total = eventLog.getCount();
        int show = (total > 10) ? 10 : total;
        int start = total - show;
        for (int i = start; i < total; i++) {
            BorrowEvent e;
            if (eventLog.getAt(i, e)) {
                printEventLine(e, memberService, gameService);
            }
        }
    }
}

void TransactionService::memberBorrowReturnSummary(int memberId) {
    // Phase 0: filter by memberId from event log
    if (memberService == nullptr) {
        std::cout << "Member data unavailable.\n";
        return;
    }

    Member* member = memberService->getMemberById(memberId);
    if (member == nullptr) {
        std::cout << "Member not found.\n";
        return;
    }

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

    std::cout << "\n[Member Summary] " << member->name << " (ID " << memberId << ")\n";
    if (eventLog.getCount() == 0) {
        std::cout << "No borrow/return events recorded yet.\n";
    }
    std::cout << "Borrow events: " << borrowCount << "\n";
    std::cout << "Return events: " << returnCount << "\n";
    std::cout << "Net outstanding (borrow - return): " << (borrowCount - returnCount) << "\n";
    int currentBorrowed = countBorrowedGames(member);
    std::cout << "Currently borrowed: " << currentBorrowed << "\n";
    std::cout << "Borrowed games: ";
    printBorrowedGames(member, gameService);

    std::cout << "Recent events (last 10):\n";
    int total = eventLog.getCount();
    BorrowEvent recent[10];
    int recentCount = 0;
    for (int i = 0; i < total; i++) {
        BorrowEvent e;
        if (eventLog.getAt(i, e) && e.memberId == memberId) {
            if (recentCount < 10) {
                recent[recentCount++] = e;
            } else {
                for (int j = 1; j < 10; j++) {
                    recent[j - 1] = recent[j];
                }
                recent[9] = e;
            }
        }
    }

    if (recentCount == 0) {
        std::cout << "None\n";
    } else {
        for (int i = 0; i < recentCount; i++) {
            printEventLine(recent[i], memberService, gameService);
        }
    }
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

    std::cout << "\n[Admin Summary - By Game]\n";
    std::cout << "Format: GameID - Title | Borrow: X Return: Y Net: Z | Available: A/T\n";
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

        int net = borrowCount - returnCount;
        std::cout << "GameID " << all[i]->gameID << " - " << all[i]->title
            << " | Borrow: " << borrowCount << " Return: " << returnCount
            << " Net: " << net
            << " | Available: " << all[i]->copiesAvailable << "/" << all[i]->copiesTotal << "\n";
    }

    delete[] all;
}

CircularQueue& TransactionService::getEventLog() {
    return eventLog;
}
