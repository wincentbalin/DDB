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

#include <algorithm>

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

enum Print::Verbosity
Print::get_verbosity(void)
{
    return specified_verbosity;
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
Print::add_disc(const char* disc_name)
{
    results.push_back(std::string(disc_name));
}

void
Print::add_directory(const char* disc_name, const char* directory)
{
    std::ostringstream line;

    // Create an output line
    line << disc_name << ':' << '\t' << directory << std::endl;

    // Push line to results
    results.push_back(line.str());
}

void
Print::add_file(const char* disc_name, const char* directory, const char* file)
{
    std::ostringstream line;

    // Create an output line
    line << disc_name << ':' << '\t' << directory << '/' << file << std::endl;

    // Push line to results
    results.push_back(line.str());
}

void
Print::output(void)
{
    // Sort results
    std::sort(results.begin(), results.end());

    // First, print one empty line to separate the following output
    std::cout << std::endl;

    // Second print all results
    foreach(std::string line, results)
        std::cout << line << std::endl;

    // Third, separate output from the following prompt
    std::cout << std::endl;
}
