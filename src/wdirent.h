#pragma once
/*
 * dirent.h - Dirent interface for Microsoft Visual Studio
 *
 * This file is part of dirent.  Dirent may be freely distributed
 * under the MIT license.  For all details and documentation, see
 * https://github.com/tronkko/dirent
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 1998-2021 Toni Ronkko
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Modifications for Cataclysm BN:
 * 1. Some functions were removed as dead code.
 * 2. dirent_mbstowcs_s and dirent_wcstombs_s were rewritten to
 *    use MultiByteToWideChar and WideCharToMultiByte
 *    to make it work under mingw64.
 * 3. dirent_mbstowcs_s and dirent_wcstombs_s are always used
 *    instead of mbstowcs_s and wcstombs_s on MSC.
 */
#ifndef DIRENT_H
#define DIRENT_H

/* Hide warnings about unreferenced local functions */
#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#   pragma warning(disable:4505)
#elif defined(__GNUC__)
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif

/*
 * Include windows.h without Windows Sockets 1.1 to prevent conflicts with
 * Windows Sockets 2.0.
 */
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

/* Indicates that d_type field is available in dirent structure */
#define _DIRENT_HAVE_D_TYPE

/* Indicates that d_namlen field is available in dirent structure */
#define _DIRENT_HAVE_D_NAMLEN

/* Entries missing from MSVC 6.0 */
#if !defined(FILE_ATTRIBUTE_DEVICE)
#   define FILE_ATTRIBUTE_DEVICE 0x40
#endif

/* File type and permission flags for stat(), general mask */
#if !defined(S_IFMT)
#   define S_IFMT _S_IFMT
#endif

/* Directory bit */
#if !defined(S_IFDIR)
#   define S_IFDIR _S_IFDIR
#endif

/* Character device bit */
#if !defined(S_IFCHR)
#   define S_IFCHR _S_IFCHR
#endif

/* Pipe bit */
#if !defined(S_IFFIFO)
#   define S_IFFIFO _S_IFFIFO
#endif

/* Regular file bit */
#if !defined(S_IFREG)
#   define S_IFREG _S_IFREG
#endif

/* Read permission */
#if !defined(S_IREAD)
#   define S_IREAD _S_IREAD
#endif

/* Write permission */
#if !defined(S_IWRITE)
#   define S_IWRITE _S_IWRITE
#endif

/* Execute permission */
#if !defined(S_IEXEC)
#   define S_IEXEC _S_IEXEC
#endif

/* Pipe */
#if !defined(S_IFIFO)
#   define S_IFIFO _S_IFIFO
#endif

/* Block device */
#if !defined(S_IFBLK)
#   define S_IFBLK 0
#endif

/* Link */
#if !defined(S_IFLNK)
#   define S_IFLNK 0
#endif

/* Socket */
#if !defined(S_IFSOCK)
#   define S_IFSOCK 0
#endif

/* Read user permission */
#if !defined(S_IRUSR)
#   define S_IRUSR S_IREAD
#endif

/* Write user permission */
#if !defined(S_IWUSR)
#   define S_IWUSR S_IWRITE
#endif

/* Execute user permission */
#if !defined(S_IXUSR)
#   define S_IXUSR 0
#endif

/* Read group permission */
#if !defined(S_IRGRP)
#   define S_IRGRP 0
#endif

/* Write group permission */
#if !defined(S_IWGRP)
#   define S_IWGRP 0
#endif

/* Execute group permission */
#if !defined(S_IXGRP)
#   define S_IXGRP 0
#endif

/* Read others permission */
#if !defined(S_IROTH)
#   define S_IROTH 0
#endif

/* Write others permission */
#if !defined(S_IWOTH)
#   define S_IWOTH 0
#endif

/* Execute others permission */
#if !defined(S_IXOTH)
#   define S_IXOTH 0
#endif

/* Maximum length of file name */
#if !defined(PATH_MAX)
#   define PATH_MAX MAX_PATH
#endif
#if !defined(FILENAME_MAX)
#   define FILENAME_MAX MAX_PATH
#endif
#if !defined(NAME_MAX)
#   define NAME_MAX FILENAME_MAX
#endif

/* File type flags for d_type */
#define DT_UNKNOWN 0
#define DT_REG S_IFREG
#define DT_DIR S_IFDIR
#define DT_FIFO S_IFIFO
#define DT_SOCK S_IFSOCK
#define DT_CHR S_IFCHR
#define DT_BLK S_IFBLK
#define DT_LNK S_IFLNK

/* Macros for converting between st_mode and d_type */
#define IFTODT(mode) ((mode) & S_IFMT)
#define DTTOIF(type) (type)

/*
 * File type macros.  Note that block devices, sockets and links cannot be
 * distinguished on Windows and the macros S_ISBLK, S_ISSOCK and S_ISLNK are
 * only defined for compatibility.  These macros should always return false
 * on Windows.
 */
#if !defined(S_ISFIFO)
#   define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISDIR)
#   define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG)
#   define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISLNK)
#   define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK)
#   define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISCHR)
#   define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISBLK)
#   define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#endif

/* Return the exact length of the file name without zero terminator */
#define _D_EXACT_NAMLEN(p) ((p)->d_namlen)

/* Return the maximum size of a file name */
#define _D_ALLOC_NAMLEN(p) ((PATH_MAX)+1)


#ifdef __cplusplus
extern "C" {
#endif


/* Wide-character version */
struct _wdirent {
    /* Always zero */
    long d_ino;

    /* File position within stream */
    long d_off;

    /* Structure size */
    unsigned short d_reclen;

    /* Length of name without \0 */
    size_t d_namlen;

    /* File type */
    int d_type;

    /* File name */
    wchar_t d_name[PATH_MAX + 1];
};
typedef struct _wdirent _wdirent;

struct _WDIR {
    /* Current directory entry */
    struct _wdirent ent;

    /* Private file data */
    WIN32_FIND_DATAW data;

    /* True if data is valid */
    int cached;

    /* Win32 search handle */
    HANDLE handle;

    /* Initial directory name */
    wchar_t *patt;
};
typedef struct _WDIR _WDIR;

/* Multi-byte character version */
struct dirent {
    /* Always zero */
    long d_ino;

    /* File position within stream */
    long d_off;

    /* Structure size */
    unsigned short d_reclen;

    /* Length of name without \0 */
    size_t d_namlen;

    /* File type */
    int d_type;

    /* File name */
    char d_name[PATH_MAX + 1];
};
typedef struct dirent dirent;

struct DIR {
    struct dirent ent;
    struct _WDIR *wdirp;
};
typedef struct DIR DIR;


/* Dirent functions */
static DIR *opendir( const char *dirname );
static _WDIR *_wopendir( const wchar_t *dirname );

static struct dirent *readdir( DIR *dirp );

static int readdir_r(
    DIR *dirp, struct dirent *entry, struct dirent **result );

static int closedir( DIR *dirp );
static int _wclosedir( _WDIR *dirp );

/* For compatibility with Symbian */
#define wdirent _wdirent
#define WDIR _WDIR
#define wopendir _wopendir
#define wreaddir _wreaddir

/* Optimize dirent_set_errno() away on modern Microsoft compilers */
#if defined(_MSC_VER) && _MSC_VER >= 1400
#   define dirent_set_errno _set_errno
#endif


/* Internal utility functions */
static WIN32_FIND_DATAW *dirent_first( _WDIR *dirp );
static WIN32_FIND_DATAW *dirent_next( _WDIR *dirp );

static int dirent_mbstowcs_s(
    size_t *pReturnValue, wchar_t *wcstr, size_t sizeInWords,
    const char *mbstr, size_t count );

static int dirent_wcstombs_s(
    size_t *pReturnValue, char *mbstr, size_t sizeInBytes,
    const wchar_t *wcstr, size_t count );

#if !defined(_MSC_VER) || _MSC_VER < 1400
static void dirent_set_errno( int error );
#endif


/*
 * Open directory stream DIRNAME for read and return a pointer to the
 * internal working area that is used to retrieve individual directory
 * entries.
 */
static _WDIR *_wopendir( const wchar_t *dirname )
{
    wchar_t *p;

    /* Must have directory name */
    if( dirname == NULL || dirname[0] == '\0' ) {
        dirent_set_errno( ENOENT );
        return NULL;
    }

    /* Allocate new _WDIR structure */
    _WDIR *dirp = ( _WDIR * ) malloc( sizeof( struct _WDIR ) );
    if( !dirp ) {
        return NULL;
    }

    /* Reset _WDIR structure */
    dirp->handle = INVALID_HANDLE_VALUE;
    dirp->patt = NULL;
    dirp->cached = 0;

    /*
     * Compute the length of full path plus zero terminator
     *
     * Note that on WinRT there's no way to convert relative paths
     * into absolute paths, so just assume it is an absolute path.
     */
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    /* Desktop */
    DWORD n = GetFullPathNameW( dirname, 0, NULL, NULL );
#else
    /* WinRT */
    size_t n = wcslen( dirname );
#endif

    /* Allocate room for absolute directory name and search pattern */
    dirp->patt = ( wchar_t * ) malloc( sizeof( wchar_t ) * n + 16 );
    if( dirp->patt == NULL ) {
        goto exit_closedir;
    }

    /*
     * Convert relative directory name to an absolute one.  This
     * allows rewinddir() to function correctly even when current
     * working directory is changed between opendir() and rewinddir().
     *
     * Note that on WinRT there's no way to convert relative paths
     * into absolute paths, so just assume it is an absolute path.
     */
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    /* Desktop */
    n = GetFullPathNameW( dirname, n, dirp->patt, NULL );
    if( n <= 0 ) {
        goto exit_closedir;
    }
#else
    /* WinRT */
    wcsncpy_s( dirp->patt, n + 1, dirname, n );
#endif

    /* Append search pattern \* to the directory name */
    p = dirp->patt + n;
    switch( p[-1] ) {
        case '\\':
        case '/':
        case ':':
            /* Directory ends in path separator, e.g. c:\temp\ */
            /*NOP*/
            ;
            break;

        default:
            /* Directory name doesn't end in path separator */
            *p++ = '\\';
    }
    *p++ = '*';
    *p = '\0';

    /* Open directory stream and retrieve the first entry */
    if( !dirent_first( dirp ) ) {
        goto exit_closedir;
    }

    /* Success */
    return dirp;

    /* Failure */
exit_closedir:
    _wclosedir( dirp );
    return NULL;
}

/*
 * Close directory stream opened by opendir() function.  This invalidates the
 * DIR structure as well as any directory entry read previously by
 * _wreaddir().
 */
static int _wclosedir( _WDIR *dirp )
{
    if( !dirp ) {
        dirent_set_errno( EBADF );
        return /*failure*/ -1;
    }

    /* Release search handle */
    if( dirp->handle != INVALID_HANDLE_VALUE ) {
        FindClose( dirp->handle );
    }

    /* Release search pattern */
    free( dirp->patt );

    /* Release directory structure */
    free( dirp );
    return /*success*/0;
}

/* Get first directory entry */
static WIN32_FIND_DATAW *dirent_first( _WDIR *dirp )
{
    if( !dirp ) {
        return NULL;
    }

    /* Open directory and retrieve the first entry */
    dirp->handle = FindFirstFileExW(
                       dirp->patt, FindExInfoStandard, &dirp->data,
                       FindExSearchNameMatch, NULL, 0 );
    if( dirp->handle == INVALID_HANDLE_VALUE ) {
        goto error;
    }

    /* A directory entry is now waiting in memory */
    dirp->cached = 1;
    return &dirp->data;

error:
    /* Failed to open directory: no directory entry in memory */
    dirp->cached = 0;

    /* Set error code */
    DWORD errorcode = GetLastError();
    switch( errorcode ) {
        case ERROR_ACCESS_DENIED:
            /* No read access to directory */
            dirent_set_errno( EACCES );
            break;

        case ERROR_DIRECTORY:
            /* Directory name is invalid */
            dirent_set_errno( ENOTDIR );
            break;

        case ERROR_PATH_NOT_FOUND:
        default:
            /* Cannot find the file */
            dirent_set_errno( ENOENT );
    }
    return NULL;
}

/* Get next directory entry */
static WIN32_FIND_DATAW *dirent_next( _WDIR *dirp )
{
    /* Is the next directory entry already in cache? */
    if( dirp->cached ) {
        /* Yes, a valid directory entry found in memory */
        dirp->cached = 0;
        return &dirp->data;
    }

    /* No directory entry in cache */
    if( dirp->handle == INVALID_HANDLE_VALUE ) {
        return NULL;
    }

    /* Read the next directory entry from stream */
    if( FindNextFileW( dirp->handle, &dirp->data ) == FALSE ) {
        goto exit_close;
    }

    /* Success */
    return &dirp->data;

    /* Failure */
exit_close:
    FindClose( dirp->handle );
    dirp->handle = INVALID_HANDLE_VALUE;
    return NULL;
}

/* Open directory stream using plain old C-string */
static DIR *opendir( const char *dirname )
{
    /* Must have directory name */
    if( dirname == NULL || dirname[0] == '\0' ) {
        dirent_set_errno( ENOENT );
        return NULL;
    }

    /* Allocate memory for DIR structure */
    struct DIR *dirp = ( DIR * ) malloc( sizeof( struct DIR ) );
    if( !dirp ) {
        return NULL;
    }

    /* Convert directory name to wide-character string */
    wchar_t wname[PATH_MAX + 1];
    size_t n;
    int error = dirent_mbstowcs_s( &n, wname, PATH_MAX + 1, dirname, PATH_MAX + 1 );
    if( error ) {
        goto exit_failure;
    }

    /* Open directory stream using wide-character name */
    dirp->wdirp = _wopendir( wname );
    if( !dirp->wdirp ) {
        goto exit_failure;
    }

    /* Success */
    return dirp;

    /* Failure */
exit_failure:
    free( dirp );
    return NULL;
}

/* Read next directory entry */
static struct dirent *readdir( DIR *dirp )
{
    /*
     * Read directory entry to buffer.  We can safely ignore the return
     * value as entry will be set to NULL in case of error.
     */
    struct dirent *entry;
    ( void ) readdir_r( dirp, &dirp->ent, &entry );

    /* Return pointer to statically allocated directory entry */
    return entry;
}

/*
 * Read next directory entry into called-allocated buffer.
 *
 * Returns zero on success.  If the end of directory stream is reached, then
 * sets result to NULL and returns zero.
 */
static int readdir_r(
    DIR *dirp, struct dirent *entry, struct dirent **result )
{
    /* Read next directory entry */
    WIN32_FIND_DATAW *datap = dirent_next( dirp->wdirp );
    if( !datap ) {
        /* No more directory entries */
        *result = NULL;
        return /*OK*/0;
    }

    /* Attempt to convert file name to multi-byte string */
    size_t n;
    int error = dirent_wcstombs_s(
                    &n, entry->d_name, PATH_MAX + 1,
                    datap->cFileName, PATH_MAX + 1 );

    /*
     * If the file name cannot be represented by a multi-byte string, then
     * attempt to use old 8+3 file name.  This allows the program to
     * access files although file names may seem unfamiliar to the user.
     *
     * Be ware that the code below cannot come up with a short file name
     * unless the file system provides one.  At least VirtualBox shared
     * folders fail to do this.
     */
    if( error && datap->cAlternateFileName[0] != '\0' ) {
        error = dirent_wcstombs_s(
                    &n, entry->d_name, PATH_MAX + 1,
                    datap->cAlternateFileName, PATH_MAX + 1 );
    }

    if( !error ) {
        /* Length of file name excluding zero terminator */
        entry->d_namlen = n - 1;

        /* File attributes */
        DWORD attr = datap->dwFileAttributes;
        if( ( attr & FILE_ATTRIBUTE_DEVICE ) != 0 ) {
            entry->d_type = DT_CHR;
        } else if( ( attr & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {
            entry->d_type = DT_DIR;
        } else {
            entry->d_type = DT_REG;
        }

        /* Reset dummy fields */
        entry->d_ino = 0;
        entry->d_off = 0;
        entry->d_reclen = sizeof( struct dirent );
    } else {
        /*
         * Cannot convert file name to multi-byte string so construct
         * an erroneous directory entry and return that.  Note that
         * we cannot return NULL as that would stop the processing
         * of directory entries completely.
         */
        entry->d_name[0] = '?';
        entry->d_name[1] = '\0';
        entry->d_namlen = 1;
        entry->d_type = DT_UNKNOWN;
        entry->d_ino = 0;
        entry->d_off = -1;
        entry->d_reclen = 0;
    }

    /* Return pointer to directory entry */
    *result = entry;
    return /*OK*/0;
}

/* Close directory stream */
static int closedir( DIR *dirp )
{
    int ok;

    if( !dirp ) {
        goto exit_failure;
    }

    /* Close wide-character directory stream */
    ok = _wclosedir( dirp->wdirp );
    dirp->wdirp = NULL;

    /* Release multi-byte character version */
    free( dirp );
    return ok;

exit_failure:
    /* Invalid directory stream */
    dirent_set_errno( EBADF );
    return /*failure*/ -1;
}

/* Convert multi-byte string to wide character string */
static int dirent_mbstowcs_s(
    size_t *pReturnValue, wchar_t *wcstr,
    size_t sizeInWords, const char *mbstr, size_t /*count*/ )
{
    const int required_size = MultiByteToWideChar( CP_UTF8, 0, mbstr, -1, NULL, 0 ) + 1;
    if( required_size > static_cast<int>( sizeInWords ) ) {
        return 1;
    }
    const int n = MultiByteToWideChar( CP_UTF8, 0, mbstr, -1, wcstr, required_size );
    if( n == 0 ) {
        debugmsg( "MultiByteToWideChar failed!" );
        return 1;
    }
    if( pReturnValue ) {
        *pReturnValue = n;
    }
    return 0;
}

/* Convert wide-character string to multi-byte string */
static int dirent_wcstombs_s(
    size_t *pReturnValue, char *mbstr,
    size_t sizeInBytes, const wchar_t *wcstr, size_t /*count*/ )
{
    const int required_size = WideCharToMultiByte( CP_UTF8, 0, wcstr, -1, NULL, 0, NULL, 0 ) + 1;
    if( required_size > static_cast<int>( sizeInBytes ) ) {
        return 1;
    }
    const int n = WideCharToMultiByte( CP_UTF8, 0, wcstr, -1, mbstr, required_size, NULL, 0 );
    if( n == 0 ) {
        debugmsg( "WideCharToMultiByte failed!" );
        return 1;
    }
    if( pReturnValue ) {
        *pReturnValue = n;
    }
    return 0;
}

/* Set errno variable */
#if !defined(_MSC_VER) || _MSC_VER < 1400
static void dirent_set_errno( int error )
{
    /* Non-Microsoft compiler or older Microsoft compiler */
    errno = error;
}
#endif

#ifdef __cplusplus
}
#endif
#endif /*DIRENT_H*/
