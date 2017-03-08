#ifndef MYSTACK_H
#define MYSTACK_H

template<typename T>
class mystack
{
    static int const capacity = 1024;
    unsigned pos;
    T *a;

    void resize() {
        T *temp = new T[pos + capacity];
        for(unsigned i = 0; i < pos; ++i)
            temp[i] = a[i];
        delete[] a;
        a = temp;
    }

public:
    mystack() {
        a = new T[capacity];
    }

    ~mystack() {
        delete[] a;
    }

    void push(T item) {
        if (pos == capacity)
            resize();
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
