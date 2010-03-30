/**
    ddb.hpp

    Main include file.

    Part of Disc Data Base.

    Copyright (c) 2010 Wincent Balin

    Based upon ddb.pl, created years before and serving faithfully until today.

    Uses SQLite database version 3.

    Published under MIT license. See LICENSE file for further information.
*/

#ifndef DDB_HPP
#define DDB_HPP

#include <string>

#include <sqlite3.h>


using namespace std;


class ddb
{
public:
    ddb(int argc, char** argv);
    ~ddb(void);
    void run(void);
private:
    void print_help(void);
    void msg(int min_verbosity, char* message);
    // Database handle
    sqlite3* db;
    // Configuration flags
    string db_filename;
    string disc_name;
    string adding_filename;
    bool do_initialize;
    bool do_add;
    bool do_list;
    bool do_remove;
    bool directories_only;
    int verbosity;
};

#endif /* DDB_HPP */
