#ifndef SERVICES_TRANSACTIONSERVICE_H
#define SERVICES_TRANSACTIONSERVICE_H

#include "MemberService.h"
#include "CircularQueue.h"

// Member B - Phase 0
// Transaction service owns borrow/return rules and event logging.
// NOTE: It will integrate with Member A's GameService in Phase 1.

class TransactionService {
private:
    MemberService* memberService;  // not owned
    CircularQueue eventLog;        // owned

public:
    TransactionService(MemberService* ms, int eventCapacity = 200);

    // Borrow / Return
    Status borrowGame(int memberId, int gameId);
    Status returnGame(int memberId, int gameId);

    // Summaries
    void adminBorrowReturnSummary();
    void memberBorrowReturnSummary(int memberId);

    // Access for testing/demo tools
    CircularQueue& getEventLog();
};

#endif
