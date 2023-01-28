#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Public static functions for printing with colors.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios
CREATION:	15/12/2021
EDITED:		17/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "colors.h"

// Print class
class Print {

    public:

    /// @brief      Master function to print some text to the screen
    /// @param      text - The text to be displayed
    /// @param      color - The color to print (from the colors)
    /// @param      tabs - The number of tabs to print before message
    /// @param      new_line - A flag for including a newline
    static void print (const char* text, const char* color, int tabs, bool new_line);

    /// @brief      Prints some text to the screen
    /// @param      text - The text to be displayed
    static void print (const char* text);

    /// @brief      Prints some text to the screen
    /// @param      text - The text to be displayed
    /// @param      color - The color to print (from the colors)
    static void print (const char* text, const char* color);

    /// @brief      Prints some text to the screen
    /// @param      text - The text to be displayed
    /// @param      tabs - The number of tabs to print before message
    static void print (const char* text, int tabs);

    /// @brief      Prints some text to the screen
    /// @param      text - The text to be displayed
    /// @param      new_line - A flag for including a newline
    static void print (const char* text, bool new_line);

    /// @brief      Prints some text to the screen
    /// @param      text - The text to be displayed
    /// @param      color - The color to print (from the colors)
    /// @param      tabs - The number of tabs to print before message
    static void print (const char* text, const char* color, int tabs);

    /// @brief      Prints some text to the screen
    /// @param      text - The text to be displayed
    /// @param      color - The color to print (from the colors)
    /// @param      new_line - A flag for including a newline
    static void print (const char* text, const char* color, bool new_line);

    /// @brief      Prints a title text to the screen
    /// @param      text - The text to be displayed
    static void title (const char* text);

};