#include "tgz_archiver.h"

#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <locale>
#include <numeric>
#include <sstream>

#include "filesystem.h"
#include "zlib.h"

tgz_archiver::~tgz_archiver()
{
    finalize();
}

std::string tgz_archiver::_gen_tar_header( char const *name,
        fs::directory_entry const &path )
{
    unsigned const size = path.is_regular_file() ? path.file_size() : 0;
    unsigned const type = path.is_directory() ? 5 : 0;
    unsigned const perms = path.is_directory() ? 0775 : 0664;
    // https://stackoverflow.com/a/61067330
    std::time_t const mtime = std::chrono::system_clock::to_time_t(
                                  std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                      path.last_write_time() - _fsnow + _sysnow ) );

    std::string buf( tar_block_size, '\0' );
    std::ostringstream ss( buf );
    ss.imbue( std::locale::classic() );
    ss << name;
    ss.seekp( 100 );
    ss << std::oct << std::setfill( '0' );
    ss << std::setw( 7 ) << perms << '\0';
    ss << std::setw( 7 ) << '0' << '\0';
    ss << std::setw( 7 ) << '0' << '\0';
    ss << std::setw( 11 ) << size << " ";
    ss << std::setw( 11 ) << mtime << " ";
    ss << "        ";
    ss << type;
    ss.seekp( 257 );
    ss << "ustar" << '\0' << "00";
    std::string const tmp = ss.str();
    unsigned const checksum = std::accumulate( tmp.cbegin(), tmp.cend(), 0 );
    ss.seekp( 148 );
    ss << std::setw( 6 ) << checksum << '\0' << ' ';
    return ss.str();
}

bool tgz_archiver::add_file( fs::path const &real_path, fs::path const &archived_path )
{
    if( fd == nullptr && ( fd = gzopen( output.c_str(), "wb" ), fd == nullptr ) ) {
        return false;
    }

    fs::directory_entry const de( real_path );
    std::string const header = _gen_tar_header( archived_path.generic_u8string().c_str(), de );
    bool ret = gzwrite( fd, header.c_str(), tar_block_size ) != 0;

    if( !ret || !de.is_regular_file() ) {
        return ret;
    }

    cata::ifstream file( real_path, std::ios::binary );
    while( !file.eof() && ret ) {
        tar_block_t buf{};
        file.read( buf.data(), buf.size() );
        if( file.gcount() > 0 ) {
            ret &= gzwrite( fd, buf.data(), buf.size() ) != 0;
        }
    }

    return ret;
}

void tgz_archiver::finalize()
{
    if( fd == nullptr ) {
        return;
    }
    std::array<char, tar_block_size * 2> const fin{};
    gzwrite( fd, fin.data(), fin.size() );
    gzclose_w( fd );
    fd = nullptr;
}
