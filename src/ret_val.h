#pragma once
#ifndef RET_VAL_H
#define RET_VAL_H

#include "output.h"
#include "printf_check.h"

#include <string>
#include <type_traits>

/**
 * The class represents a composite return value of an arbitrary function (result).
 * 'Composite' means that apart from the value itself, the result also contains:
 *   - a boolean flag which indicates the binary state of the result: success/failure.
 *   - an arbitrary string which additionally describes the result (may be empty).
 *
 * Template parameter T specifies the type of the value.
 * 'default_success' and 'default_failure' are optional constants that would be used
 * as defaults for the success/failure values respectively.
 */

template<typename S>
using _is_convertible_to_string =
    typename std::enable_if< std::is_convertible<S, std::string>::value>::type;


template<typename T, T default_success = T(), T default_failure = T()>
class ret_val
{
    public:
        ret_val() = delete;

        template<class... A, typename S = std::string, typename = _is_convertible_to_string<S>>
        static ret_val make_success( T val, const S &msg = S(),
                                     A && ... args ) PRINTF_LIKE( 2, 3 ) {
            return ret_val( string_format( msg, std::forward<A>( args )... ), val, true );
        }

        template<class... A, typename S = std::string, typename = _is_convertible_to_string<S>>
        static ret_val make_failure( T val, const S &msg = S(),
                                     A && ... args ) PRINTF_LIKE( 2, 3 ) {
            return ret_val( string_format( msg, std::forward<A>( args )... ), val, false );
        }

        template<class... A, typename S = std::string, typename = _is_convertible_to_string<S>>
        static ret_val make_success( const S &msg = S(), A && ... args ) PRINTF_LIKE( 1, 2 ) {
            return make_success( default_success, msg, std::forward<A>( args )... );
        }

        template<class... A, typename S = std::string, typename = _is_convertible_to_string<S>>
        static ret_val make_failure( const S &msg = S(), A && ... args ) PRINTF_LIKE( 1, 2 ) {
            return make_failure( default_failure, msg, std::forward<A>( args )... );
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
struct ret_val<bool, true, false> {};

#endif // RET_VAL_H
