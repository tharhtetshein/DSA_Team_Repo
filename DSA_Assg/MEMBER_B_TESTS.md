# Member B Phase 0 Test Plan

1. Borrow success: member exists, game exists, availableCopies=1 -> borrow returns `OK`, event queued.
2. Borrow unavailable: availableCopies=0 -> borrow returns `NOT_AVAILABLE`, no event enqueued.
3. Duplicate borrow: member tries to borrow same game twice -> second call returns `ALREADY_EXISTS`.
4. Borrow invalid input: negative gameId -> `INVALID_INPUT`.
5. Borrow non-existent game: gameId not in GameService -> `NOT_FOUND`.
6. Return success: member has game -> `OK`, event queued, availableCopies incremented.
7. Return without borrow: member never borrowed game -> `NOT_BORROWED`.
8. Return invalid member: memberId missing -> `NOT_FOUND`.
9. Queue overwrite: fill queue to capacity, enqueue one more -> oldest event replaced, returns `OK`.
10. Member summary: after mixed events, summary lists active borrows and history without mutating queue.
