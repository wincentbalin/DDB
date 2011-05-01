/**
 *  db.hpp
 *
 *  Database abstraction part of Disc Data Base.
 *
 *  Copyright (c) 2010-2011 Wincent Balin
 *
 *  Based upon ddb.pl, created years before and serving faithfully until today.
 *
 *  Uses SQLite database version 3.
 *
 *  Published under MIT license. See LICENSE file for further information.
 */

#include "db.hpp"

#include <sstream>
#include <utility>

#include <cassert>

#include <boost/foreach.hpp>

// Use shortcut from example
#define foreach BOOST_FOREACH

//  Deprecated features not wanted
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>

// Use a shortcut
namespace fs = boost::filesystem;


DB::DB(Print* print)
{
    // Store pointer to the printer
    p = print;

    // Perform initialization
    init();
}

void
DB::init(void)
{
    // Reset database pointer
    db = NULL;

    // Set version
    version = 1;

    // Define database format
    format.push_back("CREATE TABLE ddb (directory TEXT NOT NULL, file TEXT, disc TEXT NOT NULL)");
    format.push_back("CREATE INDEX ddb_index ON ddb (directory, file, disc)");
    format.push_back("CREATE TABLE ddb_version(version INTEGER NOT NULL)");
    std::ostringstream ddb_version_table_contents;
    ddb_version_table_contents << "INSERT INTO ddb_version VALUES (" << version << ")";
    format.push_back(ddb_version_table_contents.str());
}

DB::~DB(void) throw(DBError)
{
    // Close database
    close();
}

void
DB::open(const char* dbname, bool initialize) throw(DBError)
{
    std::string error_message = std::string("Could not open file ") + dbname;

    int result;

    // Assume database is not open already
    assert(db == NULL);

    p->msg("Opening database...", Print::VERBOSE);

    // Open database
    result =
    sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message), DBError::FILE_ERROR);

    p->msg("Done.", Print::DEBUG);
}

void
DB::close(void) throw(DBError)
{
    std::string error_message = "Could not close database";
    int result;

    p->msg("Closing database...", Print::VERBOSE);

    // Close database
    result =
    sqlite3_close(db);

    // If something went wrong, throw an exception
    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::FILE_ERROR));

    p->msg("Done.", Print::DEBUG);
}

bool
DB::has_correct_format(void) throw(DBError)
{
    const char* version_check = "SELECT COUNT(*) AS count, version FROM ddb_version";

    std::string error_message = "Could not check database correctness";

    int result;

    bool format_is_correct = false;

    // Prepare SQL statement
    sqlite3_stmt* stmt;

    result =
    sqlite3_prepare_v2(db, version_check, -1, &stmt, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::PREPARE_STATEMENT));

    // Execute SQL statement
    result =
    sqlite3_step(stmt);

    if(result != SQLITE_ROW)
        throw(DBError(error_message, DBError::EXECUTE_STATEMENT));

    // Result should have only one row
    if(sqlite3_column_int(stmt, 0) == 1)
    {
        // Version in database should be equal to the version of this class
        format_is_correct = (sqlite3_column_int(stmt, 1) == version);
    }

    // Finalize SQL statement
    result =
    sqlite3_finalize(stmt);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::FINALIZE_STATEMENT));

    if(!format_is_correct)
        p->msg("Database has wrong format!", Print::INFO);

    // Return correctness
    return format_is_correct;
}

bool
DB::is_disc_present(const char* discname) throw(DBError)
{
    const char* disc_presence_check = "SELECT DISTINCT disc FROM ddb WHERE disc LIKE ?";

    int result;

    bool disc_present = false;

    std::string error_message = "Could not check disc presence";

    // Prepare SQL statement
    sqlite3_stmt* stmt;

    result =
    sqlite3_prepare_v2(db, disc_presence_check, -1, &stmt, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::PREPARE_STATEMENT));

    // Execute SQL statement
    result =
    sqlite3_step(stmt);

    // Check whether we have at least one disc in the database
    if(result == SQLITE_ROW)
    {
        disc_present = true;
    }
    else if(result == SQLITE_DONE)
    {
        disc_present = false;
    }
    else
    {
        // We got an error
        throw(DBError(error_message, DBError::EXECUTE_STATEMENT));
    }

    // Finalize SQL statement
    result =
    sqlite3_finalize(stmt);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::FINALIZE_STATEMENT));

    // Return disc presence
    return disc_present;
}

void
DB::add_disc(const char* disc_name, const char* starting_path) throw(DBError)
{
    const char* begin_transaction = "BEGIN";
    const char* add_entry = "INSERT INTO ddb (directory, file, disc) VALUES (?, ?, ?)";
    const char* end_transaction = "COMMIT";

    std::string error_message = std::string("Could not add disc ") + disc_name;

    int result;

    // Declare disc root directory
    fs::path disc_path(starting_path);

    // Check, whether disc path is a directory
    if(!fs::is_directory(disc_path))
        throw(DBError(std::string("Path ") + starting_path + " is not a directory", DBError::FILE_ERROR));

    // Container of file names
    std::vector<std::pair<fs::path, bool> > filenames;

    // Open directory and iterate through it recursively
    fs::path current_path;
    bool current_path_is_directory;
    fs::recursive_directory_iterator end;

    for(fs::recursive_directory_iterator dir(disc_path);
        dir != end;
        dir++)
    {
        current_path = dir->path();
        current_path_is_directory = fs::is_directory(current_path);

        // Put current file name into vector
        filenames.push_back(std::make_pair(current_path, current_path_is_directory));
    }

    // Sort filenames
    sort(filenames.begin(),filenames.end());

    // Print file names, if verbosity is set high enough
    if(p->get_verbosity() >= Print::VERBOSE_DEBUG)
    {
        std::pair<fs::path, bool> it;
        foreach(it, filenames)
        {
            std::cout << (it.second ? "Directory" : "File") << " " << it.first << std::endl;
        }
    }

    // Begin transaction
    result =
    sqlite3_exec(db, begin_transaction, NULL, NULL, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::BEGIN_TRANSACTION));

    // Prepare SQL statement
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, add_entry, -1, &stmt, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::PREPARE_STATEMENT));

    // Bind disc name
    sqlite3_bind_text(stmt, 3, disc_name, -1, SQLITE_STATIC);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::BIND_PARAMETER));

    p->msg("Inserting files into database...", Print::VERBOSE);

    // Add files
    bool file_is_directory;
    fs::path path;
    std::string d_entry;
    std::string f_entry;

    std::pair<fs::path, bool> it;
    foreach(it, filenames)
    {
        path = it.first;
        file_is_directory = it.second;

        d_entry = file_is_directory ?
                    path.generic_string() :
                    path.parent_path().generic_string();

        f_entry = file_is_directory ?
                    "NULL" :
                    path.filename().generic_string();

        // Reset SQL statement
        result =
        sqlite3_reset(stmt);

        if(result != SQLITE_OK)
            throw(DBError(error_message, DBError::RESET_STATEMENT));

        // Bind directory and file
        result =
        sqlite3_bind_text(stmt, 1, d_entry.c_str(), -1, SQLITE_STATIC);

        if(result != SQLITE_OK)
            throw(DBError(error_message, DBError::BIND_PARAMETER));

        result =
        sqlite3_bind_text(stmt, 2, f_entry.c_str(), -1, SQLITE_STATIC);

        if(result != SQLITE_OK)
            throw(DBError(error_message, DBError::BIND_PARAMETER));

        // Execute SQL statement
        result =
        sqlite3_step(stmt);

        // Check for errors
        if(result != SQLITE_DONE)
            throw(DBError(error_message, DBError::EXECUTE_STATEMENT));
    }

    // End transaction
    result =
    sqlite3_exec(db, end_transaction, NULL, NULL, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::END_TRANSACTION));

    p->msg("Done.", Print::DEBUG);
}

void
DB::remove_disc(const char* disc_name) throw(DBError)
{
    const char* remove_query = "DELETE FROM ddb WHERE disc=?";

    int result;

    std::string error_message = std::string("Could not remove disc ") + disc_name;

    // Initialize and prepare SQL statement
    sqlite3_stmt* stmt;

    result =
    sqlite3_prepare_v2(db, remove_query, -1, &stmt, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::PREPARE_STATEMENT));

    // Bind disc name
    result =
    sqlite3_bind_text(stmt, 1, disc_name, -1, SQLITE_STATIC);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::BIND_PARAMETER));

    // Execute SQL statement
    result =
    sqlite3_step(stmt);

    if(result != SQLITE_DONE)
        throw(DBError(error_message, DBError::EXECUTE_STATEMENT));

    // Clean up
    result =
    sqlite3_finalize(stmt);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::FINALIZE_STATEMENT));
}

void
DB::list_files(const char* disc_name, bool directories_only) throw(DBError)
{
    const char* list_files_query = "SELECT directory,file FROM ddb WHERE disc LIKE ?";
    const char* list_directories_query = "SELECT DISTINCT directory FROM ddb WHERE disc LIKE ?";

    const char* list_query = directories_only ? list_directories_query : list_files_query;

    std::string error_message = "Could not list " + directories_only ? "directories" : "files";

    int result;

    // Prepare statement
    sqlite3_stmt* stmt;

    result =
    sqlite3_prepare_v2(db, list_query, -1, &stmt, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::PREPARE_STATEMENT));

    // Bind disc name
    result =
    sqlite3_bind_text(stmt, 1, disc_name, -1, SQLITE_STATIC);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::BIND_PARAMETER));

    // Get data
    const char* directory;
    const char* file;

    while(true)
    {
        // Execute SQL statement
        result =
        sqlite3_step(stmt);

        // Check whether we have at least one disc in the database
        if(result == SQLITE_ROW)
        {
            if(directories_only)
            {
                directory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                p->add_directory(disc_name, directory);
            }
            else
            {
                directory = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                file = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                p->add_file(disc_name, directory, file);
            }
        }
        else if(result == SQLITE_DONE)
        {
            // No more results
            break;
        }
        else
        {
            // We got an error
            throw(DBError(error_message, DBError::EXECUTE_STATEMENT));
        }
    }

    // Finalize statement
    result =
    sqlite3_finalize(stmt);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::FINALIZE_STATEMENT));
}
