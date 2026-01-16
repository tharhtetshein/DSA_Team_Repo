#include "MemberList.h"
#include <cstring>

MemberList::MemberList() : head(0), tail(0), count(0) {}

MemberList::~MemberList() {
    MemberNode* current = head;
    while (current) {
        MemberNode* toDelete = current;
        current = current->next;
        delete toDelete;
    }
}

Status MemberList::add(const Member& member) {
    // TODO: Phase 1 - enforce unique IDs
    MemberNode* node = new MemberNode();
    node->data = member;

    if (!head) {
        head = tail = node;
    } else {
        tail->next = node;
        node->prev = tail;
        tail = node;
    }

    count++;
    return OK;
}

Member* MemberList::findById(int memberId) {
    MemberNode* current = head;
    while (current) {
        if (current->data.id == memberId) {
            return &current->data;
        }
        current = current->next;
    }
    return 0;
}

MemberNode* MemberList::getHead() {
    return head;
}

int MemberList::size() const {
    return count;
}
