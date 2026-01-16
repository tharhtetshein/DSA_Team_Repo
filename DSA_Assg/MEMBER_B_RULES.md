# Member B Borrow/Return Rules Spec

## Borrow Validations
- Member must exist; otherwise return `NOT_FOUND`.
- Game must exist via Member A's GameService; otherwise `NOT_FOUND`.
- `availableCopies` must be > 0; otherwise `NOT_AVAILABLE`.
- Member cannot borrow the same game twice simultaneously; if already in `BorrowedNode` list, return `ALREADY_EXISTS`.
- Input IDs must be valid/non-negative; otherwise `INVALID_INPUT`.

## Return Validations
- Member must exist; otherwise `NOT_FOUND`.
- Game must exist; otherwise `NOT_FOUND`.
- Member must currently have the game in their `BorrowedNode` list; otherwise `NOT_BORROWED`.

## Duplicate Borrow Rule
- A member may have at most one active borrow for a given game at a time.
- Additional borrow attempts for the same game while outstanding return `ALREADY_EXISTS`.

## Event Recording & Circular Queue Behavior
- Every successful borrow or return generates a `BorrowEvent`.
- Events are stored in a circular queue with **overwrite-oldest** behavior when full.
- When full, enqueue discards the oldest event, advances `front`, then writes the new event and returns `OK`.

## Summary Computation
- Member summary: iterate current `BorrowedNode` list for active borrows and scan `BorrowEvent` history for that member.
- Admin summary: aggregate counts across all `BorrowEvent` entries and current member borrow lists.
- Summaries are read-only views; they must not mutate queue state.

## Integration Notes (Phase 1 targets)
- Borrow/return will call Member A's **GameService**: `getGameById(gameId)`, and adjust `availableCopies` (+1 on return, -1 on borrow).
- Ratings are handled by Member C's **RatingService**; TransactionService will only call it if summaries need rating info (not planned for borrow/return core paths).

## Compile/Run snippet (Phase 0 skeleton)
```sh
# from repo root
# build (example using g++)
g++ DSA_Assg/DSA_Assg.cpp ds/*.cpp services/*.cpp -I./ -o dsa_app
./dsa_app
```
