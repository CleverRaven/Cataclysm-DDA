#pragma once
#ifndef CATA_SRC_JSON_LOADER_H
#define CATA_SRC_JSON_LOADER_H

#include <ghc/fs_std_fwd.hpp>

#include "path_info.h"
#include "flexbuffer_json.h"

class json_loader
{
    public:
        // Create a JsonValue from the given json source file, optionally starting parsing
        // at the given offset in the file (eg. because it starts with some non-json
        // content like a version header). Throws if the file cannot be found or fails to parse.
        static JsonValue from_path( const cata_path &source_file ) noexcept( false );
        static JsonValue from_path_at_offset( const cata_path &source_file,
                                              size_t offset = 0 ) noexcept( false );

        // Like json_loader::from_path, except does not throw if the file does not exist. It will still throw if the json cannot be parsed.
        static cata::optional<JsonValue> from_path_opt( const cata_path &source_file ) noexcept( false );
        static cata::optional<JsonValue> from_path_at_offset_opt( const cata_path &source_file,
                size_t offset = 0 ) noexcept( false );

        // Like json_loader::from_path, except instead of parsing data from a file, will parse data from a string in memory.
        static JsonValue from_string( std::string const &data ) noexcept( false );
        static cata::optional<JsonValue> from_string_opt( std::string const &data ) noexcept( false );

};

#endif // CATA_SRC_JSON_LOADER_H
