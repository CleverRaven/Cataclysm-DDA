#pragma once
#ifndef CATA_SRC_MEMORIAL_LOGGER_H
#define CATA_SRC_MEMORIAL_LOGGER_H

#include <iosfwd>
#include <string>
#include <vector>

#include "event_bus.h"
#include "string_formatter.h"

namespace cata
{
class event;
}  // namespace cata

class memorial_log_entry
{
    public:
        memorial_log_entry() = default;
        memorial_log_entry( const std::string &preformatted_msg );
        memorial_log_entry( time_point, const oter_type_str_id &, const std::string &oter_name,
                            const std::string &msg );

        std::string to_string() const;

        void deserialize( JsonIn & );
        void serialize( JsonOut & ) const;
    private:
        time_point time_;
        oter_type_str_id oter_id_;
        std::string oter_name_;
        std::string message_;

        // For legacy lines loaded from the text memorial file.  For any given
        // memorial_log_entry either the following value will be populated or
        // the above four values (but not both).
        cata::optional<std::string> preformatted_;
};

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
        void load( std::istream & );
        void save( std::ostream & ) const;
        // Dumps all memorial events into a single newline-delimited string
        // (this is the content of the temporary file used to preserve the log
        // over saves, not the final memorial file).
        std::string dump() const;

        // Prints out the final memorial file
        void write_text_memorial( std::ostream &memorial_file,
                                  const std::string &epitaph ) const;
        void write_json_memorial( std::ostream &memorial_file ) const;

        void notify( const cata::event & ) override;
    private:
        std::vector<memorial_log_entry> log;
};

#endif // CATA_SRC_MEMORIAL_LOGGER_H
