#include "mystack.h"
#include <iostream>

void testMyStack()
{
    mystack<int> i;
    i.push(10);
    i.push(20);

    std::cout << i.pop() << std::endl;
    std::cout << i.pop() << std::endl;
}
