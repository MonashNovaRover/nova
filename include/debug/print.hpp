#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Public static functions for printing with colors.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios
CREATION:	15/12/2021
EDITED:		15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the printing module
#include <iostream>

// Include the colors module
#include "colors.hpp"

// Use the standard namespace
using namespace std;


/// @brief      Master function to print some text to the screen
/// @param      text - The text to be displayed
/// @param      color - The color to print (from the colors)
/// @param      tabs - The number of tabs to print before message
/// @param      new_line - A flag for including a newline
void print (const char* text, const char* color, int tabs, bool new_line) {
    for (int i = 0; i < tabs; i++)
        cout << "\t";
    cout << color << text << C_END;
    if (new_line)
        cout << endl;
}

/// @brief      Prints some text to the screen
/// @param      text - The text to be displayed
void print (const char* text) {
    print(text, C_END, 0, true);
}

/// @brief      Prints some text to the screen
/// @param      text - The text to be displayed
/// @param      color - The color to print (from the colors)
void print (const char* text, const char* color) {
    print(text, color, 0, true);
}

/// @brief      Prints some text to the screen
/// @param      text - The text to be displayed
/// @param      tabs - The number of tabs to print before message
void print (const char* text, int tabs) {
    print(text, C_END, tabs, true);
}

/// @brief      Prints some text to the screen
/// @param      text - The text to be displayed
/// @param      new_line - A flag for including a newline
void print (const char* text, bool new_line) {
    print(text, C_END, 0, new_line);
}

/// @brief      Prints some text to the screen
/// @param      text - The text to be displayed
/// @param      color - The color to print (from the colors)
/// @param      tabs - The number of tabs to print before message
void print (const char* text, const char* color, int tabs) {
    print(text, color, tabs, true);
}

/// @brief      Prints some text to the screen
/// @param      text - The text to be displayed
/// @param      color - The color to print (from the colors)
/// @param      new_line - A flag for including a newline
void print (const char* text, const char* color, bool new_line) {
    print(text, color, 0, new_line);
}

/// @brief      Prints a title text to the screen
/// @param      text - The text to be displayed
void title (const char* text) {
    print("--------------------", true);
    print(text, C_TITLE);
    print("--------------------", true);
}

