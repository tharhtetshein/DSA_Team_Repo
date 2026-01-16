#ifndef DS_CIRCULARQUEUE_H
#define DS_CIRCULARQUEUE_H

#include "BorrowEvent.h"

// Member B - Phase 0
// Array-based circular queue for BorrowEvent.
// Queue-full policy: overwrite oldest event.

class CircularQueue {
private:
    BorrowEvent* buffer;
    int capacity;
    int head;   // index of oldest element
    int tail;   // index of next insertion point
    int count;  // number of elements in queue

public:
    CircularQueue(int cap = 200);
    ~CircularQueue();

    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;

    bool isEmpty() const;
    bool isFull() const;
    int getCount() const;
    int getCapacity() const;

    // Enqueue with overwrite-oldest behavior if full.
    void enqueue(const BorrowEvent& e);

    // Dequeue oldest. Returns false if empty.
    bool dequeue(BorrowEvent& out);

    // Peek oldest. Returns false if empty.
    bool peek(BorrowEvent& out) const;

    // Read-only traversal helper: copy event at logical index i (0..count-1)
    // Returns false if i out of range.
    bool getAt(int i, BorrowEvent& out) const;

    void clear();
};

#endif
