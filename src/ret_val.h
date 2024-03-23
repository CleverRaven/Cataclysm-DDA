#pragma once
#ifndef CATA_SRC_RET_VAL_H
#define CATA_SRC_RET_VAL_H

#include <string>
#include <type_traits>

#include "string_formatter.h"

class ret_val_common
{
    public:
        bool success() const {
            return succ;
        }

        const std::string &str() const {
            return msg;
        }

        const char *c_str() const {
            return msg.c_str();
        }
    protected:
        template<typename S>
        using is_convertible_to_string = std::enable_if_t <
                                         !std::is_same_v<S, std::nullptr_t>
                                         &&std::is_convertible_v<S, std::string> >;

        ret_val_common( const std::string &msg, bool succ )
            : msg( msg ), succ( succ )
        {}
    private:
        std::string msg;
        bool succ;
};

/**
 * The class represents a composite return value of an arbitrary function (result).
 * 'Composite' means that apart from the value itself, the result also contains:
 *   - a boolean flag which indicates the binary state of the result: success/failure.
 *   - an arbitrary string which additionally describes the result (may be empty).
 *
 * Template parameter T specifies the type of the value.
 */

template<typename T>
class ret_val : public ret_val_common
{
        static_assert( !std::is_convertible_v<T, std::string>, "string values aren't allowed" );

    public:
        /**
         * These structures are mandatory only if you want to omit the explicit return
         * values of type T when creating instances of @ref ret_val with make_ functions.
         * Each of these structures must contain a static constexpr member named 'value'.
         */
        struct default_success;
        struct default_failure;

        ret_val() = delete;

        static ret_val make_success( T val = default_success::value ) {
            return make_success( val, std::string() );
        }

        static ret_val make_failure( T val = default_failure::value ) {
            return make_failure( val, std::string() );
        }

        template<class... A, typename S = std::string, typename = is_convertible_to_string<S>>
        static ret_val make_success( T val, const S &msg, A && ... args ) {
            return ret_val( string_format( msg, std::forward<A>( args )... ), val, true );
        }

        template<class... A, typename S = std::string, typename = is_convertible_to_string<S>>
        static ret_val make_failure( T val, const S &msg, A && ... args ) {
            return ret_val( string_format( msg, std::forward<A>( args )... ), val, false );
        }

        template<class... A, typename S = std::string, typename = is_convertible_to_string<S>>
        static ret_val make_success( const S &msg, A && ... args ) {
            return make_success( default_success::value, msg, std::forward<A>( args )... );
        }

        template<class... A, typename S = std::string, typename = is_convertible_to_string<S>>
        static ret_val make_failure( const S &msg, A && ... args ) {
            return make_failure( default_failure::value, msg, std::forward<A>( args )... );
        }

        T value() const {
            return val;
        }

    private:
        ret_val( const std::string &msg, T val, bool succ )
            : ret_val_common( msg, succ ), val( val ) {}

        T val;
};

template<>
class ret_val<void> : public ret_val_common
{
    public:
        static ret_val make_success() {
            return make_success( std::string() );
        }

        static ret_val make_failure() {
            return make_failure( std::string() );
        }

        template<class... A, typename S = std::string, typename = is_convertible_to_string<S>>
        static ret_val make_success( const S &msg, A && ... args ) {
            return ret_val( string_format( msg, std::forward<A>( args )... ), true );
        }

        template<class... A, typename S = std::string, typename = is_convertible_to_string<S>>
        static ret_val make_failure( const S &msg, A && ... args ) {
            return ret_val( string_format( msg, std::forward<A>( args )... ), false );
        }
    private:
        using ret_val_common::ret_val_common;
};

#endif // CATA_SRC_RET_VAL_H
