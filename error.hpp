/**
 *  error.hpp
 *
 *  Exception include part of Disc Data Base.
 *
 *  Copyright (c) 2010-2011 Wincent Balin
 *
 *  Based upon ddb.pl, created years before and serving faithfully until today.
 *
 *  Uses SQLite database version 3.
 *
 *  Published under MIT license. See LICENSE file for further information.
 */

#ifndef ERROR_HPP
#define ERROR_HPP

#include <exception>
#include <string>

// Modeled after the RtError class from RtAudio package
class DBError : public std::exception
{
public:
    enum Type
    {
        UNSPECIFIED,
        WARNING,
        FILE_ERROR,
        RESOURCE_BUSY,
        PREPARE_STATEMENT,
        BIND_PARAMETER,
        EXECUTE_STATEMENT,
        RESET_STATEMENT,
        FINALIZE_STATEMENT,
        BEGIN_TRANSACTION,
        END_TRANSACTION
    };
    DBError(const std::string& message, Type type = DBError::WARNING) throw() : msg(message), t(type) {}
    virtual ~DBError(void) throw() {}
    virtual void print_message(void) const throw() { std::cerr << std::endl << msg << std::endl << std::endl; }
    virtual const Type& get_type(void) const throw() { return t; }
    virtual const std::string& get_message(void) const throw() { return msg; }
    virtual const char* what(void) const throw() { return msg.c_str(); }
protected:
    std::string msg;
    Type t;
};

#endif /* ERROR_HPP */
