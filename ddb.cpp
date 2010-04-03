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

#include <getopt.h>


using namespace std;


ddb::ddb(int argc, char** argv) :
    db_filename("discdb"), do_initialize(false),
    do_add(false), do_list(false), do_remove(false),
    directories_only(false), verbosity(0)
{


    // Parse command line arguments
    // Getopt variables
    int ch, option_index;
    static struct option long_options[] =
    {
        {"add",          1, 0, 'a'},
        {"directory",    0, 0, 'd'},
        {"file",         1, 0, 'f'},
        {"help",         0, 0, 'h'},
        {"initialize",   0, 0, 'i'},
        {"list",         0, 0, 'l'},
        {"quite",        0, 0, 'q'},
        {"remove",       1, 0, 'r'},
        {"verbose",      0, 0, 'v'},
        { 0,             0, 0,  0 }
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
                disc_root = argv[argc-1];
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
                // Store disc name if needed
                if(optarg)
                {
                    disc_name = optarg;
                }
                break;

            // Quite
            case 'q':
                verbosity--;
                break;

            // Remove disc
            case 'r':
                do_remove = true;
                if(optarg)
                {
                    disc_name = optarg;
                }
                else
                {
                    print_help();
                    exit(EXIT_FAILURE);
                }
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
}

ddb::~ddb(void)
{
}

void
ddb::run(void)
{
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

    ddb.run();

    return EXIT_SUCCESS;
}
