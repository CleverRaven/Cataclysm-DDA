#include "tgz_archiver.h"

#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <numeric>
#include <sstream>

#include "debug.h"
#include "filesystem.h"
#include "zlib.h"

tgz_archiver::~tgz_archiver()
{
    finalize();
}

std::string tgz_archiver::_gen_tar_header( fs::path const &file_name, fs::path const &prefix,
        fs::path const &real_path, std::streamsize size )
{
    unsigned const type = fs::is_directory( real_path ) ? 5 : 0;
    unsigned const perms = fs::is_directory( real_path ) ? 0775 : 0664;
    // https://stackoverflow.com/a/61067330
    std::time_t const mtime = fs::exists( real_path )
                              ? std::chrono::system_clock::to_time_t(
                                  std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                      fs::last_write_time( real_path ) - _fsnow + _sysnow ) ) : std::time_t{};

    std::string buf( tar_block_size, '\0' );
    std::ostringstream ss( buf );
    ss.imbue( std::locale::classic() );
    ss << file_name.generic_u8string();
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
    ss.seekp( 345 );
    ss << prefix.generic_u8string();
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

    std::size_t const archived_path_size = archived_path.generic_u8string().size();
    std::size_t const prefix_len = archived_path_size > 100 ? archived_path_size - 100 : 0;

    if( archived_path.filename().generic_u8string().size() > 100 ||
        prefix_len > 150 ) {
        DebugLog( DebugLevel::D_WARNING, DebugClass::D_MAIN )
                << "file skipped (path too long): " << archived_path << std::endl;
        return false;
    }

    fs::path prefix;
    fs::path file_name;
    if( prefix_len > 0 ) {
        std::size_t len = 0;
        for( fs::path const &it : archived_path ) {
            prefix /= it;
            len += it.generic_u8string().size() + 1;
            if( len >= prefix_len ) {
                break;
            }
        }
        file_name = archived_path.lexically_relative( prefix );
    } else {
        file_name = archived_path;
    }

    std::string header( tar_block_size, '\0' );
    std::streamsize size = 0;
    std::ifstream file( real_path, std::ios::binary | std::ios::ate );
    if( fs::is_regular_file( real_path ) ) {
        size = file.tellg();
        header = _gen_tar_header( file_name, prefix, real_path, size );
        file.seekg( 0 );
        file.clear();
    } else {
        header = _gen_tar_header( file_name, prefix, real_path, 0 );
    }
    bool ret = gzwrite( fd, header.c_str(), tar_block_size ) != 0;

    if( !ret || !fs::is_regular_file( real_path ) || size == 0 ) {
        return ret;
    }

    double const buf_size = std::ceil( static_cast<double>( size ) / tar_block_size ) * tar_block_size;
    std::string buf( static_cast<std::string::size_type>( buf_size ), '\0' );
    if( file.read( buf.data(), size ) ) {
        ret &= gzwrite( fd, buf.data(), buf.size() ) != 0;
    } else {
        debugmsg( "failed to read file %s", real_path.string() );
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
