#include "chapter_1.h"

#include <iostream>
#include <algorithm>
#include <cstring>

using namespace std;

/* 1.1 Implement an algorithm to determine if a string has all unique characters.
 * What if you cannot use additional data structures? */
bool _1_1_hasUniqueChars(std::string str)
{
    std::cout << __FUNCTION__ << std::endl;

    bool hasChar[256];

    for(unsigned i = 0; i < str.length(); ++i) {
        int val = str[i];
        if ( hasChar[val] )
            return false;
        hasChar[val] = true;
    }

    return true;
}

/* 1.2 Implement a function void reverse(char* str) in C or C++ which reverses a null-terminated string. */
void _1_2_reverseChar(char *str)
{
    std::cout << __FUNCTION__ << std::endl;

    unsigned len = strlen(str);
    for(unsigned i = 0; i < len / 2; ++i) {
        char temp = str[i];
        str[i] = str[len-i - 1];
        str[len-i - 1] = temp;
        // std::swap(&str[i], &str[len-i]);
    }
}

/* 1.3 Given two strings, write a method to decide if one is a permutation of the other. */
bool _1_3_isPermut(string str1, string str2)
{
    std::cout << __FUNCTION__ << std::endl;

    if ( str1.length() != str2.length() )
        return false;

    std::sort(str1.begin(), str1.end());
    std::sort(str2.begin(), str2.end());
    return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end());
}

/* 1.4 Write a method to replace all spaces in a string with'%20'. You may assume that
the string has sufficient space at the end of the string to hold the additional
characters, and that you are given the "true" length of the string. (Note: if imple-
menting in Java, please use a character array so that you can perform this opera-
tion in place.) */

std::string _1_4_replaceSpaces(std::string str) {
    std::cout << __FUNCTION__ << std::endl;
    std::string result;
    for(unsigned i = 0; i < str.length(); ++i)
        if ( str[i] == ' ' )
            result += "%20";
        else
            result += str[i];
    return result;
}

/* 1.5 Implement a method to perform basic string compression using the counts
of repeated characters. For example, the string aabcccccaaa would become
a2blc5a3. If the "compressed" string would not become smaller than the orig-
inal string, your method should return the original string. */
std::string _1_5_basicCompression(std::string str) {
    std::cout << __FUNCTION__ << std::endl;
    std::string compressed;

    int count = 0;
    char last;
    for( unsigned i = 0; i < str.length(); ++i ) {
        if ( count == 0 ) {
            last = str[i];
            ++count;
        } else
            if ( last == str[i] )
                ++count;
            else {
                compressed += last;
                compressed += std::to_string(count);
                count = 1;
                last = str[i];
            }

        if ( i == str.length() - 1 ) {
            compressed += last;
            compressed += std::to_string(count);
            break;
        }
    }

    if ( str.length() < compressed.length() )
        return str;
    else
        return compressed;
}

/* 1.6 Given an image represented by an NxN matrix, where each pixel in the image is
4 bytes, write a method to rotate the image by 90 degrees. Can you do this in
place? */

/* 1.7 Write an algorithm such that if an element in an MxN matrix is 0, its entire row
and column are set to 0. */
void _1_7_matrixToZero(std::vector<std::vector<int>> &matrix)
{
    std::cout << __FUNCTION__ << std::endl;

    std::vector<std::vector<bool>> zeroForced(matrix.size(), vector<bool>(matrix[0].size(), false));

    for(unsigned i = 0; i < matrix.size(); ++i)
        for(unsigned j = 0; j < matrix[i].size(); ++j)
            if ( matrix[i][j] == 0 && !zeroForced[i][j]) {
                for(unsigned row = 0; row < matrix.size();++row) {
                    if(matrix[row][j] != 0)
                        zeroForced[row][j] = true;
                    matrix[row][j] = 0;
                }
                for(unsigned col = 0; col < matrix[i].size(); ++col) {
                    if ( matrix[i][col] != 0)
                        zeroForced[i][col] = true;
                    matrix[i][col] = 0;
                }
            }
}

void run_chapter_1()
{
    do { // 1.1
        break;
        cout << _1_1_hasUniqueChars("abcdef") << endl;
        cout << _1_1_hasUniqueChars("aabcd") << endl;
    } while(0);

    do { // 1.2
        break;
        char ss[] = "abcde";
        _1_2_reverseChar(ss);
        cout << ss << endl;
    } while( 0 );

    do { // 1.3
        break;
        std::string s1 = "abcdefg";
        std::string s2 = "acbedgf";
        std::cout << _1_3_isPermut(s1, s2) << std::endl;

        s1 = "abcdefg";
        s2 = "acbedg";
        std::cout << _1_3_isPermut(s1, s2) << std::endl;

        s1 = "abcdefg";
        s2 = "acbadgf";
        std::cout << _1_3_isPermut(s1, s2) << std::endl;
    } while( 0 );

    do { // 1.4
        break;
        std::cout << _1_4_replaceSpaces("Mr. John Smith") << std::endl;
    } while (0 );

    do { // 1.5
        break;
        std::string tocompress = "aaabbcccccd";
        std::cout << tocompress << " -> " <<_1_5_basicCompression(tocompress) << std::endl;

        tocompress = "aaabbcccccdd";
        std::cout << tocompress << " -> " <<_1_5_basicCompression(tocompress) << std::endl;

        tocompress = "aaabcccd";
        std::cout << tocompress << " -> " <<_1_5_basicCompression(tocompress) << std::endl;
    } while (0 );

    do { // 1.7
        // break;
        std::vector<std::vector<int>> mat = {
            {1, 0, 1, 1},
            {0, 0, 1, 1},
            {1, 1, 1, 1},
            {1, 1, 1, 1},
            {1, 0, 1, 1} };
        for(unsigned i = 0; i < mat.size(); ++i) {
            for(unsigned j = 0; j < mat[i].size(); ++j)
                std::cout << mat[i][j] << ' ';
            std::cout << std::endl;
        }

        _1_7_matrixToZero(mat);

        for(unsigned i = 0; i < mat.size(); ++i) {
            for(unsigned j = 0; j < mat[i].size(); ++j)
                std::cout << mat[i][j] << ' ';
            std::cout << std::endl;
        }
    } while(0);
}
