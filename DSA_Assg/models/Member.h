#ifndef MEMBER_H
#define MEMBER_H

#include "BorrowEvent.h"

struct BorrowedNode {
    int gameId;
    BorrowedNode* prev;
    BorrowedNode* next;

    BorrowedNode() : gameId(0), prev(0), next(0) {}
};

struct Member {
    int id;
    char name[64];
    BorrowedNode* borrowedHead; // doubly linked list of currently borrowed games
    int borrowedCount;

    Member() : id(0), borrowedHead(0), borrowedCount(0) {
        name[0] = '\0';
    }
};

#endif // MEMBER_H
