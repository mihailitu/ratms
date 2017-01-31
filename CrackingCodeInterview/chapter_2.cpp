#include "chapter_2.h"

#include <iostream>

template<class T>
class LinkedList {
    struct Node {
        Node *next;
        T data;
    };

    Node *root;

public:
    LinkedList() {

    }

    void insert(T data) {

    }

    void remove(T data) {

    }
};

//class Node {
//    Node *next = nullptr;
//    int data;

//public:
//    Node( int d ) : data( d ) {}

//    void appendToTail( int d ) {
//        Node *end = new Node(d);
//        Node *n = this;
//        while( n->next != nullptr )
//            n = n->next;

//        n->next = end;
//    }

//    Node* deleteNode(Node *head, int d) {
//        Node *n = head;

//        if (n->data == d)
//            return head->next;

//        while (n->next != nullptr) {
//            if (n->next->data == d) {
//                n->next = n->next->next;
//                return head;
//            }
//            n = n->next;
//        }
//        return head;
//    }
//};

void run_chapter_2()
{
    LinkedList<int> ll;

    ll.insert(5);
}
