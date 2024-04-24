#pragma once
#ifndef CATA_SRC_MEMORIAL_LOGGER_H
#define CATA_SRC_MEMORIAL_LOGGER_H

#include <iosfwd>
#include <new>
#include <optional>
#include <vector>

#include "calendar.h"
#include "event_subscriber.h"
#include "string_formatter.h"
#include "type_id.h"

namespace cata
{
class event;
}  // namespace cata
class JsonObject;
class JsonOut;

class memorial_log_entry
{
    public:
        memorial_log_entry() = default;
        explicit memorial_log_entry( const std::string &preformatted_msg );
        memorial_log_entry( time_point, const oter_type_str_id &, std::string_view oter_name,
                            std::string_view msg );

        std::string to_string() const;

        void deserialize( const JsonObject &jo );
        void serialize( JsonOut & ) const;
    private:
        time_point time_;
        oter_type_str_id oter_id_;
        std::string oter_name_;
        std::string message_;

        // For legacy lines loaded from the text memorial file.  For any given
        // memorial_log_entry either the following value will be populated or
        // the above four values (but not both).
        std::optional<std::string> preformatted_;
};

class memorial_logger : public event_subscriber
{
    public:
        void clear();

        void add( std::string_view male_msg,
                  std::string_view female_msg );
        template<typename Arg, typename ...Args>
        void add( const std::string_view male_msg, const std::string &female_msg,
                  Arg &&arg, Args &&... args ) {
            return add( string_format( male_msg, arg, args... ),
                        string_format( female_msg, arg, args... ) );
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

        using event_subscriber::notify;
        void notify( const cata::event & ) override;
    private:
        std::vector<memorial_log_entry> log;
};

memorial_logger &get_memorial();

#endif // CATA_SRC_MEMORIAL_LOGGER_H
