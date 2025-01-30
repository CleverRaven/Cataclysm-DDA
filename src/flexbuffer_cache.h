#pragma once
#ifndef CATA_SRC_FLEXBUFFER_CACHE_H
#define CATA_SRC_FLEXBUFFER_CACHE_H

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <unordered_map>

#include <flatbuffers/flexbuffers.h>


struct flexbuffer_storage {
    virtual ~flexbuffer_storage() = default;
    virtual const uint8_t *data() const = 0;
    virtual size_t size() const = 0;
};

struct parsed_flexbuffer {
        parsed_flexbuffer() = delete;

        virtual ~parsed_flexbuffer() = default;

        // Returns true if the source of truth has changed since this flexbuffer was parsed.
        virtual bool is_stale() const = 0;

        // Returns an istream suitable for constructing a JsonIn from representing the textual
        // source for this flexbuffer.
        // Returns nullptr if is_stale() returns false or the source of truth is otherwise inaccessible.
        // May throw exceptions.
        virtual std::unique_ptr<std::istream> get_source_stream() const noexcept( false ) = 0;

        // Returns the path to a file containing the text source for the flexbuffer, if it exists.
        virtual std::filesystem::path get_source_path() const noexcept = 0;

        // Returns reference to the underlying storage containing the FlexBuffer binary data.
        const std::shared_ptr<flexbuffer_storage> &get_storage() const {
            return storage_;
        }

    protected:
        explicit parsed_flexbuffer( std::shared_ptr<flexbuffer_storage> storage );

        std::shared_ptr<flexbuffer_storage> storage_;
};

class flexbuffer_disk_cache;
struct flexbuffer_storage;

class flexbuffer_cache
{
        using shared_flexbuffer = std::shared_ptr<parsed_flexbuffer>;

    public:
        explicit flexbuffer_cache( const std::filesystem::path &cache_directory,
                                   const std::filesystem::path &root_directory );
        ~flexbuffer_cache();

        // Throw exceptions on IO and parse errors.
        static shared_flexbuffer parse( std::filesystem::path json_source_path,
                                        size_t offset = 0 ) noexcept( false );
        shared_flexbuffer parse_and_cache( std::filesystem::path lexically_normal_json_source_path,
                                           size_t offset = 0 ) noexcept( false ) ;

        static shared_flexbuffer parse_buffer( std::string buffer ) noexcept( false );

    private:
        flexbuffer_cache( flexbuffer_cache && ) noexcept = default;

        struct ParsedBuffer;

        // Map of original json file path to disk serialized FlexBuffer path and mtime of input.
        std::unique_ptr<flexbuffer_disk_cache> disk_cache_;
};

#endif // CATA_SRC_FLEXBUFFER_CACHE_H
