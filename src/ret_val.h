#pragma once
#ifndef CATA_SRC_RET_VAL_H
#define CATA_SRC_RET_VAL_H

#include <string>
#include <type_traits>

#include "string_formatter.h"

/**
 * The class represents a composite return value of an arbitrary function (result).
 * 'Composite' means that apart from the value itself, the result also contains:
 *   - a boolean flag which indicates the binary state of the result: success/failure.
 *   - an arbitrary string which additionally describes the result (may be empty).
 *
 * Template parameter T specifies the type of the value.
 */

template<typename T>
class ret_val
{
        static_assert( !std::is_convertible<T, std::string>::value, "string values aren't allowed" );

        template<typename S>
        using is_convertible_to_string = typename
                                         std::enable_if< std::is_convertible<S, std::string>::value>::type;

    public:
        /**
         * These structures are mandatory only if you want to omit the explicit return
         * values of type T when creating instances of @ref ret_val with make_ functions.
         * Each of these structures must contain a static constexpr member named 'value'.
         * The recommended way of achieving this is inheriting the specializations from
         * @ref std::integral_constant. See the specialization for 'bool' (below) as an example.
         */
        struct default_success;
        struct default_failure;

    public:
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

        bool success() const {
            return succ;
        }

        T value() const {
            return val;
        }

        const std::string &str() const {
            return msg;
        }

        const char *c_str() const {
            return msg.c_str();
        }

    protected:
        ret_val( const std::string &msg, T val, bool succ ) : msg( msg ), val( val ), succ( succ ) {}

    private:
        std::string msg;
        T val;
        bool succ;
};

template<>
struct ret_val<bool>::default_success : public std::integral_constant<bool, true> {};

template<>
struct ret_val<bool>::default_failure : public std::integral_constant<bool, false> {};

#endif // CATA_SRC_RET_VAL_H
