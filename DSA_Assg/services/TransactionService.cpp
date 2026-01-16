#include "TransactionService.h"

TransactionService::TransactionService(MemberService* mService, CircularQueue* queue)
    : memberService(mService), eventQueue(queue) {}

Status TransactionService::borrowGame(int memberId, int gameId) {
    if (!memberService || !eventQueue) {
        return INVALID_INPUT;
    }

    // TODO: Phase 1 - validate member exists, check duplicate borrow, and game availability
    // TODO: Call Member A's GameService: getGameById(gameId) and decrement availableCopies if available

    BorrowEvent event;
    event.memberId = memberId;
    event.gameId = gameId;
    event.action = BORROW;
    // TODO: Phase 1 - populate timestamp

    eventQueue->enqueue(event);
    return OK;
}

Status TransactionService::returnGame(int memberId, int gameId) {
    if (!memberService || !eventQueue) {
        return INVALID_INPUT;
    }

    // TODO: Phase 1 - validate member currently borrowed the game
    // TODO: Call Member A's GameService to increment availableCopies

    BorrowEvent event;
    event.memberId = memberId;
    event.gameId = gameId;
    event.action = RETURN;
    // TODO: Phase 1 - populate timestamp

    eventQueue->enqueue(event);
    return OK;
}

Status TransactionService::memberBorrowReturnSummary(int memberId) {
    // TODO: Phase 1 - aggregate borrow/return events and current borrows for the member
    // NOTE: member summaries should not modify queue; just read from it.
    return OK;
}

Status TransactionService::adminBorrowReturnSummary() {
    // TODO: Phase 1 - summarize all events (borrow/return totals, queue iteration)
    // NOTE: This is where Member C's RatingService might be consulted if summaries include ratings.
    return OK;
}
