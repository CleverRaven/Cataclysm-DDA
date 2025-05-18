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

#include <filesystem>

#include "cata_scope_helpers.h"
#include "cata_utility.h"

#ifdef __clang__
#define CLANG_REINITIALIZES [[clang::reinitializes]]
#else
#define CLANG_REINITIALIZES
#endif

mmap_file::mmap_file() = default;

mmap_file::~mmap_file() = default;

struct mmap_file::impl {
#ifdef _WIN32
    impl( HANDLE file, bool writeable ) : writeable { writeable }, file { file} {}
#else
    impl( int file, bool writeable ) : writeable { writeable}, file { file } {}
#endif
    CLANG_REINITIALIZES void reset() {
        writeable = false;
        base = nullptr;
        len = 0;
#ifdef _WIN32
        file = INVALID_HANDLE_VALUE;
        file_mapping = NULL;
#else
        file = -1;
#endif
    }

    impl( impl &&rhs ) noexcept {
        *this = std::move( rhs );
        rhs.reset();
    }
    impl &operator=( impl &&rhs ) = default;

    // No copying
    impl( const impl & ) = delete;
    impl &operator=( const impl & ) = delete;

    bool writeable = false;

    void *base = nullptr;
    size_t len = 0;

#ifdef _WIN32
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE file_mapping = NULL;

    bool map_view() {
        if( base != nullptr && len != 0 ) {
            return true;
        }
        LARGE_INTEGER file_size{};
        if( !GetFileSizeEx( file, &file_size ) ) {
            // Failed to get file size.
            return false;
        }

        if( file_size.QuadPart == 0 ) {
            return false;
        }

        HANDLE file_mapping_handle = CreateFileMappingW(
                                         file,
                                         nullptr,
                                         writeable ? PAGE_READWRITE : PAGE_READONLY,
                                         file_size.HighPart,
                                         file_size.LowPart,
                                         nullptr
                                     );
        if( file_mapping_handle == NULL ) {
            return false;
        }
        on_out_of_scope close_file_mapping_guard( [&] { CloseHandle( file_mapping_handle ); } );

        void *map_base = MapViewOfFile(
                             file_mapping_handle,
                             ( writeable ? FILE_MAP_WRITE : 0 ) | FILE_MAP_READ,
                             0,
                             0,
                             file_size.QuadPart
                         );
        if( map_base == nullptr ) {
            // Failed to mmap file.
            return false;
        }
        close_file_mapping_guard.cancel();
        file_mapping = file_mapping_handle;
        base = map_base;
        len = file_size.QuadPart;
        return true;
    }

    bool unmap_view() {
        bool success = true;
        if( base != nullptr ) {
            if( !UnmapViewOfFile( base ) ) {
                success = false;
            }
        }
        base = nullptr;
        len = 0;
        if( file_mapping != NULL ) {
            if( !CloseHandle( file_mapping ) ) {
                success = false;
            }
        }
        file_mapping = NULL;
        return success;
    }

    ~impl() {
        unmap_view();
        if( file != INVALID_HANDLE_VALUE ) {
            CloseHandle( file );
        }
    }
#else
    int file = -1;

    bool map_view() {
        struct stat buf {};
        if( fstat( file, &buf ) ) {
            return false;
        }
        size_t file_size = buf.st_size;

        void *map_base = mmap( nullptr, file_size, ( writeable ? PROT_WRITE : 0 ) | PROT_READ, MAP_SHARED,
                               file, 0 );
        if( map_base == MAP_FAILED ) {
            return false;
        }

        base = map_base;
        len = file_size;

        return true;
    }

    bool unmap_view() {
        if( base != nullptr ) {
            munmap( base, len );
        }
        base = nullptr;
        len = 0;
        return true;
    }
    ~impl() {
        unmap_view();
        if( file != -1 ) {
            close( file );
        }
    }
#endif
};

std::unique_ptr<mmap_file> mmap_file::map_file_generic(
    const std::filesystem::path &file_path,
    bool writeable )
{
    std::unique_ptr<mmap_file> mapped_file;

#ifdef _WIN32
    HANDLE file_handle;
    file_handle = CreateFileW(
                      file_path.native().c_str(),
                      ( writeable ? GENERIC_WRITE : 0 ) | GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      nullptr,
                      writeable ? OPEN_ALWAYS : OPEN_EXISTING,
                      0,
                      nullptr
                  );

    if( file_handle == INVALID_HANDLE_VALUE ) {
        return nullptr;
    }
#else
    const std::string &file_path_string = file_path.native();
    // 644 = User RW, Group R, Other R.
    // Only used when creating a file. Ignored when file exists.
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int file_handle = open( file_path_string.c_str(), writeable ? O_CREAT | O_RDWR : O_RDONLY, perms );
    if( file_handle == -1 ) {
        return nullptr;
    }
#endif
    mapped_file = std::unique_ptr<mmap_file> { new mmap_file() };
    mapped_file->pimpl = std::make_shared<impl>( file_handle, writeable );
    if( !mapped_file->pimpl->map_view() && !writeable ) {
        return nullptr;
    }
    return mapped_file;
}

std::shared_ptr<const mmap_file> mmap_file::map_file( const std::filesystem::path &file_path )
{
    return map_file_generic( file_path, false );
}

std::unique_ptr<mmap_file> mmap_file::map_writeable_file( const std::filesystem::path &file_path )
{
    return map_file_generic( file_path, true );
}

bool mmap_file::resize_file( size_t desired_size )
{
    if( desired_size == pimpl->len ) {
        return true;
    }
    if( !pimpl->unmap_view() ) {
        return false;
    }
#ifdef _WIN32
    LARGE_INTEGER file_size;
    file_size.QuadPart = desired_size;
    if( !SetFilePointerEx( pimpl->file, file_size, NULL, FILE_BEGIN ) ) {
        return false;
    }
    if( !SetEndOfFile( pimpl->file ) ) {
        return false;
    }
#else
    if( ftruncate( pimpl->file, desired_size ) ) {
        return false;
    }
#endif

    if( !pimpl->map_view() ) {
        return false;
    }
    return true;
}

void *mmap_file::base()
{
    return pimpl->base;
}

void const *mmap_file::base() const
{
    return pimpl->base;
}

size_t mmap_file::len() const
{
    return pimpl->len;
}

void mmap_file::flush()
{
    if( !base() || len() == 0 ) {
        return;
    }
#ifdef _WIN32
    FlushViewOfFile( base(), 0 );
    FlushFileBuffers( pimpl->file );
#else
    msync( base(), len(), MS_SYNC );
#endif
}

void mmap_file::flush( size_t offset, size_t length )
{
    if( offset + length > len() ) {
        return;
    }
    char *base_ptr = reinterpret_cast<char *>( base() ) + offset;
#ifdef _WIN32
    FlushViewOfFile( base_ptr, length );
    FlushFileBuffers( pimpl->file );
#else
    // msync requires the base pointer to be rounded to a page boundary.
    size_t page_offset = offset % 4096;
    base_ptr -= page_offset;
    length += page_offset;
    msync( base_ptr, length, MS_SYNC );
#endif
}
