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
#include <vector>
#include <algorithm>
#include <stack>

#include <cstdlib>
#include <cstring>
#include <cctype>

#include <dirent.h>
#include <sys/stat.h>

#include <getopt.h>


using namespace std;


DDB::DDB(int argc, char** argv) :
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
                do_add = true;
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
                if(optarg)
                {
                    argument = optarg;
                }
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

DDB::~DDB(void)
{
}

void
DDB::run(void) throw (Exception)
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
        std::string msg = "Error while opening database " +
                          db_filename + " : " + sqlite3_errmsg(db);
        throw Exception(msg);
    }

    // Check whether the database has table ddb
    if(!do_initialize && !is_discdb())
    {
        sqlite3_close(db);

        std::string msg = "Wrong database " + db_filename;
        throw Exception(msg);
    }

    // Choose functionality to run
    if(do_add)
    {
        success =
        add_disc();

        if(!success && verbosity >= 1)
        {
            std::string msg = "Error while adding disc " + disc_name;
            throw Exception(msg);
        }
    }
    else if(do_remove)
    {
        success =
        remove_disc();

        if(!success && verbosity >= 1)
        {
            std::string msg = "Error while removing disc " + disc_name;
            throw Exception(msg);
        }
    }
    else if(do_list)
    {
        success =
        list_contents();

        if(!success && verbosity >= 1)
        {
            throw Exception("Error while listing contents");
        }
    }
    else if(do_initialize)
    {
        success =
        initialize_database();

        if(!success && verbosity >= 1)
        {
            throw Exception("Error initializing database");
        }
    }
    else    // If nothing else specified, search text
    {
        success =
        search_text();

        if(!success && verbosity >= 1)
        {
            throw Exception("Error searching");
        }
    }

    // Close database
    msg(2, "Closing the database...");
    sqlite3_close(db);
    msg(3, "Done.");
}

bool
DDB::is_discdb(void)
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
        std::string err_msg = "Error checking database: ";
                    err_msg += sqlite3_errmsg(db);
        msg(1, err_msg, 2);

        sqlite3_finalize(stmt);
        return false;
    }

    const char* schema = (const char*) sqlite3_column_text(stmt, 0);

    if(strncmp(schema, discdb_schema, strlen(discdb_schema)) != 0)
    {
        msg(1, "Database has wrong schema!", 2);

        sqlite3_finalize(stmt);
        return false;
    }

    result =
    sqlite3_step(stmt);

    if(result != SQLITE_DONE)
    {
        std::string err_msg = "Something wrong with the database: ";
                    err_msg += sqlite3_errmsg(db);
        msg(1, err_msg, 2);

        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool
DDB::is_disc_present(string& name)
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
DDB::is_directory(string& filename)
{
    struct stat dir_stat;

    stat(filename.c_str(), &dir_stat);
    return (S_ISDIR(dir_stat.st_mode)) ? true : false;   // ugly hack
}

bool
DDB::add_disc(void)
{
    const char* begin_transaction = "BEGIN";
    const char* add_entry =
        "INSERT INTO ddb (directory, file, disc) VALUES (?, ?, ?)";
    const char* end_transaction = "COMMIT";

    int result;
    char* error_message = NULL;

    // Check whether the disc is already in the database
    if(is_disc_present(disc_name))
    {
        std::string err_msg = "Disc " + disc_name + " already present in the database!";
        msg(0, err_msg, 2);

        return false;
    }

    // Check whether the given argument is a directory
    if(! is_directory(argument))
    {
        std::string err_msg = argument + " is not a directory!";
        msg(0, err_msg, 2);

        return false;
    }

    // Collect file names
    vector<pair<string, string> > filenames;
    string directory_filename = "";
    string current_directory = argument;
    string current_file;
    string current_absolute_path;
    stack <pair<string, DIR*> > directory_stack;
    struct dirent* entry;

    // Open the directory
open_directory:
    DIR* dir = opendir(current_directory.c_str());

    if(dir == NULL)
    {
        std::string err_msg = "Error opening directory " + argument + " !";
        msg(0, err_msg, 2);
        return false;
    }

    // Add directory to the filenames
    filenames.push_back(make_pair(current_directory, directory_filename));

    while(true)
    {
        entry = readdir(dir);

        // Check whether the directory ended
        if(entry == NULL)
        {
            // Close directory
            closedir(dir);

            // If stack is not empty, pop parent directory from it
            if(! directory_stack.empty())
            {
                pair<string, DIR*> directory_pair = directory_stack.top();
                directory_stack.pop();

                current_directory = directory_pair.first;
                dir = directory_pair.second;

                // Go to reading of the next entry of the directory
                continue;
            }
            else // End searching files
            {
                break;
            }
        }

        current_file = entry->d_name;

        // Do not process relative directories
        if(current_file == "." || current_file == "..")
        {
            continue;
        }

        current_absolute_path = current_directory + "/" + current_file;

        // If current file is a directory, push the current one on the stack,
        // make current file to the current directory and open it
        if(is_directory(current_absolute_path))
        {
            directory_stack.push(make_pair(current_directory, dir));
            current_directory = current_absolute_path;
            goto open_directory;
        }

        // Add filename
        filenames.push_back(make_pair(current_directory, current_file));
    }

    // Sort filenames
    sort(filenames.begin(),filenames.end());

    if(verbosity >= 4)
    {
        vector<pair<string, string> >::const_iterator it;
        for(it = filenames.begin(); it != filenames.end(); it++)
        {
            cout << "Directory " << it->first << " Entry " << it->second << endl;
        }
    }

    // Begin SQL transaction
    msg(2, "Inserting files into the database...");
    result =
    sqlite3_exec(db, begin_transaction, NULL, NULL, &error_message);

    if(result != SQLITE_OK)
    {
        std::string err_msg = "Error while beginning add transaction: ";
                    err_msg += error_message;
        msg(3, err_msg, 2);

        sqlite3_free(error_message);

        return false;
    }
    // Prepare SQL statement
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, add_entry, -1, &stmt, NULL);

    // Bind disc name
    sqlite3_bind_text(stmt, 3, disc_name.c_str(), -1, SQLITE_STATIC);

    //Add files
    const char* d_entry;
    const char* f_entry;
    const char* f_entry_directory = "NULL";

    vector<pair<string, string> >::const_iterator i;
    for(i = filenames.begin(); i != filenames.end(); i++)
    {
        d_entry = i->first.c_str();
        f_entry = i->second.c_str();

        // If needed, UNIXify directory
        if(isalpha(d_entry[0]) && d_entry[1] == ':')
        {
            d_entry += 2;
        }

        // If entry is a directory, replace file with NULL
        if(strlen(f_entry) == 0)
        {
            f_entry = f_entry_directory;
        }

        // Reset SQL statement
        sqlite3_reset(stmt);

        // Bind directory and file
        sqlite3_bind_text(stmt, 1, d_entry, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, f_entry, -1, SQLITE_TRANSIENT);

        // Execute SQL statement
        result =
        sqlite3_step(stmt);

        // Check for errors
        if(result != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);

            msg(3, "Error while add transaction!", 2);

            return false;
        }
    }

    sqlite3_finalize(stmt);


    // End SQL transaction
    result =
    sqlite3_exec(db, end_transaction, NULL, NULL, &error_message);

    if(result != SQLITE_OK)
    {
        std::string err_msg = "Error while ending add transaction: ";
                    err_msg += error_message;
        msg(3, err_msg, 2);

        sqlite3_free(error_message);

        return false;
    }
    msg(3, "Done.");

    return true;
}

bool
DDB::remove_disc(void)
{
    const char* remove_query = "DELETE FROM ddb WHERE disc=?";

    // Check whether the disc is in the database
    if(! is_disc_present(disc_name))
    {
        std::string err_msg = "Disc " + disc_name + " is not in the database!";
        msg(0, err_msg, 2);

        return false;
    }

    // Ask user for confirmation
    char c;
    cout << "Remove disc " << disc_name << " from database? (y/n): ";
    cin >> c;

    if(c == 'y')
    {
        msg(2, "Removing of the disc confirmed.");
    }
    else
    {
        msg(2, "Removing of the disc canceled.");
        return true;
    }

    // Initialize and prepare SQL statement
    int result;
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, remove_query, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, disc_name.c_str(), -1, SQLITE_STATIC);

    // Execute SQL statement
    result =
    sqlite3_step(stmt);

    // Check for errors
    if(result != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);

        msg(3, "Error removing disc!", 2);

        return false;
    }

    // Clean up
    sqlite3_finalize(stmt);

    return true;
}

bool
DDB::list_contents(void)
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
DDB::list_discs(void)
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
                msg(1, "Error while listing contents!", 2);

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
DDB::list_directories(void)
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
                msg(1, "Error while listing contents!", 2);

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
DDB::list_files(void)
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
            path.append("/");
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
                msg(1, "Error while listing contents!", 2);

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
DDB::initialize_database(void)
{
    char* error_message = NULL;

    // Create the table
    int result =
    sqlite3_exec(db, discdb_schema, NULL, NULL, &error_message);

    if(result != SQLITE_OK)
    {
        msg(1, "Error creating table!", 2);

        sqlite3_free(error_message);

        return false;
    }

    // Check the table again
    if(!is_discdb())
    {
        msg(1, "Table was not created!", 2);

        return false;
    }

    return true;
}

bool
DDB::search_text(void)
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
            string absolute_path;
            absolute_path.append((const char*) sqlite3_column_text(stmt, 1));
            absolute_path.push_back('/');
            absolute_path.append((const char*) sqlite3_column_text(stmt, 2));

            files.push_back(make_pair(disc, absolute_path));
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
                msg(1, "Error while listing contents!", 2);

                return false;
            }
        }
    }

    // Sort and print results
    sort(files.begin(), files.end());

    vector<pair<string, string> >::const_iterator i;
    for(i = files.begin(); i != files.end(); i++)
    {
        cout << i->first << ":\t" << i->second << endl;
    }

    return true;
}

void
DDB::print_help(void)
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
DDB::msg(int min_verbosity, const char* message, unsigned int newlines)
{
    if(verbosity >= min_verbosity)
    {
        std::cerr << message;

        for(unsigned int i = 0; i < newlines; i++)
            std::cerr << std::endl;
    }
}

void
DDB::msg(int min_verbosity, const std::string& message, unsigned int newlines)
{
    msg(min_verbosity, message.c_str(), newlines);
}


int main(int argc, char** argv)
{
    DDB ddb(argc, argv);

    try
    {
        ddb.run();
    }
    catch(Exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
