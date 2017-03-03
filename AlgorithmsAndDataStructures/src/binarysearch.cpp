#include "binarysearch.h"

#include <iostream>
#include <vector>
#include <algorithm>

int recursive_rank(int key, std::vector<int> list, int lo, int hi)
{
    if (lo > hi)
        return -1;
    int mid = lo + (hi - lo) / 2;
    if (key < list[mid])
        return recursive_rank(key, list, lo, mid - 1);
    else if (key > list[mid])
        return recursive_rank(key, list, mid + 1, hi);
    else return mid;
}

int rank(int key, std::vector<int> list)
{
    int lo = 0;
    int hi = list.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (key < list[mid])
            hi = mid - 1;
        else if (key > list[mid])
            lo = mid + 1;
        else
            return mid;
    }

    return -1;
}

void binarysearch()
{
    std::vector<int> list =
        { 23, 50, 10, 99, 18, 23, 98, 84, 11, 10, 48, 77, 13, 54, 98, 77, 77, 68 };

    std::sort(list.begin(), list.end());

    int key = 99;
    for(auto el : list)
        std::cout << el << " ";
    std::cout << std::endl;
    std::cout << rank(key, list) << std::endl;
    std::cout << recursive_rank(key, list, 0, list.size() -1) << std::endl;
}
