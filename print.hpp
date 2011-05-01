/**
 *  print.hpp
 *
 *  Printing include  part of Disc Data Base.
 *
 *  Copyright (c) 2010-2011 Wincent Balin
 *
 *  Based upon ddb.pl, created years before and serving faithfully until today.
 *
 *  Uses SQLite database version 3.
 *
 *  Published under MIT license. See LICENSE file for further information.
 */

#ifndef PRINT_HPP
#define PRINT_HPP

#include <string>
#include <vector>

class Print
{
public:
    enum Verbosity
    {
        CRITICAL = 0,
        INFO = 1,
        VERBOSE = 2,
        DEBUG = 3,
        VERBOSE_DEBUG = 4
    };
    //
    Print();
    virtual ~Print();
private:
    enum Verbosity verbosity;
};

#endif /* PRINT_HPP */
