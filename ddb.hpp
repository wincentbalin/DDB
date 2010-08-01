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

// Name of the database
#define DATABASE_NAME "discdb"

// Name of the table
#define TABLE_NAME "ddb"



class DDB
{
public:
    DDB(int argc, char** argv);
    ~DDB(void);
    bool run(void);
    // Constants
    const static char* discdb_schema;
private:
    bool is_discdb(void);
    bool is_disc_present(string& name);
    bool is_directory(string& filename);
    inline bool add_disc(void);
    inline bool remove_disc(void);
    inline bool list_contents(void);
    inline bool list_discs(void);
    inline bool list_directories(void);
    inline bool list_files(void);
    inline bool initialize_database(void);
    inline bool search_text(void);
    void print_help(void);
    void msg(int min_verbosity, const char* message);
    // Database handle
    sqlite3* db;
    // Configuration flags
    string db_filename;
    string disc_name;
    string argument;
    bool do_initialize;
    bool do_add;
    bool do_list;
    bool do_remove;
    bool directories_only;
    int verbosity;
};


// discdb schema
const char* DDB::discdb_schema =
    "CREATE TABLE "TABLE_NAME" "
    "(directory TEXT NOT NULL, file TEXT, disc TEXT NOT NULL)";



#endif /* DDB_HPP */
