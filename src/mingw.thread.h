#pragma once
/**
* @file mingw.thread.h
* @brief std::thread implementation for MinGW
*
* This file is part of the mingw-w64 runtime package.
* No warranty is given; refer to the file DISCLAIMER within this package.
*/

#ifndef WIN32STDTHREAD_H
#ifndef _GLIBCXX_HAS_GTHREADS

#define WIN32STDTHREAD_H
#define _GLIBCXX_THREAD 1
#define _GLIBCXX_HAS_GTHREADS 1

#include <windows.h>
#include <process.h>
#include <functional>
#include <memory>
#include <chrono>
#include <system_error>

//instead of INVALID_HANDLE_VALUE _beginthreadex returns 0
#define _STD_THREAD_INVALID_HANDLE 0
namespace std
{

class thread
{
    public:
        class id
        {
                DWORD mId;
                void clear() {
                    mId = 0;
                }
                friend class thread;
            public:
                id( DWORD aId = 0 ): mId( aId ) {}
                bool operator==( const id &other ) const {
                    return mId == other.mId;
                }
        };
    protected:
        HANDLE mHandle;
        id mThreadId;
    public:
        typedef HANDLE native_handle_type;
        id get_id() const noexcept {
            return mThreadId;
        }
        native_handle_type native_handle() const {
            return mHandle;
        }
        thread(): mHandle( _STD_THREAD_INVALID_HANDLE ) {}
        thread( thread &other )
            : mHandle( other.mHandle ), mThreadId( other.mThreadId ) {
            other.mHandle = _STD_THREAD_INVALID_HANDLE;
            other.mThreadId.clear();
        }
        template<class Function, class... Args>
        explicit thread( Function &&f, Args &&... args ) {
            typedef decltype( std::bind( f, args... ) ) Call;
            Call *call = new Call( std::bind( f, args... ) );
            mHandle = ( HANDLE )_beginthreadex( NULL, 0, threadfunc<Call>,
                                                ( LPVOID )call, 0, ( unsigned * ) & ( mThreadId.mId ) );
        }
        template <class Call>
        static unsigned int __stdcall threadfunc( void *arg ) {
            std::unique_ptr<Call> upCall( static_cast<Call *>( arg ) );
            ( *upCall )();
            return ( unsigned long )0;
        }
        bool joinable() const {
            return mHandle != _STD_THREAD_INVALID_HANDLE;
        }
        void join() {
            if( get_id() == GetCurrentThreadId() ) {
                throw system_error( EDEADLK, generic_category() );
            }
            if( mHandle == _STD_THREAD_INVALID_HANDLE ) {
                throw system_error( ESRCH, generic_category() );
            }
            if( !joinable() ) {
                throw system_error( EINVAL, generic_category() );
            }
            WaitForSingleObject( mHandle, INFINITE );
            CloseHandle( mHandle );
            mHandle = _STD_THREAD_INVALID_HANDLE;
            mThreadId.clear();
        }

        ~thread() {
            if( joinable() ) {
                std::terminate();
            }
        }
        thread &operator=( const thread & ) = delete;
        thread &operator=( thread &&other ) noexcept {
            if( joinable() ) {
                std::terminate();
            }
            swap( std::forward<thread>( other ) );
            return *this;
        }
        void swap( thread &&other ) noexcept {
            std::swap( mHandle, other.mHandle );
            std::swap( mThreadId.mId, other.mThreadId.mId );
        }
        static unsigned int hardware_concurrency() noexcept {
            return 1;
        }
        void detach() {
            if( !joinable() ) {
                throw system_error();
            }
            mHandle = _STD_THREAD_INVALID_HANDLE;
            mThreadId.clear();
        }
};
namespace this_thread
{
inline thread::id get_id()
{
    return thread::id( GetCurrentThreadId() );
}
inline void yield()
{
    Sleep( 0 );
}
template< class Rep, class Period >
void sleep_for( const std::chrono::duration<Rep, Period> &sleep_duration )
{
    Sleep( chrono::duration_cast<chrono::milliseconds>( sleep_duration ).count() );
}
template <class Clock, class Duration>
void sleep_until( const std::chrono::time_point<Clock, Duration> &sleep_time )
{
    sleep_for( sleep_time - Clock::now() );
}
}

}
#endif // _GLIBCXX_THREAD
#endif // WIN32STDTHREAD_H
