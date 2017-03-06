#ifndef MYSTACK_H
#define MYSTACK_H

template<typename T>
class mystack
{
    static int const capacity = 1024;
    int pos;
    T *a;
public:
    mystack() {
        a = new T[capacity];
    }

    ~mystack() {
        delete[] a;
    }

    void push(T item) {
        a[pos++] = item;
    }

    T pop() {
        return a[--pos];
    }

    bool isEmpty() const {
        return pos == 0;
    }

    int size() const {
        return pos;
    }

};

void testMyStack();

#endif // MYSTACK_H
