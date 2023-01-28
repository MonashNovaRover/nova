/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):  Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the printing code
#include "print.h"
#include <stdlib.h>
#include <iostream>

using namespace std;


void Print::print (const char* text, const char* color, int tabs, bool new_line) {
    for (int i = 0; i < tabs; i++)
        cout << "    ";
    cout << color << text << C_END;
    if (new_line)
        cout << endl;
    fflush(stdout);
}

void Print::print (const char* text) {
    print(text, C_END, 0, true);
}

void Print::print (const char* text, const char* color) {
    print(text, color, 0, true);
}

void Print::print (const char* text, int tabs) {
    print(text, C_END, tabs, true);
}

void Print::print (const char* text, bool new_line) {
    print(text, C_END, 0, new_line);
}

void Print::print (const char* text, const char* color, int tabs) {
    print(text, color, tabs, true);
}

void Print::print (const char* text, const char* color, bool new_line) {
    print(text, color, 0, new_line);
}

void Print::title (const char* text) {
    print("--------------------", true);
    print(text, C_TITLE);
    print("--------------------", true);
}