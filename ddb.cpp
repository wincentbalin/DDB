/**
    ddb.cpp

    Main file.

    Part of Disc Data Base.

    Copyright (c) 2010 Wincent Balin

    Based upon ddb.pl, created years before and serving faithfully until today.

    Uses SQLite database version 3.

    Published under MIT license. See LICENSE file for further information.
*/

#include "ddb.hpp"

#include <iostream>

#include <cstdlib>
#include <cstring>

#include <getopt.h>


using namespace std;


ddb::ddb(int argc, char** argv) :
    db_filename(DATABASE_NAME), do_initialize(false),
    do_add(false), do_list(false), do_remove(false),
    directories_only(false), verbosity(0)
{


    // Parse command line arguments
    // Getopt variables
    int ch, option_index;
    static struct option long_options[] =
    {
        {"add",          required_argument, 0, 'a'},
        {"directory",    no_argument,       0, 'd'},
        {"file",         required_argument, 0, 'f'},
        {"help",         no_argument,       0, 'h'},
        {"initialize",   no_argument,       0, 'i'},
        {"list",         optional_argument, 0, 'l'},
        {"quite",        no_argument,       0, 'q'},
        {"remove",       required_argument, 0, 'r'},
        {"verbose",      no_argument,       0, 'v'},
        { 0,             0,                 0,  0 }
    };

    // If no command line arguments given, print help and exit
    if(argc == 1)
    {
        print_help();
        exit(EXIT_FAILURE);
    }

    // Process command line arguments
    while(true)
    {
        ch = getopt_long(argc, argv, "a:df:hilqr:v", long_options, &option_index);

        if(ch == -1)
            break;

        switch(ch)
        {
            // Add disc
            case 'a':
                disc_name = optarg;
                break;

            // Directories only
            case 'd':
                directories_only = true;
                break;

            // Database File
            case 'f':
                db_filename = optarg;
                break;

            // Help
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;

            // Initialize
            case 'i':
                do_initialize = true;
                break;

            // List
            case 'l':
                do_list = true;
                break;

            // Quite
            case 'q':
                verbosity--;
                break;

            // Remove disc
            case 'r':
                do_remove = true;
                disc_name = optarg;
                break;

            // Verbosity
            case 'v':
                verbosity++;
                break;

            // Unknown options
            default:
                print_help();
                exit(EXIT_FAILURE);
                break;
        }
    }

    // Save last argument
    if(argv[argc-1][0] != '-')
    {
        argument = argv[argc-1];
    }
}

ddb::~ddb(void)
{
}

bool
ddb::run(void)
{
    int result;
    bool success = true;

    // Open database
    msg(2, "Opening database...");
    result =
    sqlite3_open(db_filename.c_str(), &db);
    msg(3, "Done.");

    if(result != SQLITE_OK)
    {
        cerr << "Error while opening database " << db_filename << " : "
             << sqlite3_errmsg(db) << endl;
        exit(EXIT_FAILURE);
    }

    // Check whether the database has table ddb
    if(!do_initialize && !is_discdb())
    {
        cerr << "Wrong database " << db_filename << endl;
        sqlite3_close(db);
        return false;
    }

    // Choose functionality to run
    if(do_add)
    {
        success =
        add_disc();

        if(!success)
        {
            cerr << "Error while adding disc " << disc_name << endl;
        }
    }
    else if(do_remove)
    {
        success =
        remove_disc();

        if(!success)
        {
            cerr << "Error while removing disc " << disc_name << endl;
        }
    }
    else if(do_list)
    {
        success =
        list_contents();

        if(!success)
        {
            cerr << "Error while listing contents" << endl;
        }
    }
    else if(do_initialize)
    {
        success =
        initialize_database();

        if(!success)
        {
            cerr << "Error initializing database" << endl;
        }
    }
    else    // If nothing else specified, search text
    {
        search_text();
    }

    // Close database
    msg(2, "Closing the database...");
    sqlite3_close(db);
    msg(3, "Done.");

    return success;
}

bool
ddb::is_discdb(void)
{
    const char* check_discdb_table =
        "SELECT sql FROM "
        "(SELECT sql sql, type type, tbl_name tbl_name, name name FROM "
        "sqlite_master UNION ALL "
        "SELECT sql, type, tbl_name, name FROM sqlite_temp_master) "
        "WHERE tbl_name LIKE '"TABLE_NAME"' AND type!='meta' AND sql NOTNULL "
        "ORDER BY substr(type,2,1), name";
    int result;
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, check_discdb_table, -1, &stmt, NULL);

    result =
    sqlite3_step(stmt);

    if(result != SQLITE_ROW || sqlite3_column_count(stmt) != 1)
    {
        if(verbosity >= 1)
        {
            cerr << "Error checking database: " << sqlite3_errmsg(db) << endl
                 << endl;
        }

        sqlite3_finalize(stmt);
        return false;
    }

    const char* schema = (const char*) sqlite3_column_text(stmt, 0);

    if(strncmp(schema, discdb_schema, strlen(discdb_schema)) != 0)
    {
        if(verbosity >= 1)
        {
            cerr << "Database has wrong schema!" << endl
                 << endl;
        }

        sqlite3_finalize(stmt);
        return false;
    }

    result =
    sqlite3_step(stmt);

    if(result != SQLITE_DONE)
    {
        if(verbosity >= 1)
        {
            cerr << "Something wrong with the database: "
                 << sqlite3_errmsg(db) << endl
                 << endl;
        }

        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool
ddb::is_disc_present(string& name)
{
    const char* disc_present =
        "SELECT DISTINCT disc FROM ddb WHERE disc LIKE ?";
    int result;
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, disc_present, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    result =
    sqlite3_step(stmt);

    // A row means that the disc is present
    if(result == SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

bool
ddb::add_disc(void)
{
    return false;
}

bool
ddb::remove_disc(void)
{
    return false;
}

bool
ddb::list_contents(void)
{
    if(directories_only)
    {
        return list_directories();
    }
    else if(argument.length() > 0)
    {
        return list_files();
    }
    else
    {
        return list_discs();
    }
}

bool
ddb::list_discs(void)
{
    const char* list_discs = "SELECT DISTINCT disc FROM ddb";
    int result;
    sqlite3_stmt* stmt;
    vector<string> discs;

    sqlite3_prepare_v2(db, list_discs, -1, &stmt, NULL);

    // Fetch results
    while(true)
    {
        result =
        sqlite3_step(stmt);

        if(result == SQLITE_ROW)
        {
            // Store results
            string s = (const char*) sqlite3_column_text(stmt, 0);
            discs.push_back(s);
        }
        else    // End or error
        {
            sqlite3_finalize(stmt);

            if(result == SQLITE_DONE)
            {
                break;
            }
            else
            {
                if(verbosity >= 1)
                {
                    cerr << "Error while listing contents!" << endl
                         << endl;
                }

                return false;
            }
        }
    }

    // Sort and print results
    sort(discs.begin(), discs.end());

    vector<string>::const_iterator i;
    for(i = discs.begin(); i != discs.end(); i++)
    {
        cout << *i << endl;
    }

    return true;
}

bool
ddb::list_directories(void)
{
    const char* list_dirs =
        "SELECT DISTINCT directory FROM ddb WHERE disc LIKE ?";
    int result;
    sqlite3_stmt* stmt;
    vector<string> directories;

    sqlite3_prepare_v2(db, list_dirs, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, argument.c_str(), -1, SQLITE_STATIC);

    // Fetch results
    while(true)
    {
        result =
        sqlite3_step(stmt);

        if(result == SQLITE_ROW)
        {
            // Store results
            string s = (const char*) sqlite3_column_text(stmt, 0);
            directories.push_back(s);
        }
        else    // End or error
        {
            sqlite3_finalize(stmt);

            if(result == SQLITE_DONE)
            {
                break;
            }
            else
            {
                if(verbosity >= 1)
                {
                    cerr << "Error while listing contents!" << endl
                         << endl;
                }

                return false;
            }
        }
    }

    // Sort and print results
    sort(directories.begin(), directories.end());

    vector<string>::const_iterator i;
    for(i = directories.begin(); i != directories.end(); i++)
    {
        cout << *i << endl;
    }

    return true;
}

bool
ddb::list_files(void)
{
    const char* list_files =
        "SELECT directory,file FROM ddb WHERE disc LIKE ?";
    int result;
    sqlite3_stmt* stmt;
    vector<string> files;

    sqlite3_prepare_v2(db, list_files, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, argument.c_str(), -1, SQLITE_STATIC);

    // Fetch results
    while(true)
    {
        result =
        sqlite3_step(stmt);

        if(result == SQLITE_ROW)
        {
            // Store results
            string path;
            path.append((const char*) sqlite3_column_text(stmt, 0));
            path.append((const char*) sqlite3_column_text(stmt, 1));
            files.push_back(path);
        }
        else    // End or error
        {
            sqlite3_finalize(stmt);

            if(result == SQLITE_DONE)
            {
                break;
            }
            else
            {
                if(verbosity >= 1)
                {
                    cerr << "Error while listing contents!" << endl
                         << endl;
                }

                return false;
            }
        }
    }

    // Sort and print results
    sort(files.begin(), files.end());

    vector<string>::const_iterator i;
    for(i = files.begin(); i != files.end(); i++)
    {
        cout << *i << endl;
    }

    return true;
}

bool
ddb::initialize_database(void)
{
    // Create the table
    int result =
    sqlite3_exec(db, discdb_schema, NULL, NULL, NULL);

    if(result != SQLITE_OK)
    {
        if(verbosity >= 1)
        {
            cerr << "Error creating table!" << endl
                 << endl;
        }

        return false;
    }

    // Check the table again
    if(!is_discdb())
    {
        if(verbosity >= 1)
        {
            cerr << "Table was not created!" << endl
                 << endl;
        }

        return false;
    }

    return true;
}

bool
ddb::search_text(void)
{
    const char* search = directories_only ?
        "SELECT disc,directory,file FROM ddb WHERE directory LIKE ?" :
        "SELECT disc,directory,file FROM ddb WHERE file LIKE ?";
    int result;
    sqlite3_stmt* stmt;
    vector<pair<string, string> > files;

    sqlite3_prepare_v2(db, search, -1, &stmt, NULL);

    // Create query with wildcards
    string wildcard;
    wildcard.append("%");
    wildcard.append(argument);
    wildcard.append("%");

    // Bind the query
    sqlite3_bind_text(stmt, 1, wildcard.c_str(), -1, SQLITE_STATIC);

    // Fetch results
    while(true)
    {
        result =
        sqlite3_step(stmt);

        if(result == SQLITE_ROW)
        {
            // Store results
            string disc;
            disc.append((const char*) sqlite3_column_text(stmt, 0));
            string path;
            path.append((const char*) sqlite3_column_text(stmt, 1));
            path.append("/");
            path.append((const char*) sqlite3_column_text(stmt, 2));

            files.push_back(make_pair(disc, path));
        }
        else    // End or error
        {
            sqlite3_finalize(stmt);

            if(result == SQLITE_DONE)
            {
                break;
            }
            else
            {
                if(verbosity >= 1)
                {
                    cerr << "Error while listing contents!" << endl
                         << endl;
                }

                return false;
            }
        }
    }

    // Sort and print results
    sort(files.begin(), files.end());

    vector<pair<string, string> >::const_iterator i;
    for(i = files.begin(); i != files.end(); i++)
    {
        cout << (*i).first << ":\t" << (*i).second << endl;
    }

    return true;
}

void
ddb::print_help(void)
{
    cerr << "Disc Data Base" << endl
         << endl
         << "ddb [options] [file ...]" << endl
         << endl
         << "  Options:" << endl
         << "  No options                        Search file(s)" << endl
         << "  -a, --add title disc_directory    Add disc to the database" << endl
         << "  -d, --directory                   Directories only" << endl
         << "  -r, --remove title                Remove disc from database" << endl
         << "  -l, --list                        List the given disc or directory" << endl
         << "  -h, --help                        Print this help message" << endl
         << "  -v, --verbose                     Increase verbosity" << endl
         << "  -q, --quiet                       Decrease verbosity" << endl
         << "  -f, --file                        Use another database file" << endl
         << "  -i, --initialize                  Create new database" << endl;
}

void
ddb::msg(int min_verbosity, char* message)
{
    if(verbosity >= min_verbosity)
    {
        cerr << message << endl;
    }
}


int main(int argc, char** argv)
{
    ddb ddb(argc, argv);

    return ddb.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}
