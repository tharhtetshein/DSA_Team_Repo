#include "CircularQueue.h"

CircularQueue::CircularQueue(int cap)
    : buffer(nullptr), capacity(cap), head(0), tail(0), count(0) {
    if (capacity < 1) capacity = 1;
    buffer = new BorrowEvent[capacity];
}

CircularQueue::~CircularQueue() {
    delete[] buffer;
    buffer = nullptr;
    capacity = 0;
    head = tail = count = 0;
}

bool CircularQueue::isEmpty() const {
    return count == 0;
}

bool CircularQueue::isFull() const {
    return count == capacity;
}

int CircularQueue::getCount() const {
    return count;
}

int CircularQueue::getCapacity() const {
    return capacity;
}

void CircularQueue::enqueue(const BorrowEvent& e) {
    // Ray Feature DS: Append borrow/return events with fixed-capacity overwrite.
    // If full, overwrite oldest: advance head by 1 (drop oldest)
    if (isFull()) {
        head = (head + 1) % capacity;
        count--; // we'll increment again below
    }

    buffer[tail] = e;
    tail = (tail + 1) % capacity;
    count++;
}

bool CircularQueue::dequeue(BorrowEvent& out) {
    if (isEmpty()) return false;

    out = buffer[head];
    head = (head + 1) % capacity;
    count--;
    return true;
}

bool CircularQueue::peek(BorrowEvent& out) const {
    if (isEmpty()) return false;
    out = buffer[head];
    return true;
}

bool CircularQueue::getAt(int i, BorrowEvent& out) const {
    // Ray Feature DS: Read event by logical order for summary/report screens.
    if (i < 0 || i >= count) return false;
    int idx = (head + i) % capacity;
    out = buffer[idx];
    return true;
}

void CircularQueue::clear() {
    head = 0;
    tail = 0;
    count = 0;
}
