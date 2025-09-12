#include <array>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <zstd/zstd.h>
#include <zstd/common/mem.h>
#include <zstd/common/xxhash.h>

// This is a c++ utility written like a oneshot python script.
// Do not consider code here to exemplify best practices.

std::string_view kZzipExt{ ".zzip" };
std::string_view kMapMemoryExt{ ".mm1" };
std::string_view kSavExt{ ".sav" };

std::string_view kMapsFolder{ "maps" };
std::string_view kOvermapsFolder{ "overmaps" };

std::string_view kMapsDict{ "maps.dict" };
std::string_view kOvermapsDict{ "overmaps.dict" };
std::string_view kMapMemoryDict{ "mmr.dict" };

constexpr unsigned int kEntryFileNameMagic = 0;
constexpr unsigned int kEntryChecksumMagic = 1;
constexpr unsigned int kFooterChecksumMagic = 15;

constexpr uint64_t kCheckumSeed = 0x1337C0DE;

struct IDK : public std::runtime_error {
    IDK( std::filesystem::path file ) : runtime_error{ "Don't know what to do with " + file.generic_u8string() } {}
};

struct ZstdError : public std::exception {
    ZstdError( const std::filesystem::path &context, size_t offset, size_t code,
               std::string message ) : offset{ offset }, code { code } {
        what_str = context.generic_u8string() + std::to_string( offset ) + ":" + ": " + message;
        if( ZSTD_isError( code ) ) {
            what_str = std::move( what_str ) + "\nZSTD error: " + ZSTD_getErrorString( ZSTD_getErrorCode(
                           code ) );
        }
    }

    virtual const char *what() const noexcept override {
        return what_str.c_str();
    }

    size_t offset;
    size_t code;
    std::string what_str;
};

struct FsError : public std::exception {
    FsError( const std::filesystem::path &context, std::error_code ec, std::string message ) : ec{ ec } {
        what_str = context.generic_u8string() + ": " + message;
        if( ec ) {
            what_str = std::move( what_str ) + "\nFS error: " + ec.message();
        }
    }

    virtual const char *what() const noexcept override {
        return what_str.c_str();
    }

    std::error_code ec;
    std::string what_str;
};

struct ZzipError : public std::exception {
    ZzipError( const std::filesystem::path &context, size_t offset, std::string message ) : offset{offset} {
        what_str = context.generic_u8string() + ":" + std::to_string( offset ) + ": " + message;
    }

    virtual const char *what() const noexcept override {
        return what_str.c_str();
    }

    size_t offset;
    std::string what_str;
};

static std::pair<std::filesystem::path, std::filesystem::path>
get_world_root_and_relative_content_path(
    std::filesystem::path const &content )
{
    std::filesystem::path absolute_content = content;
    std::error_code ec;
    if( !absolute_content.is_absolute() ) {
        absolute_content = std::filesystem::canonical( absolute_content, ec );
        if( ec ) {
            throw FsError( content, ec, "Unable to absolutize path." );
        }
    }

    std::filesystem::path world_root = absolute_content;

    // Walk the path looking for worldoptions.json.
    while( world_root.has_relative_path() &&
           !std::filesystem::exists( world_root / "worldoptions.json", ec ) && !ec ) {
        world_root = world_root.parent_path();
    }
    if( ec || !world_root.has_relative_path() ) {
        throw FsError( content, ec, "Unable to find worldoptions.json, cannot find world root folder." );
    }
    return std::pair{ world_root, absolute_content.lexically_relative( world_root ) };
}

struct Exn {
    virtual ~Exn() = default;
    [[noreturn]] virtual void throw_exception() = 0;
};

template<typename E>
struct ExnHolder : Exn {
    template<typename T = E>
    ExnHolder( T && e ) : e{std::forward<T>( e )} {}
    virtual ~ExnHolder() override = default;
    [[noreturn]] virtual void throw_exception() override {
        throw std::move( e );
    }

    E e;
};

struct ExnPtr {
    ExnPtr() = default;
    ExnPtr( ExnPtr && ) = default;

    template<typename E>
    ExnPtr( E &&e ) : ep{std::make_unique<ExnHolder<std::decay_t<E>>>( std::forward<E>( e ) )} {}

    operator bool() const {
        return static_cast<bool>( ep );
    }

    [[noreturn]] void throw_exception() {
        ep->throw_exception();
        // Some compiler's can't figure out the [[noreturn]] on Exn::throw_exception()
#if defined(__GNUC__) || defined(__clang__)
        __builtin_unreachable();
#elif defined(_MSC_VER)
        __assume( false );
#else
#error Unexpected compiler.
#endif
    }

    std::unique_ptr<Exn> ep;
};

static std::pair<const char *, ExnPtr> for_each_zzip_entry(
    std::filesystem::path zzip_path, const std::vector<char> &zzip_contents,
    std::function<ExnPtr( const std::string &, uint64_t, const char *, size_t )>
    fn )
{
    size_t remaining_len = zzip_contents.size();
    const char *base = zzip_contents.data();
    const char *end_of_last_entry = base;

#define RETURN_ERROR(e) return {end_of_last_entry, ExnPtr{e}}

    // Skip the zzip header.
    if( !ZSTD_isSkippableFrame( base, remaining_len ) ) {
        RETURN_ERROR( ZzipError( zzip_path, 0, "Zzip does not start with skippable frame." ) );
    }

    // The first frame of a zzip should be a skippable frame of two 64 bit ints
    ZSTD_FrameHeader header;
    size_t error_code = ZSTD_getFrameHeader( &header, base, remaining_len );
    if( ZSTD_isError( error_code ) ||
        header.dictID != kFooterChecksumMagic ||
        header.frameContentSize != 2 * sizeof( uint64_t ) ) {
        RETURN_ERROR( ZzipError( zzip_path, 0, "Error reading zzip header." ) );
    }

    base += ZSTD_SKIPPABLEHEADERSIZE + 2 * sizeof( uint64_t );
    remaining_len -= ZSTD_SKIPPABLEHEADERSIZE + 2 * sizeof( uint64_t );

    std::string filename;
    uint64_t checksum = 0;

    while( remaining_len > 0 && ZSTD_isFrame( base, remaining_len ) ) {
        size_t offset = zzip_contents.size() - remaining_len;
        size_t frame_size = ZSTD_findFrameCompressedSize( base, remaining_len );
        if( ZSTD_isError( frame_size ) ) {
            RETURN_ERROR( ZstdError( zzip_path, offset, frame_size, "Error reading frame size." ) );
        }
        if( frame_size > remaining_len ) {
            RETURN_ERROR( ZzipError( zzip_path, offset, "Frame size exceeds remaining file" ) );
        }
        if( ZSTD_isSkippableFrame( base, remaining_len ) ) {
            error_code = ZSTD_getFrameHeader( &header, base, remaining_len );
            if( error_code ) {
                RETURN_ERROR( ZstdError( zzip_path, offset, error_code, "Error reading frame header." ) );
            }
            if( header.dictID == kEntryFileNameMagic ) {
                filename.resize( header.frameContentSize );
                error_code = ZSTD_readSkippableFrame( filename.data(), filename.size(), nullptr, base,
                                                      remaining_len );
                if( ZSTD_isError( error_code ) || error_code != header.frameContentSize ) {
                    RETURN_ERROR( ZstdError( zzip_path, offset, error_code, "Error reading filename header." ) );
                }
            }
            if( header.dictID == kEntryChecksumMagic ) {
                uint64_t raw_checksum;
                error_code = ZSTD_readSkippableFrame( &raw_checksum, sizeof( checksum ), nullptr, base,
                                                      remaining_len );
                if( ZSTD_isError( error_code ) || error_code != sizeof( uint64_t ) ) {
                    RETURN_ERROR( ZstdError( zzip_path, offset, error_code, "Error reading checksum." ) );
                }
                checksum = MEM_readLE64( &raw_checksum );
            }
            base += frame_size;
            remaining_len -= frame_size;
            continue;
        }

        // At this point we should have read the filename and compressed frame checksums.
        // If either is missing, the end of the zzip is corrupt.
        if( filename.empty() || checksum == 0 ) {
            RETURN_ERROR( ZzipError( zzip_path, offset, "Entry metadata missing." ) );
        }

        if( frame_size > remaining_len ) {
            RETURN_ERROR( ZzipError( zzip_path, offset, filename + ": compressed contents truncated." ) );
        }

        uint64_t computed_checksum = XXH64( base, frame_size, kCheckumSeed );
        if( checksum != computed_checksum ) {
            RETURN_ERROR( ZzipError( zzip_path, offset, filename + ": compressed frame corrupted." ) );
        }
        ExnPtr err = fn( filename, checksum, base, frame_size );
        if( err ) {
            return { end_of_last_entry, std::move( err ) };
        }

        filename.clear();
        checksum = 0;
        base += frame_size;
        remaining_len -= frame_size;
        end_of_last_entry = base;
    }
    if( !filename.empty() || checksum != 0 ) {
        RETURN_ERROR( ZzipError( zzip_path, zzip_contents.size() - remaining_len,
                                 "Expected a compressed frame after entry metadata." ) );
    }
    return { end_of_last_entry, nullptr };
#undef RETURN_ERROR
}

static int decompress_to( std::filesystem::path const &zzip_path,
                          std::filesystem::path output_folder,
                          std::filesystem::path const &dict )
{
    std::cout << "Decompressing " << zzip_path.generic_u8string() << " -> " <<
              output_folder.generic_u8string() << std::endl;
    std::string zzip_path_string = zzip_path.generic_u8string();

    std::error_code ec;
    if( !std::filesystem::is_directory( output_folder ) &&
        ( !std::filesystem::create_directories( output_folder, ec ) || ec ) ) {
        throw FsError( output_folder, ec, "Unable to create output folder." );
    }

    if( !std::filesystem::is_regular_file( zzip_path, ec ) || ec ) {
        throw FsError( zzip_path, ec, "Not a file." );
    }
    std::ifstream zip{ zzip_path, std::ios::binary };
    if( !zip ) {
        throw std::runtime_error( zzip_path_string + ": Unable to open file." );
    }

    zip.seekg( 0, std::ios_base::end );
    std::streamoff zzip_size = zip.tellg();
    zip.seekg( 0 );
    std::vector<char> zzip_contents;
    zzip_contents.resize( zzip_size );
    zip.read( zzip_contents.data(), zzip_size );

    ZSTD_DCtx *ctx = ZSTD_createDCtx();

    std::vector<char> dictionary;
    if( !dict.empty() ) {
        std::ifstream d{ dict, std::ios::binary };
        if( d ) {
            d.seekg( 0, std::ios_base::end );
            std::streamoff dict_size = d.tellg();
            d.seekg( 0 );

            dictionary.resize( dict_size );
            d.read( dictionary.data(), dict_size );
            ZSTD_DCtx_loadDictionary_byReference( ctx, dictionary.data(), dictionary.size() );
        } else {
            throw std::runtime_error( dict.generic_u8string() + ": Cannot open dictionary." );
        }
    }

    // Iterate through the zstd frames extracting each file as we get to it.
    auto [after_last_entry_ptr, err] = for_each_zzip_entry(
                                           zzip_path,
                                           zzip_contents,
                                           [&](
                                                   const std::string & filename,
                                                   uint64_t /*checksum*/,
                                                   const char *entry_base,
                                                   size_t frame_size
    ) -> ExnPtr {
#define RETURN_ERROR(e) return ExnPtr{e}
        std::cout << zzip_path_string << ": " << filename << " -> " << ( output_folder / filename ).generic_u8string() <<
                  std::endl;

        unsigned long long file_size = ZSTD_decompressBound( entry_base, frame_size );
        std::vector<char> buf;
        buf.resize( file_size );
        size_t actual = ZSTD_decompressDCtx( ctx, buf.data(), file_size, entry_base, frame_size );
        if( ZSTD_isError( actual ) )
        {
            size_t offset = zzip_contents.data() - entry_base;
            RETURN_ERROR( ZstdError( zzip_path, offset, actual, "Decompression error." ) );
        }

        std::filesystem::path output_file = output_folder / filename;
        std::ofstream out{ output_file, std::ios::binary };
        if( !out )
        {
            RETURN_ERROR( FsError( output_file, {}, "Unable to open output file." ) );
        }
        try
        {
            out.write( buf.data(), actual );
        } catch( std::exception &e )
        {
            RETURN_ERROR( FsError( output_file, {}, std::string{ "Error writing output: " } + e.what() ) );
        }
        return {};
#undef RETURN_ERROR
    } );
    if( err ) {
        err.throw_exception();
    }

    return 0;
}

static int compress_to( std::filesystem::path const &zzip_path,
                        std::filesystem::path archive_filename,
                        std::filesystem::path input_file, std::filesystem::path const &dict )
{
    std::cout << "Compressing " << input_file.generic_u8string() << " -> " <<
              zzip_path.generic_u8string() << ":" << archive_filename.generic_u8string() <<
              std::endl;
    std::filesystem::path output_folder = zzip_path.parent_path();

    std::error_code ec;
    if( !std::filesystem::is_directory( output_folder ) &&
        ( !std::filesystem::create_directories( output_folder, ec ) || ec ) ) {
        throw FsError( output_folder, ec, "Unable to create output folder." );
    }

    std::string input_path_string = input_file.generic_u8string();
    std::vector<char> input_contents;
    {
        std::ifstream input{ input_path_string, std::ios::binary };
        if( !input ) {
            throw std::runtime_error( input_path_string + ": Unable to open input file." );
        }
        input.seekg( 0, std::ios_base::end );
        std::streamoff input_size = input.tellg();
        input.seekg( 0 );
        input_contents.resize( input_size );
        input.read( input_contents.data(), input_size );
    }

    std::string zzip_path_string = zzip_path.generic_u8string();
    std::vector<char> zzip_contents;
    bool zzip_exists = false;
    if( std::filesystem::is_regular_file( zzip_path, ec ) ) {
        zzip_exists = true;
        std::ifstream zip{ zzip_path, std::ios::binary };
        if( !zip ) {
            throw std::runtime_error( zzip_path_string + ": Unable to open zzip." );
        }
        zip.seekg( 0, std::ios_base::end );
        std::streamoff zzip_size = zip.tellg();
        zip.seekg( 0 );
        zzip_contents.resize( zzip_size );
        zip.read( zzip_contents.data(), zzip_size );
    } else if( std::filesystem::exists( zzip_path, ec ) || ec ) {
        throw FsError( zzip_path, ec, "Output is not a writeable file." );
    }

    ZSTD_CCtx *ctx = ZSTD_createCCtx();

    std::vector<char> dictionary;
    if( !dict.empty() ) {
        std::ifstream d{ dict, std::ios::binary };
        if( d ) {
            d.seekg( 0, std::ios_base::end );
            std::streamoff dict_size = d.tellg();
            d.seekg( 0 );

            dictionary.resize( dict_size );
            d.read( dictionary.data(), dict_size );
            ZSTD_CCtx_loadDictionary_byReference( ctx, dictionary.data(), dictionary.size() );
        } else {
            throw std::runtime_error( dict.generic_u8string() + ": Cannot open dictionary." );
        }
    }


    auto [after_last_entry_ptr, err] = for_each_zzip_entry(
                                           zzip_path,
                                           zzip_contents,
                                           [&](
                                                   const std::string & entry_filename,
                                                   uint64_t /*checksum*/,
                                                   const char * /*entry_base*/,
                                                   size_t /*zzip_remaining*/
    ) -> ExnPtr {
        std::cout << zzip_path.generic_u8string() << ":" << entry_filename << std::endl;
        return {};
    } );

    size_t write_offset = after_last_entry_ptr - zzip_contents.data();

    // We have to add the `in` bit to avoid truncating the file, if it exists
    // But if it does not exist, we must omit it to create a new file.
    // Who designed this api.
    std::ofstream zip{ zzip_path, zzip_exists ? std::ios::binary | std::ios::in : std::ios::binary};
    if( !zip ) {
        throw std::runtime_error( zzip_path_string + ": Unable to open zzip for writing." );
    }

    // We are invalidating the current footer but may not overwrite it,
    // so overwrite the header instead.
    // This has the advantage of also working for freshly created zzips.
    std::vector<char> buf;
    buf.resize( ZSTD_SKIPPABLEHEADERSIZE + 2 * sizeof( uint64_t ) );

    std::array<uint64_t, 2> dummy{ 0, 0 };
    MEM_writeLE64( &dummy[1], XXH64( nullptr, 0,
                                     kCheckumSeed ) + 1 ); // Write an intentionally incorrect checksum.
    size_t header_size = ZSTD_writeSkippableFrame(
                             buf.data(),
                             buf.size(),
                             dummy.data(),
                             2 * sizeof( uint64_t ),
                             kFooterChecksumMagic
                         );
    if( ZSTD_isError( header_size ) ) {
        throw std::runtime_error( "How did this fail to write the header to a buffer?" );
    }
    zip.write( buf.data(), header_size );

    // Fast forward to where we need to write the new entry.
    if( write_offset == 0 ) {
        // Brand new zzip, skip to after the header.
        write_offset = header_size;
    }
    zip.seekp( write_offset );

    // Compress the file.
    size_t compress_bound = ZSTD_compressBound( input_contents.size() );
    buf.resize( compress_bound );
    size_t file_frame_size = ZSTD_compress2(
                                 ctx,
                                 buf.data(),
                                 buf.size(),
                                 input_contents.data(),
                                 input_contents.size()
                             );
    if( ZSTD_isError( file_frame_size ) ) {
        throw ZstdError( input_file, 0, file_frame_size, "Failed to compress input file." );
    }
    buf.resize( file_frame_size );
    // Write the filename frame
    {
        std::string filename_string = archive_filename.generic_u8string();
        std::vector<char> buf2;
        buf2.resize( ZSTD_SKIPPABLEHEADERSIZE + filename_string.size() );

        size_t filename_header_size = ZSTD_writeSkippableFrame(
                                          buf2.data(),
                                          buf2.size(),
                                          filename_string.data(),
                                          filename_string.size(),
                                          kEntryFileNameMagic
                                      );
        if( ZSTD_isError( filename_header_size ) ) {
            throw std::runtime_error( "How did this fail to write the filename header to a buffer?" );
        }
        zip.write( buf2.data(), filename_header_size );
    }

    // Then the checksum frame.
    {
        uint64_t checksum = XXH64( buf.data(), buf.size(), kCheckumSeed );
        std::vector<char> buf2;
        buf2.resize( ZSTD_SKIPPABLEHEADERSIZE + sizeof( uint64_t ) );
        uint64_t checksum_le = 0;
        MEM_writeLE64( &checksum_le, checksum );
        size_t checksum_header_size = ZSTD_writeSkippableFrame(
                                          buf2.data(),
                                          buf2.size(),
                                          reinterpret_cast<const char *>( &checksum_le ),
                                          sizeof( checksum_le ),
                                          kEntryChecksumMagic
                                      );
        zip.write( buf2.data(), checksum_header_size );
    }
    // And finally the compressed file.
    zip.write( buf.data(), buf.size() );

    return 0;
}

static std::filesystem::path assert_exists( std::filesystem::path f )
{
    if( !std::filesystem::exists( f ) ) {
        throw std::runtime_error( "Unable to find " + f.generic_u8string() );
    }
    return f;
}

static int handle_file_contextual( std::filesystem::path const &input_file )
{
    auto [world_root, relative_input] = get_world_root_and_relative_content_path( input_file );
    std::vector<std::filesystem::path> parts;
    for( std::filesystem::path part : relative_input ) {
        parts.emplace_back( part );
    }

    std::filesystem::path dict = [world_root_ = world_root, &parts]() -> std::filesystem::path {
        if( parts[0] == kMapsFolder )
        {
            return assert_exists( world_root_ / kMapsDict );
        }
        if( parts[0] == kOvermapsFolder || parts[0].generic_u8string().find( "o." ) == 0 )
        {
            return assert_exists( world_root_ / kOvermapsDict );
        }
        if( parts[0].extension() == kMapMemoryExt )
        {
            return assert_exists( world_root_ / kMapMemoryDict );
        }
        return {};
    }();

    if( parts.back().extension() == kZzipExt ) {
        // Currently only support maps/, overmaps/, char.mm1/, and char.sav.zzip
        if( parts.size() > 2 ) {
            throw IDK( input_file );
        }

        std::filesystem::path world_relative_output_folder;
        if( parts[0] == kMapsFolder ) {
            // maps/x.y.z.zzip[a.b.c.map] -> maps/x.y.z/a.b.c.map
            world_relative_output_folder = relative_input;
            world_relative_output_folder.replace_extension( "" );
        }
        if( parts[0] == kOvermapsFolder ) {
            // overmaps/o.x.y.zzip[o.x.y] -> o.x.y
            world_relative_output_folder = "";
        }
        if( parts[0].extension() == kMapMemoryExt ) {
            // char.mm1/char.*.zzip[a.b.c.mmr] -> char.mm1/a.b.c.mmr
            world_relative_output_folder = parts[0];
        }

        return decompress_to( world_root / relative_input, world_root / world_relative_output_folder,
                              dict );
    }

    std::filesystem::path entry_key;
    std::filesystem::path world_relative_zzip;

    if( parts[0] == kMapsFolder ) {
        if( parts.size() != 3 || parts[2].extension() != ".map" ) {
            throw IDK( input_file );
        }
        // maps/x.y.z/a.b.c.map -> maps/x.y.z.zzip[a.b.c.map]
        entry_key = parts[2];
        world_relative_zzip = kMapsFolder / parts[1];
    }
    if( parts[0].extension() == kMapMemoryExt ) {
        if( parts.size() != 2 || parts[1].extension() != ".mmr" ) {
            throw IDK( input_file );
        }
        // char.mm1/a.b.c.mmr -> char.mm1/char.mm1.cold.zzip[a.b.c.mmr]
        entry_key = parts[1];
        // We have to break this up to not mutate parts[0] in an undefined order by calling .concat
        world_relative_zzip = parts[0];
        world_relative_zzip /= parts[0];
        world_relative_zzip += ".cold";
    }
    if( entry_key.empty() && parts.size() > 1 ) {
        throw IDK( input_file );
    }
    if( parts[0].extension() == kSavExt ) {
        // char.sav -> char.sav.zzip[char.sav]
        entry_key = parts[0];
        world_relative_zzip = parts[0];
    }
    if( parts[0].generic_u8string().find( "o." ) == 0 ) {
        // o.x.y -> overmaps/o.x.y.zzip[o.x.y]
        entry_key = parts[0];
        world_relative_zzip = kOvermapsFolder / parts[0];
    }

    if( entry_key.empty() ) {
        throw IDK( input_file );
    }

    world_relative_zzip.concat( ".zzip" );
    return compress_to( world_root / world_relative_zzip, entry_key, world_root / relative_input,
                        dict );
}

int main( int argc, char **argv )
{
    if( argc == 2 ) {
        // If there's exactly one argument, it's 'something' that we need to deal with.
        // For example someone drag and dropping something onto the binary.
        // Use heuristics to figure out what to do.
        // For now we only support files, not folders.
        std::filesystem::path input_file{ argv[1] };
        try {
            if( !std::filesystem::is_regular_file( input_file ) ) {
                throw IDK( input_file );
            }
            return handle_file_contextual( input_file );
        } catch( const std::exception &e ) {
            std::cerr << "Unable to handle " << input_file << std::endl;
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }
    return 0;
}
