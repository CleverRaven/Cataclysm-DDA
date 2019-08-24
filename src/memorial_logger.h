#pragma once
#ifndef CATA_MEMORIAL_LOGGER_H
#define CATA_MEMORIAL_LOGGER_H

#include <map>
#include <string>
#include <vector>

#include "event_bus.h"

class memorial_logger : public event_subscriber
{
    public:
        void clear();

        void add( const std::string &male_msg,
                  const std::string &female_msg );
        template<typename ...Args>
        void add( const char *const male_msg, const char *const female_msg, Args &&... args ) {
            return add( string_format( male_msg, args... ),
                        string_format( female_msg, args... ) );
        }
        template<typename ...Args>
        void add( const std::string &male_msg, const std::string &female_msg,
                  Args &&... args ) {
            return add( string_format( male_msg, args... ),
                        string_format( female_msg, args... ) );
        }

        // Loads the memorial log from a file
        void load( std::istream &fin );
        // Dumps all memorial events into a single newline-delimited string
        // (this is the content of the temporary file used to preserve the log
        // over saves, not the final memorial file).
        std::string dump() const;

        // Prints out the final memorial file
        void write( std::ostream &memorial_file, const std::string &epitaph ) const;

        void notify( const event & ) override;
    private:
        std::vector<std::string> log;
};

#endif // CATA_MEMORIAL_LOGGER_H
