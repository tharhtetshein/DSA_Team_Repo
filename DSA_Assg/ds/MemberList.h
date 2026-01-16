#ifndef MEMBER_LIST_H
#define MEMBER_LIST_H

#include "../models/Member.h"

struct MemberNode {
    Member data;
    MemberNode* prev;
    MemberNode* next;

    MemberNode() : prev(0), next(0) {}
};

class MemberList {
private:
    MemberNode* head;
    MemberNode* tail;
    int count;

public:
    MemberList();
    ~MemberList();

    Status add(const Member& member);
    Member* findById(int memberId);

    MemberNode* getHead();
    int size() const;
};

#endif // MEMBER_LIST_H
