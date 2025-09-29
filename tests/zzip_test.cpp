#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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
        // NOLINTNEXTLINE(cata-determinism,cert-msc30-c,cert-msc50-cpp)
        bytes.push_back( static_cast<std::byte>( rand() %
                         std::numeric_limits<std::underlying_type_t<std::byte>>::max() ) );
    }
    return bytes;
}

} // namespace

TEST_CASE( "zzip_basic_functionality", "[.][zzip]" )
{
    std::shared_ptr<mmap_file> mem_file = mmap_file::map_writeable_memory( 0 );

    SECTION( "Text files" ) {
        std::unordered_map<std::filesystem::path, std::string, std_fs_path_hash> files = {
            {std::filesystem::u8path( "first.txt" ), "first"},
            {std::filesystem::u8path( "second.txt" ), "second"},
            {std::filesystem::u8path( "third.txt" ), "third"},
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
            for( std::filesystem::path const &entry : z->get_entries() ) {
                REQUIRE( files.find( entry ) != files.end() );
            }
        }
    }
    SECTION( "Binary files" ) {
        std::unordered_map<std::filesystem::path, std::vector<std::byte>, std_fs_path_hash> files{
            {std::filesystem::u8path( "bytes.bin" ), make_bytes( 1024 )},
            {std::filesystem::u8path( "bytes2.bin" ), make_bytes( 1024 )},
            {std::filesystem::u8path( "zeroes.bin" ), std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
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
            for( std::filesystem::path const &entry : z->get_entries() ) {
                REQUIRE( files.find( entry ) != files.end() );
            }
        }
    }
}

TEST_CASE( "zzip_compaction", "[.][zzip]" )
{
    std::unordered_map<std::filesystem::path, std::vector<std::byte>, std_fs_path_hash> files{
        {std::filesystem::u8path( "bytes.bin" ), make_bytes( 1024 )},
        {std::filesystem::u8path( "bytes2.bin" ), make_bytes( 1024 )},
        {std::filesystem::u8path( "zeroes.bin" ), std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
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
    std::unordered_map<std::filesystem::path, std::vector<std::byte>, std_fs_path_hash> files{
        {std::filesystem::u8path( "bytes.bin" ), make_bytes( 1024 )},
        {std::filesystem::u8path( "bytes2.bin" ), make_bytes( 1024 )},
        {std::filesystem::u8path( "zeroes.bin" ), std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
    };
    std::shared_ptr<mmap_file> mem_file = mmap_file::map_writeable_memory( 0 );
    REQUIRE( mem_file );

    std::optional<zzip> z = zzip::load( mem_file );
    REQUIRE( z.has_value() );
    for( auto& [name, contents] : files ) {
        REQUIRE( z->add_file( name, _view( contents ) ) );
    }

    SECTION( "Deletion" ) {
        std::filesystem::path bytes2_path = std::filesystem::u8path( "bytes2.bin" );
        SECTION( "Deletion works" ) {
            REQUIRE( z->delete_files( { bytes2_path } ) );
            files.erase( bytes2_path );

            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                CHECK( _view( data ) == _view( contents ) );
            }
            for( std::filesystem::path const &entry : z->get_entries() ) {
                CHECK( files.find( entry ) != files.end() );
            }
        }
        SECTION( "Readding a file after deletion works" ) {
            REQUIRE( z->delete_files( { bytes2_path } ) );
            files[bytes2_path] = make_bytes( 512 );
            z->add_file( bytes2_path, _view( files[bytes2_path] ) );

            for( auto& [name, contents] : files ) {
                std::vector<std::byte> data = z->get_file( name );
                CHECK( _view( data ) == _view( contents ) );
            }
            for( std::filesystem::path const &entry : z->get_entries() ) {
                CHECK( files.find( entry ) != files.end() );
            }
        }
        SECTION( "Compaction skips deleted files" ) {
            std::shared_ptr<mmap_file> mem_file2 = mmap_file::map_writeable_memory( 0 );
            REQUIRE( z->compact_to( mem_file2 ) );

            z = zzip::load( mem_file2 );
            REQUIRE( z->delete_files( { bytes2_path } ) );

            std::shared_ptr<mmap_file> mem_file3 = mmap_file::map_writeable_memory( 0 );
            REQUIRE( z->compact_to( mem_file3 ) );

            CHECK( mem_file3->len() < mem_file2->len() );
        }
    }
}

TEST_CASE( "zzip_corruption_recovery", "[.][zzip]" )
{
    std::unordered_map<std::filesystem::path, std::vector<std::byte>, std_fs_path_hash> files{
        {std::filesystem::u8path( "bytes.bin" ), make_bytes( 1024 )},
        {std::filesystem::u8path( "bytes2.bin" ), make_bytes( 1024 )},
        {std::filesystem::u8path( "zeroes.bin" ), std::vector<std::byte>{ 1024, static_cast<std::byte>( 0 ) }},
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
            for( std::filesystem::path const &entry : z->get_entries() ) {
                CHECK( files.find( entry ) != files.end() );
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
            for( std::filesystem::path const &entry : z->get_entries() ) {
                CHECK( files.find( entry ) != files.end() );
            }
        }
    }

    SECTION( "Content truncated" ) {
        SECTION( "Partial recovery works" ) {
            std::vector<zzip::entry_layout> layout = z->get_layout();

            std::sort( layout.begin(), layout.end(), []( const auto & l, const auto & r ) {
                return l.offset < r.offset;
            } );

            for( int i = layout.size() - 1; i > 0; --i ) {
                auto& [name, offset, len] = layout[i];
                CAPTURE( name );

                std::unordered_map<std::filesystem::path, bool, std_fs_path_hash> remaining_files;
                for( int j = 0; j < i; ++j ) {
                    remaining_files.emplace( layout[j].path, false );
                }

                for( size_t truncated_len : std::array{ len - 1, len / 2, static_cast<size_t>( 1 ) } ) {
                    z.reset();

                    // Truncate each file one byte off the end, in the middle, and one byte from the start

                    mem_file->resize_file( offset + truncated_len );

                    z = zzip::load( mem_file );
                    REQUIRE( z.has_value() );

                    for( std::filesystem::path const &entry : z->get_entries() ) {
                        CAPTURE( entry );
                        auto it = remaining_files.find( entry );
                        CHECK( it != remaining_files.end() );
                        it->second = true;
                    }
                    for( auto& [file, found] : remaining_files ) {
                        CAPTURE( file );
                        CHECK( found );
                        found = false;
                    }
                }
            }
        }

        SECTION( "Older versions of files are recoverable" ) {
            std::unordered_map<std::filesystem::path, std::vector<std::byte>, std_fs_path_hash> files2{
                {std::filesystem::u8path( "bytes.bin" ), make_bytes( 1024 )},
                {std::filesystem::u8path( "bytes2.bin" ), make_bytes( 1024 )},
                {std::filesystem::u8path( "zeroes.bin" ), std::vector<std::byte>{ 1024, static_cast<std::byte>( 1 ) }},
            };

            for( auto& [name, contents] : files2 ) {
                z->add_file( name, _view( contents ) );
            }

            std::vector<zzip::entry_layout> layout = z->get_layout();

            std::sort( layout.begin(), layout.end(), []( const zzip::entry_layout & l,
            const zzip::entry_layout & r ) {
                return l.offset < r.offset;
            } );

            for( int i = layout.size() - 1; i > 0; --i ) {
                auto& [name, offset, len] = layout[i];
                CAPTURE( name );

                std::unordered_map<std::filesystem::path, bool, std_fs_path_hash> remaining_new_files;
                for( int j = 0; j < i; ++j ) {
                    remaining_new_files.emplace( layout[j].path, false );
                }

                size_t truncated_len = len / 2;

                z.reset();
                // Truncate each overwritten file, verify when we reload we get the older copy.
                mem_file->resize_file( offset + truncated_len );

                z = zzip::load( mem_file );
                REQUIRE( z.has_value() );

                for( std::filesystem::path const &entry : z->get_entries() ) {
                    CAPTURE( entry );
                    auto it = remaining_new_files.find( entry );
                    if( it == remaining_new_files.end() ) {
                        auto it2 = files.find( entry );
                        CHECK( it2 != files.end() );
                        CHECK( _view( it2->second ) == _view( z->get_file( entry ) ) );
                    } else {
                        CHECK( _view( files2[entry] ) == _view( z->get_file( entry ) ) );
                    }
                }
            }
        }
    }
}
