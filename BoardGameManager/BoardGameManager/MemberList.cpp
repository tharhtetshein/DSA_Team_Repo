#include "MemberList.h"

MemberList::MemberList() : head(nullptr), tail(nullptr), size(0) {}

MemberList::~MemberList() {
    MemberNode* cur = head;
    while (cur != nullptr) {
        // Free borrowed list nodes for each member (owned by member)
        BorrowedNode* b = cur->data.borrowedHead;
        while (b != nullptr) {
            BorrowedNode* nextB = b->next;
            delete b;
            b = nextB;
        }

        MemberNode* next = cur->next;
        delete cur;
        cur = next;
    }
    head = nullptr;
    tail = nullptr;
    size = 0;
}

int MemberList::getSize() const {
    return size;
}

bool MemberList::addMember(const Member& m) {
    // Reject duplicates by memberId
    if (findMember(m.memberId) != nullptr) {
        return false;
    }

    MemberNode* node = new MemberNode(m);

    if (tail == nullptr) {
        head = tail = node;
    }
    else {
        tail->next = node;
        node->prev = tail;
        tail = node;
    }

    size++;
    return true;
}

Member* MemberList::findMember(int memberId) {
    MemberNode* cur = head;
    while (cur != nullptr) {
        if (cur->data.memberId == memberId) {
            return &(cur->data);
        }
        cur = cur->next;
    }
    return nullptr;
}

MemberNode* MemberList::getHead() {
    return head;
}
