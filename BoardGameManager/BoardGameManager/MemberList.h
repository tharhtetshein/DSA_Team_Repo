#ifndef DS_MEMBERLIST_H
#define DS_MEMBERLIST_H

#include "Member.h"

// Member B - Phase 0
// Doubly Linked List implementation for Member records (no STL).

struct MemberNode {
    Member data;
    MemberNode* prev;
    MemberNode* next;

    MemberNode(const Member& m) : data(m), prev(nullptr), next(nullptr) {}
};

class MemberList {
private:
    MemberNode* head;
    MemberNode* tail;
    int size;

public:
    MemberList();
    ~MemberList();

    // Disable copy to avoid accidental double free (Phase 0 safety)
    MemberList(const MemberList&) = delete;
    MemberList& operator=(const MemberList&) = delete;

    int getSize() const;

    // Adds member to tail. Returns false if memberId already exists.
    bool addMember(const Member& m);

    // Returns pointer to member data if found; otherwise nullptr.
    Member* findMember(int memberId);

    // Iteration support (for summaries later)
    MemberNode* getHead();
};

#endif
