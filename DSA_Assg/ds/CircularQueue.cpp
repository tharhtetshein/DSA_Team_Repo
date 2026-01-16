#include "CircularQueue.h"

CircularQueue::CircularQueue(int maxSize)
    : data(0), capacity(maxSize), frontIndex(0), rearIndex(-1), currentSize(0) {
    if (capacity < 1) {
        capacity = 1;
    }
    data = new BorrowEvent[capacity];
}

CircularQueue::~CircularQueue() {
    delete[] data;
}

bool CircularQueue::isEmpty() const {
    return currentSize == 0;
}

bool CircularQueue::isFull() const {
    return currentSize == capacity;
}

Status CircularQueue::enqueue(const BorrowEvent& event) {
    if (capacity <= 0) {
        return INVALID_INPUT;
    }

    if (isFull()) {
        // overwrite oldest: advance front to discard
        frontIndex = (frontIndex + 1) % capacity;
        currentSize--; // will be incremented below
    }

    rearIndex = (rearIndex + 1) % capacity;
    data[rearIndex] = event;
    if (currentSize < capacity) {
        currentSize++;
    }

    return OK;
}

Status CircularQueue::dequeue(BorrowEvent& outEvent) {
    if (isEmpty()) {
        return NOT_FOUND;
    }

    outEvent = data[frontIndex];
    frontIndex = (frontIndex + 1) % capacity;
    currentSize--;
    return OK;
}

Status CircularQueue::peek(BorrowEvent& outEvent) const {
    if (isEmpty()) {
        return NOT_FOUND;
    }

    outEvent = data[frontIndex];
    return OK;
}
