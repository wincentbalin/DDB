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


Print::Print(enum Verbosity verbosity)
{
    // Store specified verbosity
    specified_verbosity = verbosity;
}

Print::~Print()
{
    // TODO Auto-generated destructor stub
}
