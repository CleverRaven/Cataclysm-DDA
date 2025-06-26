#include "zzip.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <flatbuffers/flexbuffers.h>

#include <zstd/zstd.h>
#include <zstd/zstd_errors.h>
#include <zstd/common/mem.h>
#include <zstd/common/xxhash.h>

#include "cata_scope_helpers.h"
#include "filesystem.h"
#include "flexbuffer_cache.h"
#include "flexbuffer_json.h"
#include "mmap_file.h"
#include "std_hash_fs_path.h"

namespace
{

// Implementations of the mandatory Json helper types
// to use flexbuffers/JsonObject with zzips.

struct zzip_flexbuffer_storage : flexbuffer_storage {
    std::shared_ptr<mmap_file> mmap_handle_;

    explicit zzip_flexbuffer_storage( std::shared_ptr<mmap_file> mmap_handle )
        : mmap_handle_{ std::move( mmap_handle ) } {
    }

    const uint8_t *data() const override {
        return static_cast<const uint8_t *>( mmap_handle_->base() );
    }
    size_t size() const override {
        return mmap_handle_->len();
    }
};

struct zzip_flexbuffer : parsed_flexbuffer {
    explicit zzip_flexbuffer( std::shared_ptr<flexbuffer_storage> storage )
        : parsed_flexbuffer( std::move( storage ) ) {
    }

    bool is_stale() const override {
        return false;
    }

    std::unique_ptr<std::istream> get_source_stream() const noexcept( false ) override {
        return nullptr;
    }

    std::filesystem::path get_source_path() const noexcept override {
        return {};
    }
};

struct zzip_vector_storage : flexbuffer_storage {
    std::vector<std::byte> buffer_;

    explicit zzip_vector_storage( std::vector<std::byte> &&buffer )
        : buffer_{ std::move( buffer ) } {
    }

    const uint8_t *data() const override {
        return reinterpret_cast<const uint8_t *>( buffer_.data() );
    }

    size_t size() const override {
        return buffer_.size();
    }
};

// To save time, and because we aren't multithreading, we can cache zstd
// compress and decompress contexts, indexed by dictionary path.
struct cached_zstd_context {
    std::vector<char> dictionary_;
    ZSTD_CCtx *cctx = nullptr;
    ZSTD_DCtx *dctx = nullptr;

    cached_zstd_context(
        std::vector<char> &&dictionary,
        ZSTD_CCtx *cctx,
        ZSTD_DCtx *dctx )
        : dictionary_{std::move( dictionary )},
          cctx{ cctx },
          dctx{ dctx } {
    }

    cached_zstd_context( cached_zstd_context const & ) = delete;
    cached_zstd_context( cached_zstd_context &&rhs ) noexcept : dictionary_( std::move(
                    rhs.dictionary_ ) ), cctx( rhs.cctx ), dctx( rhs.dctx ) {
        rhs.cctx = nullptr;
        rhs.dctx = nullptr;
    }

    cached_zstd_context &operator=( cached_zstd_context const & ) = delete;
    cached_zstd_context &operator=( cached_zstd_context &&rhs ) noexcept {
        dictionary_ = std::move( rhs.dictionary_ );
        if( &rhs == this ) {
            return *this;
        }
        if( cctx ) {
            ZSTD_freeCCtx( cctx );
        }
        cctx = rhs.cctx;
        rhs.cctx = nullptr;

        if( dctx ) {
            ZSTD_freeDCtx( dctx );
        }
        dctx = rhs.dctx;
        rhs.dctx = nullptr;
        return *this;
    }

    ~cached_zstd_context() {
        ZSTD_freeCCtx( cctx );
        ZSTD_freeDCtx( dctx );
    }
};

std::unordered_map<std::string, cached_zstd_context> cached_contexts;

} // namespace

struct zzip::compressed_entry {
    std::string path;
    size_t offset;
    size_t len;
};

namespace
{

constexpr unsigned int kEntryFileNameMagic = 0;
constexpr unsigned int kEntryChecksumMagic = 1;
constexpr unsigned int kFooterChecksumMagic = 15;

constexpr size_t kFooterChecksumContentSize = 2 * sizeof( uint64_t );
constexpr size_t kFooterChecksumFrameSize = ZSTD_SKIPPABLEHEADERSIZE + kFooterChecksumContentSize;
constexpr size_t kEntryChecksumFrameSize = ZSTD_SKIPPABLEHEADERSIZE + sizeof( uint64_t );
constexpr size_t kDefaultFooterSize = 1024;

constexpr size_t kFixedSizeOverhead = kFooterChecksumFrameSize + kDefaultFooterSize;

constexpr uint64_t kCheckumSeed = 0x1337C0DE;

constexpr const std::string_view kEntriesKey = "entries";
constexpr const std::string_view kEntryOffsetKey = "offset";
constexpr const std::string_view kEntryLenKey = "len";

constexpr const std::string_view kMetaKey = "meta";
constexpr const std::string_view kMetaContentEndKey = "content_end";
constexpr const std::string_view kMetaTotalContentSizeKey = "total_content_size";

constexpr size_t kAssumedPageSize = 4 * 1024;

struct zzip_file_entry {
    size_t offset = 0;
    size_t len = 0;
};

struct zzip_meta {
    size_t content_end = 0;
    size_t total_content_size = 0;
};


// A helper class for safely and consistently accessing metadata from the footer index.
struct zzip_footer {
    const JsonObject &footer_;

    explicit zzip_footer( const JsonObject &jsobj )
        : footer_( jsobj ) {
    }

    zzip_meta get_meta() const {
        JsonObject meta_obj = footer_.get_object( kMetaKey );
        meta_obj.allow_omitted_members();
        size_t content_end = meta_obj.get_int( kMetaContentEndKey );
        size_t total_content_size = meta_obj.get_int( kMetaTotalContentSizeKey );
        return zzip_meta{ content_end, total_content_size };
    }

    std::optional<zzip_file_entry> get_entry( std::filesystem::path const &path ) const {
        std::optional<zzip_file_entry> entry;

        if( !footer_.has_object( kEntriesKey ) ) {
            return entry;
        }

        JsonObject entries = footer_.get_object( kEntriesKey );
        entries.allow_omitted_members();

        std::string path_str = path.generic_u8string();
        std::optional<JsonValue> entry_opt = entries.get_member_opt( path_str );
        if( entry_opt.has_value() ) {
            JsonObject entry_obj = *entry_opt;
            entry_obj.allow_omitted_members();
            size_t offset = entry_obj.get_int( kEntryOffsetKey );
            size_t len = entry_obj.get_int( kEntryLenKey );
            entry.emplace( zzip_file_entry{ offset, len } );
        }

        return entry;
    }

    std::vector<zzip::compressed_entry> get_entries() const {
        std::vector<zzip::compressed_entry> compressed_entries;

        if( footer_.has_object( kEntriesKey ) ) {
            JsonObject json_entries = footer_.get_object( kEntriesKey );
            compressed_entries.reserve( json_entries.size() );

            for( const JsonMember &entry : json_entries ) {
                JsonObject entry_object = entry.get_object();
                compressed_entries.emplace_back( zzip::compressed_entry{
                    entry.name(),
                    static_cast<size_t>( entry_object.get_int( kEntryOffsetKey ) ),
                    static_cast<size_t>( entry_object.get_int( kEntryLenKey ) )
                } );
            }
        }
        return compressed_entries;
    }
};

std::pair<uint64_t, uint64_t> read_footer_len_and_checksum( void *base, size_t len )
{
    std::array<uint64_t, 2> buf;

    unsigned int magic = -1;
    size_t bytes_read = ZSTD_readSkippableFrame(
                            buf.data(),
                            kFooterChecksumContentSize,
                            &magic,
                            base,
                            len );

    if( ZSTD_isError( bytes_read ) || magic != kFooterChecksumMagic ||
        bytes_read != kFooterChecksumContentSize ) {
        return { 0, 0 };
    }

    uint64_t footer_len = MEM_readLE64( &buf[0] );
    uint64_t footer_checksum = MEM_readLE64( &buf[1] );

    return { footer_len, footer_checksum };
}

bool write_footer_len_and_checksum( void *base, uint64_t frame_len, uint64_t footer_len,
                                    uint64_t footer_checksum )
{
    std::array<uint64_t, 2> buf;

    MEM_writeLE64( &buf[0], footer_len );
    MEM_writeLE64( &buf[1], footer_checksum );

    size_t footer_frame_size = ZSTD_writeSkippableFrame(
                                   base,
                                   frame_len,
                                   buf.data(),
                                   kFooterChecksumContentSize,
                                   kFooterChecksumMagic );
    if( footer_frame_size != kFooterChecksumFrameSize ) {
        return false;
    }

    return true;
}

// Given a pointer and length of an encoded entry, reads or skips the metadata
// and returns a pointer to the beginning of any content after the metadata frames.
// Returns { nullptr, 0 } on any detected errors.
std::pair<void *, size_t> read_and_skip_entry_metadata(
    void *entry_base,
    size_t entry_len,
    std::optional<std::string> *filename_out = nullptr,
    std::optional<uint64_t> *checksum_out = nullptr )
{
    char *base = static_cast<char *>( entry_base );
    while( entry_len > 0 && ZSTD_isSkippableFrame( base, entry_len ) ) {
        ZSTD_FrameHeader header;
        if( ZSTD_getFrameHeader( &header, base, entry_len ) == 0 ) {
            if( filename_out && header.dictID == kEntryFileNameMagic ) {
                std::string filename;
                filename.resize( header.frameContentSize );
                size_t ec = ZSTD_readSkippableFrame( filename.data(), filename.size(), nullptr, base, entry_len );
                if( ZSTD_isError( ec ) || ec != header.frameContentSize ) {
                    return { nullptr, 0 };
                }
                filename_out->emplace( std::move( filename ) );
            }
            if( checksum_out && header.dictID == kEntryChecksumMagic ) {
                uint64_t checksum;
                size_t ec = ZSTD_readSkippableFrame( &checksum, sizeof( checksum ), nullptr, base, entry_len );
                if( ZSTD_isError( ec ) || ec != sizeof( uint64_t ) ) {
                    return { nullptr, 0 };
                }
                checksum_out->emplace( MEM_readLE64( &checksum ) );
            }
        } else {
            return { nullptr, 0 };
        }

        size_t skip_offset = ZSTD_findFrameCompressedSize( base, entry_len );
        if( skip_offset > entry_len ) {
            return { nullptr, 0 };
        }
        base += skip_offset;
        entry_len -= skip_offset;
    }
    return { base, entry_len };
}

} // namespace

struct zzip::context {
    context( ZSTD_CCtx *cctx, ZSTD_DCtx *dctx )
        : cctx{ cctx }, dctx{ dctx }
    {}
    ZSTD_CCtx *cctx;
    ZSTD_DCtx *dctx;
};

zzip::zzip( std::filesystem::path path, std::shared_ptr<mmap_file> file, JsonObject footer )
    : path_{ std::move( path ) },
      file_{ std::move( file ) },
      footer_{ std::move( footer ) }
{}

zzip::~zzip() noexcept
{
    footer_.allow_omitted_members();
}

std::shared_ptr<zzip> zzip::load( std::filesystem::path const &path,
                                  std::filesystem::path const &dictionary_path )
{
    std::shared_ptr<zzip> zip;
    std::shared_ptr<mmap_file> file = mmap_file::map_writeable_file( path );

    flexbuffers::Reference root;
    std::shared_ptr<parsed_flexbuffer> flexbuffer;
    JsonObject footer;
    bool needs_footer = false;

    uint64_t footer_len = 0;
    uint64_t footer_checksum = 0;
    std::tie( footer_len, footer_checksum ) = read_footer_len_and_checksum( file->base(), file->len() );
    if( footer_len == 0 || footer_len > file->len() ) {
        needs_footer = true;
    } else {
        char *base = reinterpret_cast<char *>( file->base() );
        uint64_t hash = XXH64( base + file->len() - footer_len, footer_len, kCheckumSeed );
        if( hash != footer_checksum ) {
            needs_footer = true;
        }
        try {
            std::shared_ptr<zzip_flexbuffer_storage> storage{ std::make_shared<zzip_flexbuffer_storage>( file ) };

            root = flexbuffer_root_from_storage( storage );
            flexbuffer = std::make_shared<zzip_flexbuffer>( std::move( storage ) );
            footer = JsonValue( std::move( flexbuffer ), root, nullptr, 0 );
        } catch( std::exception &e ) {
            // Something went wrong. Try to recover the file.
            needs_footer = true;
        }
    }

    zip = std::shared_ptr<zzip>( new zzip( path, std::move( file ), std::move( footer ) ) );

    ZSTD_CCtx *cctx;
    ZSTD_DCtx *dctx;
    if( dictionary_path.empty() ) {
        cctx = ZSTD_createCCtx();
        dctx = ZSTD_createDCtx();
    } else if( auto it = cached_contexts.find( dictionary_path.string() );
               it != cached_contexts.end() ) {
        cctx = it->second.cctx;
        dctx = it->second.dctx;
    } else {
        cctx = ZSTD_createCCtx();
        ZSTD_CCtx_setParameter( cctx, ZSTD_c_compressionLevel, 7 );
        dctx = ZSTD_createDCtx();

        std::vector<char> dictionary;
        if( !dictionary_path.empty() ) {
            std::shared_ptr<const mmap_file> dictionary_file = mmap_file::map_file( dictionary_path );
            dictionary.resize( dictionary_file->len() );
            memcpy( dictionary.data(), dictionary_file->base(), dictionary_file->len() );
            ZSTD_CCtx_loadDictionary_byReference( cctx, dictionary.data(), dictionary.size() );
            ZSTD_DCtx_loadDictionary_byReference( dctx, dictionary.data(), dictionary.size() );
        }

        cached_contexts.emplace( dictionary_path.string(), cached_zstd_context{ std::move( dictionary ), cctx, dctx } );
    }

    zip->ctx_ = std::make_unique<zzip::context>( cctx, dctx );

    if( needs_footer && !zip->rewrite_footer() ) {
        return nullptr;
    }

    return zip;
}

bool zzip::add_file( std::filesystem::path const &zzip_relative_path, std::string_view content )
{
    size_t estimated_size = ZSTD_compressBound( content.length() );

    JsonObject footer_copy = copy_footer();
    footer_copy.allow_omitted_members();
    zzip_footer footer{ footer_copy };

    std::optional<zzip_meta> meta_opt = footer.get_meta();
    size_t old_content_end = 0;
    if( meta_opt.has_value() ) {
        old_content_end = meta_opt->content_end;
    }

    std::string relative_path_string = zzip_relative_path.generic_u8string();

    if( !ensure_capacity_for(
            old_content_end +
            ZSTD_SKIPPABLEHEADERSIZE + relative_path_string.length() +
            kEntryChecksumFrameSize +
            estimated_size +
            kFixedSizeOverhead ) ) {
        return false;
    }

    size_t final_size = write_file_at( relative_path_string, content, old_content_end );

    if( ZSTD_isError( final_size ) ) {
        return false;
    }

    std::vector<compressed_entry> new_entry;
    new_entry.emplace_back( zzip::compressed_entry{
        std::move( relative_path_string ),
        old_content_end,
        final_size
    } );

    if( !update_footer( footer_copy, old_content_end + final_size, new_entry ) ) {
        return false;
    }

    return true;
}


bool zzip::copy_files( std::vector<std::filesystem::path> const &zzip_relative_paths,
                       std::shared_ptr<zzip> const &from )
{
    zzip_footer other_footer{ from->footer_ };
    JsonObject original_footer = copy_footer();
    original_footer.allow_omitted_members();
    std::vector<compressed_entry> other_entries = other_footer.get_entries();
    std::unordered_map<std::string, int> entry_index_map;
    int idx = 0;
    for( const compressed_entry &other_entry : other_entries ) {
        entry_index_map.emplace( other_entry.path, idx++ );
    }
    std::vector<compressed_entry> entries_to_copy;
    entries_to_copy.reserve( zzip_relative_paths.size() );
    size_t required_size = 0;
    size_t new_content_end = zzip_footer{ footer_ }.get_meta().content_end;
    for( const std::filesystem::path &zzip_relative_path : zzip_relative_paths ) {
        auto it = entry_index_map.find( zzip_relative_path.u8string() );
        if( it == entry_index_map.end() ) {
            return false;
        }
        entries_to_copy.emplace_back( other_entries[it->second] );
        required_size += entries_to_copy.back().len;
    }
    ensure_capacity_for( new_content_end + required_size + kDefaultFooterSize );
    for( compressed_entry &entry_to_copy : entries_to_copy ) {
        memcpy( file_base_plus( new_content_end ), from->file_base_plus( entry_to_copy.offset ),
                entry_to_copy.len );
        entry_to_copy.offset = new_content_end;
        new_content_end += entry_to_copy.len;
    }
    return update_footer( original_footer, new_content_end, entries_to_copy );
}


bool zzip::has_file( std::filesystem::path const &zzip_relative_path ) const
{
    zzip_footer footer{ footer_ };
    std::optional<zzip_file_entry> fparams = footer.get_entry( zzip_relative_path );
    return fparams.has_value();
}

size_t zzip::get_file_size( std::filesystem::path const &zzip_relative_path ) const
{
    zzip_footer footer{ footer_ };
    std::optional<zzip_file_entry> fparams = footer.get_entry( zzip_relative_path );
    if( !fparams.has_value() ) {
        return 0;
    }
    void *base = file_base_plus( fparams->offset );
    size_t len = fparams->len;
    std::tie( base, len ) = read_and_skip_entry_metadata( base, len );
    size_t size = ZSTD_decompressBound( base, len );
    if( ZSTD_isError( size ) ) {
        return 0;
    }
    return size;
}

size_t zzip::get_entry_size( std::filesystem::path const &zzip_relative_path ) const
{
    zzip_footer footer{ footer_ };
    std::optional<zzip_file_entry> fparams = footer.get_entry( zzip_relative_path );
    if( !fparams.has_value() ) {
        return 0;
    }
    return fparams->len;
}

std::vector<std::byte> zzip::get_file( std::filesystem::path const &zzip_relative_path ) const
{
    std::vector<std::byte> buf{};
    size_t size = get_file_size( zzip_relative_path );
    buf.resize( size );
    size_t final_size = get_file_to( zzip_relative_path, buf.data(), size );
    // get_file_to returns 0 on error so we return an empty array.
    buf.resize( final_size );
    return buf;
}

size_t zzip::get_file_to( std::filesystem::path const &zzip_relative_path, std::byte *dest,
                          size_t dest_len ) const
{
    zzip_footer footer{ footer_ };
    std::optional<zzip_file_entry> fparams = footer.get_entry( zzip_relative_path );
    if( !fparams.has_value() ) {
        return 0;
    }
    size_t file_len = fparams->len;
    void *file_base = file_base_plus( fparams->offset );
    if( file_len > file_capacity_at( fparams->offset ) ) {
        return 0;
    }

    std::optional<uint64_t> checksum_opt;
    std::tie( file_base, file_len ) = read_and_skip_entry_metadata(
                                          file_base,
                                          file_len,
                                          nullptr,
                                          &checksum_opt );
    if( !checksum_opt.has_value() ) {
        return 0;
    }
    uint64_t checksum = XXH64( file_base, file_len, kCheckumSeed );
    if( checksum != *checksum_opt ) {
        return 0;
    }
    unsigned long long file_size = ZSTD_decompressBound( file_base, file_len );
    if( dest_len < file_size ) {
        return 0;
    }
    size_t actual = ZSTD_decompressDCtx( ctx_->dctx, dest, dest_len, file_base, file_len );
    if( ZSTD_isError( actual ) ) {
        return 0;
    }
    return actual;
}

size_t zzip::get_content_size() const
{
    return zzip_footer{ footer_ }.get_meta().content_end;
}

size_t zzip::get_zzip_size() const
{
    return file_->len();
}

std::filesystem::path const &zzip::get_path() const
{
    return path_;
}

std::vector<std::filesystem::path> zzip::get_entries() const
{
    zzip_footer footer{ footer_ };
    std::vector<compressed_entry> entries = footer.get_entries();
    std::vector<std::filesystem::path> entry_paths;
    entry_paths.reserve( entries.size() );
    for( const compressed_entry &entry : entries ) {
        entry_paths.emplace_back( std::filesystem::u8path( entry.path ) );
    }
    return entry_paths;
}

// Returns a copy of the footer json as a fresh independent object, not backed by the zzip.
JsonObject zzip::copy_footer() const
{
    if( !file_ ) {
        return footer_;
    } else {
        zzip_footer footer{ footer_ };
        std::optional<zzip_meta> meta_opt = footer.get_meta();
        size_t old_content_end = 0;
        if( meta_opt.has_value() ) {
            old_content_end = meta_opt->content_end;
        }
        if( file_->len() <= old_content_end ) {
            // This should really be impossible.
            return JsonObject{};
        }
        size_t footer_size = file_->len() - old_content_end;
        std::vector<std::byte> footer_buf;
        footer_buf.resize( footer_size );
        memcpy( footer_buf.data(), file_base_plus( old_content_end ), footer_size );
        std::shared_ptr<flexbuffer_storage> footer_storage = std::make_shared<zzip_vector_storage>(
                    std::move( footer_buf )
                );
        flexbuffers::Reference root = flexbuffer_root_from_storage( footer_storage );
        std::shared_ptr<parsed_flexbuffer> footer_flexbuffer = std::make_shared<zzip_flexbuffer>(
                    std::move( footer_storage )
                );
        return JsonValue( std::move( footer_flexbuffer ), root, nullptr, 0 );
    }
}

// Ensures the zzip file has room to write at least this many bytes.
// Returns the guaranteed size of the file, which is at least bytes large.
size_t zzip::ensure_capacity_for( size_t bytes )
{
    // Round up to a whole page. Drives nowadays are all 4k pages.
    size_t needed_pages = std::max( size_t{1}, ( bytes + kAssumedPageSize - 1 ) / kAssumedPageSize );

    size_t new_size = needed_pages * kAssumedPageSize;

    if( file_->len() < new_size && !file_->resize_file( new_size ) ) {
        return 0;
    }
    return new_size;
}

// Actually performs the compression and encoding of a file into the zzip.
size_t zzip::write_file_at( std::string_view filename, std::string_view content, size_t offset )
{
    // The format of a compressed entry is a series of zstd frames.
    // There are an unbounded number of leading skippable frames of unspecified content.
    // At present, we write two skippable frames per entry:
    //   - A frame for the filename of the entry.
    //   - A frame for a 64 bit XXH checksum of the entire compressed frame.
    //   - The actual compressed frame.
    // Returns the size of the entire file entry, or the return zstd error.
    // (i.e. from the start of the first skippable frame to the end of the compressed data).

    if( file_->len() <= offset ) {
        return 0;
    }

    size_t header_size = ZSTD_writeSkippableFrame(
                             file_base_plus( offset ),
                             file_capacity_at( offset ),
                             filename.data(),
                             filename.length(),
                             kEntryFileNameMagic
                         );
    if( ZSTD_isError( header_size ) ) {
        return header_size;
    }
    offset += header_size;
    // Make room for the checksum frame before the file.
    offset += kEntryChecksumFrameSize;
    size_t file_size = ZSTD_compress2(
                           ctx_->cctx,
                           file_base_plus( offset ),
                           file_capacity_at( offset ),
                           content.data(),
                           content.size()
                       );
    if( ZSTD_isError( file_size ) ) {
        return file_size;
    }
    uint64_t checksum = XXH64( file_base_plus( offset ), file_size, kCheckumSeed );
    uint64_t checksum_le = 0;
    MEM_writeLE64( &checksum_le, checksum );
    size_t checksum_size = ZSTD_writeSkippableFrame(
                               file_base_plus( offset - kEntryChecksumFrameSize ),
                               kEntryChecksumFrameSize,
                               reinterpret_cast<const char *>( &checksum_le ),
                               sizeof( checksum_le ),
                               kEntryChecksumMagic
                           );
    if( ZSTD_isError( checksum_size ) || checksum_size != kEntryChecksumFrameSize ) {
        return checksum_size;
    }
    return header_size + checksum_size + file_size;
}

// Writes a new footer at the end of the zzip, copying old entries from the given
// original JsonObject and inserting the given new entries.
// If shrink_to_fit is true, will shrink the file as needed to eliminate padding bytes
// between the content and the footer.
bool zzip::update_footer( JsonObject const &original_footer, size_t content_end,
                          const std::vector<compressed_entry> &entries, bool shrink_to_fit )
{
    std::unordered_set<std::string> processed_files;
    flexbuffers::Builder builder;
    size_t root_start = builder.StartMap();
    size_t total_content_size = 0;
    {
        size_t entries_start = builder.StartMap( kEntriesKey.data() );
        // NOLINTNEXTLINE(cata-almost-never-auto)
        for( const auto& [zzip_relative_path, offset, len] : entries ) {
            std::string new_filename = zzip_relative_path;
            {
                size_t new_entry_map = builder.StartMap( new_filename.c_str() );
                builder.UInt( kEntryOffsetKey.data(), offset );
                builder.UInt( kEntryLenKey.data(), len );
                builder.EndMap( new_entry_map );
                content_end = std::max( content_end, offset + len );
                total_content_size += len;
                processed_files.insert( new_filename );
            }
        }
        for( JsonMember entry : original_footer.get_object( kEntriesKey.data() ) ) {
            std::string old_filename = entry.name();
            if( !processed_files.count( old_filename ) ) {
                size_t entry_map = builder.StartMap( old_filename.c_str() );
                JsonObject old_data = entry.get_object();
                size_t offset = old_data.get_int( kEntryOffsetKey );
                size_t len = old_data.get_int( kEntryLenKey );
                builder.UInt( kEntryOffsetKey.data(), offset );
                builder.UInt( kEntryLenKey.data(), len );
                content_end = std::max( content_end, offset + len );
                total_content_size += len;
                builder.EndMap( entry_map );
            }
        }
        builder.EndMap( entries_start );
    }
    {
        size_t meta_start = builder.StartMap( kMetaKey.data() );
        builder.UInt( kMetaContentEndKey.data(), content_end );
        builder.UInt( kMetaTotalContentSizeKey.data(), total_content_size );
        builder.EndMap( meta_start );
    }
    builder.EndMap( root_start );
    builder.Finish();
    auto buf = builder.GetBuffer();

    if( shrink_to_fit ) {
        if( !file_->resize_file( content_end + buf.size() ) ) {
            return false;
        }
    } else {
        size_t guaranteed_capacity = ensure_capacity_for( content_end + buf.size() );
        if( guaranteed_capacity == 0 ) {
            return false;
        }
    }

    memcpy( file_base_plus( file_->len() - buf.size() ), buf.data(), buf.size() );
    uint64_t footer_checksum = XXH64( buf.data(), buf.size(), kCheckumSeed );

    if( !write_footer_len_and_checksum( file_->base(), kFooterChecksumFrameSize, buf.size(),
                                        footer_checksum ) ) {
        return false;
    }

    std::shared_ptr<flexbuffer_storage> new_storage = std::make_shared<zzip_flexbuffer_storage>
            ( file_ );
    flexbuffers::Reference new_root = flexbuffer_root_from_storage( new_storage );
    std::shared_ptr<parsed_flexbuffer> new_flexbuffer = std::make_shared<zzip_flexbuffer>(
                std::move( new_storage )
            );
    footer_ = JsonValue( std::move( new_flexbuffer ), new_root, nullptr, 0 );

    return true;
}

// Scans the zzip to read what data we can validate and write a fresh footer.
bool zzip::rewrite_footer()
{
    const size_t zzip_len = file_->len();
    size_t scan_offset = 0;

    // Check for the footer checksum data
    std::array<uint64_t, 2> footer_checksums{ 0, 0 };
    unsigned int footer_magic = -1;
    size_t footer_checksum_size = ZSTD_readSkippableFrame(
                                      footer_checksums.data(),
                                      2 * sizeof( uint64_t ),
                                      &footer_magic,
                                      file_->base(),
                                      zzip_len );
    bool needs_footer_frame = ZSTD_isError( footer_checksum_size ) ||
                              footer_magic != kFooterChecksumMagic ||
                              footer_checksum_size != kFooterChecksumContentSize;

    if( needs_footer_frame ) {
        // Either it's a completely fresh file or it's hopelessly corrupted.
        size_t new_file_size = ensure_capacity_for( kFooterChecksumFrameSize + kDefaultFooterSize );
        if( new_file_size < kFooterChecksumContentSize ) {
            return false;
        }
        // This is a glorified memcpy, assume it works.
        size_t footer_frame_size = ZSTD_writeSkippableFrame(
                                       file_->base(),
                                       new_file_size,
                                       footer_checksums.data(),
                                       kFooterChecksumContentSize,
                                       kFooterChecksumMagic );
        if( footer_frame_size != kFooterChecksumFrameSize ) {
            return false;
        }
    }

    scan_offset += kFooterChecksumFrameSize;

    std::unordered_map<std::string, compressed_entry> entries_map;

    // Try to recover as many files as we can
    while( scan_offset < zzip_len ) {
        // Scan the headers for the filename and checksum fields.
        // The checksum is for the _compressed_ content, to make sure it is
        // okay to decompress. Not sure if zstd does sensible things when asked
        // to decompress garbage (it almost certainly does, but still).
        const size_t entry_offset = scan_offset;
        size_t file_offset = 0;

        std::optional<std::string> filename_opt;
        std::optional<uint64_t> checksum_opt;

        size_t zzip_remaining = zzip_len - scan_offset;

        // For each entry we expect at least a filename and checksum header.
        void *file_ptr = nullptr; ;
        size_t file_len = 0;
        std::tie( file_ptr, file_len ) = read_and_skip_entry_metadata(
                                             file_base_plus( scan_offset ),
                                             zzip_remaining,
                                             &filename_opt,
                                             &checksum_opt );
        if( file_ptr == nullptr || file_len == 0 || file_len > zzip_remaining ) {
            break;
        }
        // zzip_remaining = length of zzip from start of entry
        // file_len = length of zzip from start of compressed file frame
        // -> offset of file from start of entry = zzip_remaining - file_len
        file_offset = scan_offset + ( zzip_remaining - file_len );

        if( !filename_opt.has_value() || !checksum_opt.has_value() ) {
            // Missing required metadata.
            break;
        }
        if( file_offset >= file_len ) {
            // We've run off the end of the file and read past the original footer.
            break;
        }

        void *file_frame_begin = file_base_plus( file_offset );
        size_t file_frame_size = ZSTD_findFrameCompressedSize( file_frame_begin, file_len );
        if( ZSTD_isError( file_frame_size ) ) {
            break;
        }
        uint64_t checksum = XXH64( file_frame_begin, file_frame_size, kCheckumSeed );
        if( checksum != checksum_opt ) {
            // Corruption in the compressed frame. Don't try to recover it, assume
            // everything after is lost.
            break;
        }
        scan_offset = file_offset + file_frame_size;
        compressed_entry &entry = entries_map.try_emplace( *filename_opt ).first->second;
        entry = compressed_entry{ *filename_opt, entry_offset, scan_offset - entry_offset };
    }

    std::vector<compressed_entry> entries;
    entries.reserve( entries_map.size() );
    for( auto &[path, entry] : entries_map ) {
        entries.emplace_back( std::move( entry ) );
    }

    return update_footer( JsonObject{}, scan_offset, entries, /* shrink_to_fit = */ true );
}

std::shared_ptr<zzip> zzip::create_from_folder( std::filesystem::path const &path,
        std::filesystem::path const &folder,
        std::filesystem::path const &dictionary )
{
    uint64_t total_file_size = 0;
    std::unordered_set<std::string> files_to_insert;
    std::vector<compressed_entry> new_entries;
    for( const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(
             folder ) ) {
        if( !entry.is_regular_file() ) {
            continue;
        }
        total_file_size += entry.file_size();
        files_to_insert.emplace( entry.path().generic_u8string() );
    }
    std::vector<std::filesystem::path> file_paths_to_insert;
    file_paths_to_insert.reserve( files_to_insert.size() );
    for( const std::string &file : files_to_insert ) {
        file_paths_to_insert.push_back( std::filesystem::u8path( file ) );
    }

    return create_from_folder_with_files( path,
                                          folder,
                                          file_paths_to_insert,
                                          total_file_size,
                                          dictionary );
}

std::shared_ptr<zzip> zzip::create_from_folder_with_files( std::filesystem::path const &path,
        std::filesystem::path const &folder,
        const std::vector<std::filesystem::path> &files,
        uint64_t total_file_size,
        std::filesystem::path const &dictionary )
{
    std::shared_ptr<zzip> zip = zzip::load( path, dictionary );
    JsonObject original_footer = zip->copy_footer();
    zzip_footer footer{ original_footer };
    std::optional<zzip_meta> meta_opt = footer.get_meta();
    size_t content_end = 0;
    if( meta_opt.has_value() ) {
        content_end = meta_opt->content_end;
    }
    std::vector<compressed_entry> new_entries;

    if( total_file_size == 0 ) {
        std::error_code ec;
        for( std::filesystem::path const &file : files ) {
            uint64_t file_size = std::filesystem::file_size( file, ec );
            if( !ec ) {
                total_file_size += file_size;
            }
        }
    }

    // Conservatively estimate 10x compression ratio
    size_t input_consumed = 0;
    size_t bytes_written = 0;
    zip->ensure_capacity_for( ( total_file_size / 10 ) + kFixedSizeOverhead );

    for( std::filesystem::path const &file_path : files ) {
        std::shared_ptr<const mmap_file> file = mmap_file::map_file( file_path );
        std::string file_relative_path = file_path.lexically_relative( folder ).generic_u8string();
        size_t compressed_size = 0;
        size_t needed = 0;
        do {
            compressed_size = zip->write_file_at(
                                  file_relative_path,
                                  std::string_view{ static_cast<const char *>( file->base() ), file->len() },
                                  content_end
                              );
            if( ZSTD_isError( compressed_size ) ) {
                if( ZSTD_getErrorCode( compressed_size ) == ZSTD_error_dstSize_tooSmall ) {
                    // Possibly ran out of file space. Figure out how much is left to insert.
                    // Estimate based on compression ratio of data so far.
                    // If we hit this again we need to add even more space.
                    needed += total_file_size * ( input_consumed > 0 ? ( static_cast<double>
                                                  ( bytes_written ) / input_consumed ) : 1 );
                    if( !zip->ensure_capacity_for( content_end + needed + kFixedSizeOverhead ) ) {
                        return nullptr;
                    }
                } else {
                    return nullptr;
                }
            }
        } while( ZSTD_isError( compressed_size ) );

        new_entries.emplace_back( compressed_entry{ std::move( file_relative_path ), content_end, compressed_size } );
        bytes_written += compressed_size;
        input_consumed += file->len();
        content_end += compressed_size;
        total_file_size -= file->len();
    }
    zip->update_footer( original_footer, content_end, new_entries, /* shrink_to_fit = */ true );
    return zip;
}

bool zzip::extract_to_folder( std::filesystem::path const &path,
                              std::filesystem::path const &folder,
                              std::filesystem::path const &dictionary )
{
    std::shared_ptr<zzip> zip = zzip::load( path, dictionary );

    if( !zip ) {
        return false;
    }

    for( const JsonMember &entry : zip->footer_.get_object( kEntriesKey ) ) {
        if( entry.name().empty() ) {
            continue;
        }
        std::filesystem::path filename = std::filesystem::u8path( entry.name() );
        size_t len = zip->get_file_size( filename );
        std::filesystem::path destination_file_path = folder / filename;
        if( !assure_dir_exist( destination_file_path.parent_path() ) ) {
            return false;
        }
        std::shared_ptr<mmap_file> file = mmap_file::map_writeable_file( destination_file_path );
        if( !file || !file->resize_file( len ) ) {
            return false;
        }
        if( zip->get_file_to( filename, static_cast<std::byte *>( file->base() ), file->len() ) != len ) {
            return false;
        }
    }
    return true;
}

bool zzip::delete_files( std::unordered_set<std::filesystem::path, std_fs_path_hash> const
                         &zzip_relative_paths )
{
    zzip_footer footer{ footer_ };
    std::vector<compressed_entry> compressed_entries = footer.get_entries();

    bool did_erase = false;
    compressed_entries.erase(
        std::remove_if(
            compressed_entries.begin(),
            compressed_entries.end(),
    [&]( const compressed_entry & entry ) {
        bool found = zzip_relative_paths.count( entry.path ) != 0; // NOLINT(cata-u8-path)
        did_erase = did_erase || found;
        return found;
    }
        ),
    compressed_entries.end()
    );

    if( !did_erase ) {
        return false;
    }

    // Rewrite the footer from scratch, with all entries but the removed one.
    return update_footer( JsonObject{}, 0, compressed_entries );
}

bool zzip::compact( double bloat_factor )
{
    zzip_footer footer{ footer_ };
    std::optional<zzip_meta> meta_opt = footer.get_meta();
    if( !meta_opt.has_value() ) {
        return false;
    }

    zzip_meta &meta = *meta_opt;

    if( bloat_factor > 0 ) {
        size_t max_allowed_size = meta.total_content_size * bloat_factor + kFixedSizeOverhead;
        // Are we even going to try to compact?
        if( file_->len() < max_allowed_size ) {
            return false;
        }

        // Can we save at least one physical page?
        size_t delta = file_->len() - ( meta.total_content_size + kFixedSizeOverhead );
        if( delta < kAssumedPageSize ) {
            return false;
        }
    }

    std::vector<compressed_entry> compressed_entries = footer.get_entries();

    // If we were compacting in place, sorting left to right would ensure that we
    // only move entries left that we need to move. Unchanged data would stay on the left.
    // However that exposes us to data loss on power loss because zzip recovery
    // relies on an intact sequence of zstd frames.
    std::sort( compressed_entries.begin(), compressed_entries.end(), []( const compressed_entry & lhs,
    const compressed_entry & rhs ) {
        return lhs.offset < rhs.offset;
    } );

    std::filesystem::path tmp_path = path_;
    tmp_path.replace_extension( ".zzip.tmp" ); // NOLINT(cata-u8-path)

    std::shared_ptr<mmap_file> compacted_file = mmap_file::map_writeable_file( tmp_path );
    if( !compacted_file ||
        !compacted_file->resize_file( meta.total_content_size + kFixedSizeOverhead ) ) {
        return false;
    }

    size_t current_end = kFooterChecksumFrameSize;
    for( compressed_entry &entry : compressed_entries ) {
        memcpy( static_cast<char *>( compacted_file->base() ) + current_end,
                file_base_plus( entry.offset ),
                entry.len
              );
        entry.offset = current_end;
        current_end += entry.len;
    }

    // We can swap the old mmap'd file with the new one
    // and write a fresh footer directly into it.
    JsonObject old_footer{ copy_footer() };
    old_footer.allow_omitted_members();

    file_ = compacted_file;
    compacted_file = nullptr;

    std::error_code ec;
    on_out_of_scope reset_on_failure{ [&] {
            file_ = mmap_file::map_writeable_file( path_ );
            std::filesystem::remove( tmp_path, ec );
        } };

    if( !update_footer( old_footer, current_end, compressed_entries, /* shrink_to_fit = */ true ) ) {
        return false;
    }

    // Swap the tmp file over the real path. Give it a couple tries in case of filesystem races.
    size_t attempts = 0;
    do {
        std::filesystem::rename( tmp_path, path_, ec );
    } while( ec && ++attempts < 3 );
    if( ec ) {
        return false;
    }
    reset_on_failure.cancel();
    return true;
}

// Can't directly increment void*, have to cast to char* first.
void *zzip::file_base_plus( size_t offset ) const
{
    return static_cast<char *>( file_->base() ) + offset;
}

// A helper to handle underflow. I wouldn't write it if I didn't hit it.
size_t zzip::file_capacity_at( size_t offset ) const
{
    if( file_->len() < offset ) {
        return 0;
    }
    return file_->len() - offset;
}
