/**
 *  print.cpp
 *
 *  Printing part of Disc Data Base.
 *
 *  Copyright (c) 2010-2011 Wincent Balin
 *
 *  Based upon ddb.pl, created years before and serving faithfully until today.
 *
 *  Uses SQLite database version 3.
 *
 *  Published under MIT license. See LICENSE file for further information.
 */

#include "print.hpp"

#include <iostream>
#include <sstream>

#include <boost/foreach.hpp>

// Use shortcut from example
#define foreach BOOST_FOREACH


Print::Print(enum Verbosity verbosity)
{
    // Store specified verbosity
    specified_verbosity = verbosity;
}

Print::~Print()
{
}

void
Print::msg(std::string& s, enum Verbosity message_verbosity)
{
    // Print message only if its verbosity is at least as severe as the one specified
    if(message_verbosity <= specified_verbosity)
    {
        std::cout << s << std::endl;
    }
}

void
Print::output(void)
{
    // First, print one empty line to separate the following output
    std::cout << std::endl;

    // Second print all results
    foreach(std::string line, results)
        std::cout << line << std::endl;

    // Third, separate output from the following prompt
    std::cout << std::endl;
}
