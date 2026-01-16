#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include "../models/BorrowEvent.h"

class CircularQueue {
private:
    BorrowEvent* data;
    int capacity;
    int frontIndex;
    int rearIndex;
    int currentSize;

public:
    explicit CircularQueue(int maxSize = 100);
    ~CircularQueue();

    bool isEmpty() const;
    bool isFull() const;

    // Overwrite-oldest behavior: if full, oldest is discarded and replaced.
    Status enqueue(const BorrowEvent& event);
    Status dequeue(BorrowEvent& outEvent);
    Status peek(BorrowEvent& outEvent) const;
};

#endif // CIRCULAR_QUEUE_H
