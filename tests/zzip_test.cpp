#include <string>
#include <unordered_set>

#include <zstd/zstd.h>
#include <zstd/common/mem.h>

#include "cata_catch.h"
#include "mmap_file.h"
#include "std_hash_fs_path.h"
#include "zzip.h"

namespace
{
std::string_view _view( const std::vector<std::byte> &bytes )
{
    return std::string_view( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
}

std::vector<std::byte> make_bytes( size_t count )
{
    std::vector<std::byte> bytes;
    for( size_t i = 0; i < count; ++i ) {
        bytes.push_back( static_cast<std::byte>( rand() %
                         std::numeric_limits<std::underlying_type_t<std::byte>>::max() ) );
    }
    return bytes;
}
}

TEST_CASE( "zzip_basic_functionality", "[.][zzip]" )
{
    std::shared_ptr<mmap_file> mem_file = mmap_file::map_writeable_memory( 0 );

    SECTION( "Text files" ) {
        std::unordered_map<std::string, std::string> files = {
            {"first.txt", "first"},
            {"second.txt", "second"},
            {"third.txt", "third"},
        };

        {
            std::optional<zzip> z = zzip::load( mem_file );
            REQUIRE( z.has_value() );
            for( auto& [name, contents] : files ) {
                REQUIRE( z->add_file( name, contents ) );
            }
        }
        SECTION( "zzip has all expected contents" ) {
            std::optional<zzip> z = zzip::load( mem_file );
            REQUIRE( z.has_value() );
            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                REQUIRE( _view( data ) == contents );
            }
        }
        SECTION( "zzip has no unexpected contents" ) {
            std::optional<zzip> z = zzip::load( mem_file );
            REQUIRE( z.has_value() );
            for( auto entry : z->get_entries() ) {
                REQUIRE( files.find( entry.generic_u8string() ) != files.end() );
            }
        }
    }
    SECTION( "Binary files" ) {
        std::unordered_map<std::string, std::vector<std::byte>> files{
            {"bytes.bin", make_bytes( 1024 )},
            {"bytes2.bin", make_bytes( 1024 )},
            {"zeroes.bin", std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
        };

        {
            std::optional<zzip> z = zzip::load( mem_file );
            REQUIRE( z.has_value() );
            for( auto& [name, contents] : files ) {
                z->add_file( name, _view( contents ) );
            }
        }
        SECTION( "zzip has all expected contents" ) {
            std::optional<zzip> z = zzip::load( mem_file );
            REQUIRE( z.has_value() );
            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                REQUIRE( _view( data ) == _view( contents ) );
            }
        }
        SECTION( "zzip has no unexpected contents" ) {
            std::optional<zzip> z = zzip::load( mem_file );
            REQUIRE( z.has_value() );
            for( auto entry : z->get_entries() ) {
                REQUIRE( files.find( entry.generic_u8string() ) != files.end() );
            }
        }
    }
}

TEST_CASE( "zzip_compaction", "[.][zzip]" )
{
    std::unordered_map<std::string, std::vector<std::byte>> files{
        {"bytes.bin", make_bytes( 1024 )},
        {"bytes2.bin", make_bytes( 1024 )},
        {"zeroes.bin", std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
    };
    std::shared_ptr<mmap_file> mem_file = mmap_file::map_writeable_memory( 0 );

    std::optional<zzip> z = zzip::load( mem_file );
    REQUIRE( z.has_value() );
    for( auto& [name, contents] : files ) {
        REQUIRE( z->add_file( name, _view( contents ) ) );
    }

    SECTION( "Compaction" ) {
        SECTION( "Compaction saves space" ) {
            std::shared_ptr<mmap_file> mem_file2 = mmap_file::map_writeable_memory( 0 );
            z->compact_to( mem_file2 );
            CHECK( mem_file2->len() < mem_file->len() );
        }
        SECTION( "Compaction is deterministic" ) {
            std::shared_ptr<mmap_file> mem_file2 = mmap_file::map_writeable_memory( 0 );
            z->compact_to( mem_file2 );
            std::shared_ptr<mmap_file> mem_file3 = mmap_file::map_writeable_memory( 0 );
            z->compact_to( mem_file3 );
            CHECK( mem_file2->len() == mem_file3->len() );
            CHECK( memcmp( mem_file2->base(), mem_file3->base(), mem_file2->len() ) == 0 );
        }
    }
}

TEST_CASE( "zzip_deletion", "[.][zzip]" )
{
    std::unordered_map<std::string, std::vector<std::byte>> files{
        {"bytes.bin", make_bytes( 1024 )},
        {"bytes2.bin", make_bytes( 1024 )},
        {"zeroes.bin", std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
    };
    std::shared_ptr<mmap_file> mem_file = mmap_file::map_writeable_memory( 0 );
    REQUIRE( mem_file );

    std::optional<zzip> z = zzip::load( mem_file );
    REQUIRE( z.has_value() );
    for( auto& [name, contents] : files ) {
        REQUIRE( z->add_file( name, _view( contents ) ) );
    }

    SECTION( "Deletion" ) {
        SECTION( "Deletion works" ) {
            REQUIRE( z->delete_files( { "bytes2.bin" } ) );
            files.erase( "bytes2.bin" );

            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                CHECK( _view( data ) == _view( contents ) );
            }
            for( auto entry : z->get_entries() ) {
                CHECK( files.find( entry.generic_u8string() ) != files.end() );
            }
        }
        SECTION( "Readding a file after deletion works" ) {
            REQUIRE( z->delete_files( { "bytes2.bin" } ) );
            files["bytes2.bin"] = make_bytes( 512 );
            z->add_file( "bytes2.bin", _view( files["bytes2.bin"] ) );

            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                CHECK( _view( data ) == _view( contents ) );
            }
            for( auto entry : z->get_entries() ) {
                CHECK( files.find( entry.generic_u8string() ) != files.end() );
            }
        }
        SECTION( "Compaction skips deleted files" ) {
            std::shared_ptr<mmap_file> mem_file2 = mmap_file::map_writeable_memory( 0 );
            REQUIRE( z->compact_to( mem_file2 ) );

            z = zzip::load( mem_file2 );
            REQUIRE( z->delete_files( { "bytes2.bin" } ) );

            std::shared_ptr<mmap_file> mem_file3 = mmap_file::map_writeable_memory( 0 );
            REQUIRE( z->compact_to( mem_file3 ) );

            CHECK( mem_file3->len() < mem_file2->len() );
        }
    }
}

TEST_CASE( "zzip_corruption_recovery", "[.][zzip]" )
{
    std::unordered_map<std::string, std::vector<std::byte>> files{
        {"bytes.bin", make_bytes( 1024 )},
        {"bytes2.bin", make_bytes( 1024 )},
        {"zeroes.bin", std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
    };
    std::shared_ptr<mmap_file> mem_file = mmap_file::map_writeable_memory( 0 );

    std::optional<zzip> z = zzip::load( mem_file );
    for( auto& [name, contents] : files ) {
        z->add_file( name, _view( contents ) );
    }

    SECTION( "Footer truncated" ) {
        z.reset();
        mem_file->resize_file( mem_file->len() - 1 );

        SECTION( "Recovery works" ) {
            z = zzip::load( mem_file );
            REQUIRE( z.has_value() );

            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                CHECK( _view( data ) == _view( contents ) );
            }
            for( auto entry : z->get_entries() ) {
                CHECK( files.find( entry.generic_u8string() ) != files.end() );
            }
        }
    }

    SECTION( "Hash corrupted" ) {
        z.reset();

        std::array<uint64_t, 2> buf;

        MEM_writeLE64( &buf[0], 0 );
        MEM_writeLE64( &buf[1], 0 );

        size_t footer_frame_size = ZSTD_writeSkippableFrame(
                                       mem_file->base(),
                                       mem_file->len(),
                                       buf.data(),
                                       sizeof( buf ),
                                       /* kFooterChecksumMagic */ 15 );
        REQUIRE( !ZSTD_isError( footer_frame_size ) );

        SECTION( "Recovery works" ) {
            z = zzip::load( mem_file );
            REQUIRE( z.has_value() );

            for( auto& [name, contents] : files ) {
                CAPTURE( name );
                std::vector<std::byte> data = z->get_file( name );
                CHECK( _view( data ) == _view( contents ) );
            }
            for( auto entry : z->get_entries() ) {
                CHECK( files.find( entry.generic_u8string() ) != files.end() );
            }
        }
    }
}
