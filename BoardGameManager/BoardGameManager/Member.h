#ifndef MODELS_MEMBER_H
#define MODELS_MEMBER_H

// Member B - Phase 0
// No STL containers used for DS/ops.

struct BorrowedNode {
    int gameId;
    BorrowedNode* next;
};

struct Member {
    int memberId;
    char name[60];

    // Singly linked list of currently borrowed games (gameId nodes)
    BorrowedNode* borrowedHead;

    // Optional stats for additional features
    int totalPlays;
    int totalWins;

    Member()
        : memberId(0), borrowedHead(nullptr), totalPlays(0), totalWins(0) {
        name[0] = '\0';
    }
};

#endif
