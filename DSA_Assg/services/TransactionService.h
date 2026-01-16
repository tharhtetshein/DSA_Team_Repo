#ifndef TRANSACTION_SERVICE_H
#define TRANSACTION_SERVICE_H

#include "MemberService.h"
#include "../ds/CircularQueue.h"

class TransactionService {
private:
    MemberService* memberService;
    CircularQueue* eventQueue;

public:
    TransactionService(MemberService* mService, CircularQueue* queue);

    Status borrowGame(int memberId, int gameId);
    Status returnGame(int memberId, int gameId);

    // summaries will be computed from BorrowEvent history and current borrows
    Status memberBorrowReturnSummary(int memberId);
    Status adminBorrowReturnSummary();
};

#endif // TRANSACTION_SERVICE_H
