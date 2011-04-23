/**
 *  db.hpp
 *
 *  Database abstraction include part of Disc Data Base.
 *
 *  Copyright (c) 2010-2011 Wincent Balin
 *
 *  Based upon ddb.pl, created years before and serving faithfully until today.
 *
 *  Uses SQLite database version 3.
 *
 *  Published under MIT license. See LICENSE file for further information.
 */

#ifndef DB_HPP
#define DB_HPP

class DB
{
public:
    DB();
    virtual ~DB();
};

#endif /* DB_HPP */
