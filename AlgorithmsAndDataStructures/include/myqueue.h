#ifndef MY_QUEUE_H
#define MY_QUEUE_H

template<typename T>
class myqueue
{
    int pos;
    int sz;
public:
    myqueue() {
    }

    void enqueue(T item) {
    }

    T dequeue() {
        return T();
    }

    bool isEmpty() const {
        return pos;
    }

    int size() const {
        return sz;
    }
};

void testMyQueue();

#endif // MY_QUEUE_H
