#include "mmap_file.h"

#ifdef _WIN32

#include <vector>

#include "platform_win.h"

#else

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#endif

#include <ghc/fs_std_fwd.hpp>

#include "cata_scope_helpers.h"
#include "cata_utility.h"

mmap_file::mmap_file() = default;

mmap_file::~mmap_file()
{
#ifdef _WIN32
    if( base != nullptr ) {
        UnmapViewOfFile( base );
    }
#else
    if( base != nullptr ) {
        munmap( base, len );
    }
#endif
}

struct mmap_file::handle {
    // No copying
    handle() = default;
    handle( const handle & ) = delete;
    handle &operator=( const handle & ) = delete;

    // We can implement these if we need them.
    handle( handle &&h ) = delete;
    handle &operator=( handle &&h ) = delete;

    // Only Windows requires after-the-fact cleanup.
#ifdef _WIN32
    ~handle() {
        if( file_mapping != INVALID_HANDLE_VALUE ) {
            CloseHandle( file_mapping );
        }
        if( file != INVALID_HANDLE_VALUE ) {
            CloseHandle( file );
        }
    }

    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE file_mapping = INVALID_HANDLE_VALUE;
#else
#endif
};

std::shared_ptr<mmap_file> mmap_file::map_file( const std::string &file_path )
{
    return map_file( fs::u8path( file_path ) );
}

std::shared_ptr<mmap_file> mmap_file::map_file( const fs::path &file_path )
{
    std::shared_ptr<mmap_file> mapped_file;

#ifdef _WIN32
    HANDLE file_handle = CreateFileW(
                             file_path.native().c_str(),
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                         );
    if( file_handle == INVALID_HANDLE_VALUE ) {
        // Failed to open file.
        return mapped_file;
    }
    LARGE_INTEGER file_size{};
    if( !GetFileSizeEx( file_handle, &file_size ) ) {
        // Failed to get file size.
        return mapped_file;
    }
    HANDLE file_mapping_handle = CreateFileMappingW(
                                     file_handle,
                                     NULL,
                                     PAGE_READONLY,
                                     0,
                                     0,
                                     NULL
                                 );
    if( file_mapping_handle == NULL ) {
        return mapped_file;
    }
    void *map_base = MapViewOfFile(
                         file_mapping_handle,
                         FILE_MAP_READ,
                         0,
                         0,
                         file_size.QuadPart
                     );
    if( map_base == nullptr ) {
        // Failed to mmap file.
        return mapped_file;
    }
    mapped_file = std::shared_ptr<mmap_file> { new mmap_file() };
    mapped_file->mmap_handle = std::make_shared<handle>();
    mapped_file->mmap_handle->file = file_handle;
    mapped_file->mmap_handle->file_mapping = file_mapping_handle;
    mapped_file->base = static_cast<uint8_t *>( map_base );
    mapped_file->len = file_size.QuadPart;
#else
    const std::string &file_path_string = file_path.native();
    std::error_code ec;
    size_t file_size = fs::file_size( file_path_string.c_str(), ec );
    if( ec ) {
        return mapped_file;
    }

    int fd = open( file_path_string.c_str(), O_RDONLY );
    if( fd == -1 ) {
        return mapped_file;
    }
    on_out_of_scope close_file_guard( [&] { close( fd ); } );
    void *map_base = mmap( nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0 );
    if( !map_base ) {
        return mapped_file;
    }

    mapped_file = std::shared_ptr<mmap_file> { new mmap_file() };
    // No need for an underlying handle.
    mapped_file->base = static_cast<uint8_t *>( map_base );
    mapped_file->len = file_size;
#endif
    return mapped_file;
}
