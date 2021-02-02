#include <sstream>

#include "catch/catch.hpp"
#include "filesystem.h"
#include "string_formatter.h"
#include "path_info.h"
#include "game.h"
#include "cata_utility.h"

static std::string str_to_hex( const std::string &s )
{
    std::stringstream ss;
    for( char c : s ) {
        ss << std::hex << std::uppercase <<
           static_cast<int>( static_cast<unsigned char>( c ) ) << " ";
    }
    return ss.str();
}

static void filesystem_test_group( int serial, const std::string &s1, const std::string &s2,
                                   const std::string &s3 )
{
    CAPTURE( serial );
    CAPTURE( s1 );
    CAPTURE( str_to_hex( s1 ) );
    CAPTURE( s2 );
    CAPTURE( str_to_hex( s2 ) );
    CAPTURE( s3 );
    CAPTURE( str_to_hex( s3 ) );

    // Make sure there's no interference from e.g. uncleaned old runs
    std::string base = g->get_world_base_save_path() + "/fs_test_" +
                       get_pid_string() + "_" + to_string( serial ) + "/";
    REQUIRE( !dir_exist( base ) );
    REQUIRE( assure_dir_exist( base ) );
    REQUIRE( can_write_to_dir( base ) );

    std::string dir1 = base + s1 + "/";
    std::string file1_1 = dir1 + s2 + ".json";
    std::string file1_2 = dir1 + s3 + ".json";
    std::string dir2 = dir1 + s3 + "/";
    std::string file2_1 = dir2 + s2 + ".json";

    std::string writebuf = s3;
    std::string writebuf2 = s2;
    std::string readbuf;
    const auto reader = [&readbuf]( std::istream & s ) {
        s >> readbuf;
    };
    const auto writer = [&writebuf]( std::ostream & s ) {
        s << writebuf;
    };
    const auto writer2 = [&writebuf2]( std::ostream & s ) {
        s << writebuf2;
    };

    // Creating/removing directory; not confusing it with a file
    REQUIRE( !dir_exist( dir1 ) );
    REQUIRE( !file_exist( dir1 ) );
    REQUIRE( assure_dir_exist( dir1 ) );
    REQUIRE( dir_exist( dir1 ) );
    REQUIRE( !file_exist( dir1 ) );
    REQUIRE( remove_directory( dir1 ) );
    REQUIRE( !remove_directory( dir1 ) );
    REQUIRE( !dir_exist( dir1 ) );

    // Creating/reading file, renaming it, removing, not confusing with a directory
    REQUIRE( assure_dir_exist( dir1 ) );
    REQUIRE( !file_exist( file1_1 ) );
    REQUIRE( write_to_file( file1_1, writer, nullptr ) );
    REQUIRE( file_exist( file1_1 ) );
    REQUIRE( read_from_file( file1_1, reader ) );
    CHECK( readbuf == writebuf );
    REQUIRE( rename_file( file1_1, file1_2 ) );
    REQUIRE( !rename_file( file1_1, file1_2 ) );
    REQUIRE( file_exist( file1_2 ) );
    REQUIRE( !rename_file( file1_2, dir1 ) );
    REQUIRE( file_exist( file1_2 ) );
    REQUIRE( !dir_exist( file1_2 ) );
    REQUIRE( remove_file( file1_2 ) );
    REQUIRE( !remove_file( file1_2 ) );

    // Copying file
    REQUIRE( write_to_file( file1_1, writer, nullptr ) );
    REQUIRE( copy_file( file1_1, file1_2 ) );
    REQUIRE( file_exist( file1_2 ) );
    REQUIRE( copy_file( file1_1, file1_2 ) );
    REQUIRE( remove_file( file1_2 ) );
    REQUIRE( write_to_file( file1_2, writer2, nullptr ) );
    REQUIRE( copy_file( file1_2, file1_1 ) );
    REQUIRE( remove_file( file1_2 ) );
    REQUIRE( read_from_file( file1_1, reader ) );
    CHECK( readbuf == writebuf2 );
    REQUIRE( assure_dir_exist( dir2 ) );
    REQUIRE( !copy_file( file1_2, file1_1 ) );
    REQUIRE( !copy_file( file1_1, dir2 ) );
    REQUIRE( !copy_file( dir2, file1_2 ) );
    REQUIRE( remove_file( file1_1 ) );
    REQUIRE( remove_directory( dir2 ) );
    REQUIRE( !copy_file( file1_1, file1_2 ) );

    // Checking if can write to dir
    REQUIRE( can_write_to_dir( dir1 ) );
    REQUIRE( remove_directory( dir1 ) );
    REQUIRE( !can_write_to_dir( dir1 ) );

    // Listing files from dir; paths should stay valid utf-8 strings
    REQUIRE( assure_dir_exist( dir1 ) );
    REQUIRE( write_to_file( file1_1, writer, nullptr ) );
    REQUIRE( write_to_file( file1_2, writer, nullptr ) );
    REQUIRE( assure_dir_exist( dir2 ) );
    REQUIRE( write_to_file( file2_1, writer, nullptr ) );
    {
        std::vector<std::string> got = get_files_from_path(
                                           ".json",
                                           base,
                                           true,
                                           true
                                       );
        std::vector<std::string> exp = {
            file1_1,
            file1_2,
            file2_1,
        };
        const auto comparator = []( const std::string & a, const std::string & b ) {
            // We don't care about lexicographic order here, just the data order
            return a > b;
        };
        std::sort( exp.begin(), exp.end(), comparator );
        std::sort( got.begin(), got.end(), comparator );
        CHECK( got == exp );
    }

    // Can't delete directory with files
    REQUIRE( !remove_directory( dir1 ) );

    // Clean up
    REQUIRE( remove_file( file1_1 ) );
    REQUIRE( remove_file( file1_2 ) );
    REQUIRE( remove_file( file2_1 ) );
    REQUIRE( remove_directory( dir2 ) );
    REQUIRE( remove_directory( dir1 ) );
    REQUIRE( remove_directory( base ) );
}

TEST_CASE( "filesystem_ascii", "[filesystem]" )
{
    // English
    filesystem_test_group( 1, u8R"(DwarfFort)", u8R"(warf)", u8R"(fFor)" );
}

TEST_CASE( "filesystem_utf8", "[filesystem]" )
{
    // French
    filesystem_test_group( 2, u8R"(crème brûlée)", u8R"(rèm)", u8R"(rûlé)" );
    // Russian
    filesystem_test_group( 3, u8R"(МамаМылаРаму)", u8R"(амаМ)", u8R"(ылаР)" );
    // Chinese
    filesystem_test_group( 4, u8R"(你好)", u8R"(你)", u8R"(好)" );
    // Hindi (with diacritics)
    filesystem_test_group( 5, u8R"(नमस्ते)", u8R"(स्ते)", u8R"(मस्)" );
    // Let's spice it up a bit
    filesystem_test_group( 6, u8R"(Dw你ы)", u8R"(лаस्तेlé)", u8R"(मябस्or好)" );
}
