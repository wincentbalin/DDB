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

#include <iostream>
#include <string>
#include <vector>

#include "sqlite3.h"

#include "error.hpp"
#include "print.hpp"

class DB
{
public:
    DB(Print* print);
    virtual ~DB(void) throw(DBError);
    void open(const char* dbname, bool initialize = false) throw(DBError);
    void close(void) throw(DBError);
    bool has_correct_format(void) throw(DBError);
    bool is_disc_present(const char* disc_name) throw(DBError);
    void add_disc(const char* disc_name, const char* starting_directory) throw(DBError);
    void remove_disc(const char* disc_name) throw(DBError);
    void list_files(const char* disc_name, bool directories_only = false) throw(DBError);
    void list_discs(void) throw(DBError);
    void list_contents(const char* disc_name) throw(DBError);
    void search_text(void) throw(DBError);
private:
    void init(void);
private:
    // Printer
    Print* p;
    // Database handle
    sqlite3* db;
    // Database version
    int version;
    // Database creation SQL statements
    std::vector<std::string> format;
};

#endif /* DB_HPP */
