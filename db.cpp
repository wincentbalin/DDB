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

#include <cassert>

#include <boost/foreach.hpp>

// Use shortcut from example
#define foreach BOOST_FOREACH

//  Deprecated features not wanted
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>

// Use a shortcut
namespace fs = boost::filesystem;


DB::DB(void)
{
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
    int result;

    // Assume database is not open already
    assert(db == NULL);

    // Open database
    result =
    sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE, NULL);

    // If something went wrong, throw an exception
    if(result != SQLITE_OK)
        throw(DBError(std::string("Could not open file ") + dbname), DBError::FILE_ERROR);
}

void
DB::close(void) throw(DBError)
{
    int result;

    // Close database
    result =
    sqlite3_close(db);

    // If something went wrong, throw an exception
    if(result != SQLITE_OK)
        throw(DBError("Could not close database", DBError::FILE_ERROR));
}

bool
DB::has_correct_format(void) throw(DBError)
{
    const char* version_check = "SELECT COUNT(*) AS count, version FROM ddb_version";
    sqlite3_stmt* stmt;
    int result;
    bool format_is_correct = false;

    // Prepare SQL statement
    result =
    sqlite3_prepare_v2(db, version_check, -1, &stmt, NULL);

    // Check correctness of statement preparation
    if(result != SQLITE_OK)
        throw(DBError("Could not check database correctness", DBError::PREPARE_STATEMENT));

    // Execute SQL statement
    result =
    sqlite3_step(stmt);

    // Check correctness of statement execution
    if(result != SQLITE_ROW)
        throw(DBError("Could not check database correctness", DBError::EXECUTE_STATEMENT));

    // Result should have only one row
    if(sqlite3_column_int(stmt, 0) == 1)
    {
        // Version in database should be equal to the version of this class
        format_is_correct = (sqlite3_column_int(stmt, 1) == version);
    }

    // Finalize SQL statement
    result =
    sqlite3_finalize(stmt);

    // Check correctness of statement finalization
    if(result != SQLITE_OK)
        throw(DBError("Could not check database correctness", DBError::FINALIZE_STATEMENT));

    // Return correctness
    return format_is_correct;
}

bool
DB::is_disc_present(const char* discname) throw(DBError)
{
    const char* disc_presence_check = "SELECT DISTINCT disc FROM ddb WHERE disc LIKE ?";
    sqlite3_stmt* stmt;
    int result;
    bool disc_present = false;

    // Prepare SQL statement
    result =
    sqlite3_prepare_v2(db, disc_presence_check, -1, &stmt, NULL);

    // Check correctness of statement preparation
    if(result != SQLITE_OK)
        throw(DBError("Could not check disc presence", DBError::PREPARE_STATEMENT));

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
        throw(DBError("Could not check disc presence", DBError::EXECUTE_STATEMENT));
    }

    // Finalize SQL statement
    result =
    sqlite3_finalize(stmt);

    // Check correctness of statement finalization
    if(result != SQLITE_OK)
        throw(DBError("Could not check disc presence", DBError::FINALIZE_STATEMENT));

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


    // Begin transaction
    result =
    sqlite3_exec(db, begin_transaction, NULL, NULL, NULL);

    if(result != SQLITE_OK)
        throw(DBError(error_message, DBError::BEGIN_TRANSACTION));

    // Prepare SQL statement
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, add_entry, -1, &stmt, NULL);

    // Bind disc name
    sqlite3_bind_text(stmt, 3, disc_name, -1, SQLITE_STATIC);

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
        sqlite3_reset(stmt);

        // Bind directory and file
        sqlite3_bind_text(stmt, 1, d_entry.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, f_entry.c_str(), -1, SQLITE_STATIC);

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
}

void
DB::remove_disc(const char* disc_name) throw(DBError)
{
    const char* remove_query = "DELETE FROM ddb WHERE disc=?";

    int result;
    sqlite3_stmt* stmt;

    std::string error_message = std::string("Could not remove disc ") + disc_name;

    // Initialize and prepare SQL statement
    sqlite3_prepare_v2(db, remove_query, -1, &stmt, NULL);

    sqlite3_bind_text(stmt, 1, disc_name, -1, SQLITE_STATIC);

    // Execute SQL statement
    result =
    sqlite3_step(stmt);

    // Check for errors
    if(result != SQLITE_DONE)
        throw(DBError(error_message, DBError::EXECUTE_STATEMENT));

    // Clean up
    sqlite3_finalize(stmt);
}

void
DB::list_files(const char* disc_name, bool directories_only) throw(DBError)
{
    const char* list_files_query = "SELECT directory,file FROM ddb WHERE disc LIKE ?";
    const char* list_directories_query = "SELECT DISTINCT directory FROM ddb WHERE disc LIKE ?";

    const char* list_query = directories_only ? list_directories_query : list_files_query;


}
