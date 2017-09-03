#pragma once
#ifndef RET_VAL_H
#define RET_VAL_H

#include "debug.h"

#include <string>
#include <type_traits>

template<typename T, typename std::enable_if<std::is_same<T, bool>::value, int>::type = 0>
struct def_success_const : std::integral_constant<bool, true> {};

template<typename T, typename std::enable_if<std::is_same<T, bool>::value, int>::type = 0>
struct def_failure_const : std::integral_constant<bool, false> {};

template<typename T, T default_success = def_success_const<T>::value, T default_failure = def_failure_const<T>::value>
class ret_val
{
    public:
        static_assert( default_success != default_failure, "Codes for success and failure must differ." );

        ret_val() = delete;

        static ret_val make_success( const std::string &msg = std::string() ) {
            return ret_val( default_success, msg );
        }

        static ret_val make_failure( const std::string &msg, T val = default_failure ) {
            if( val == default_success ) {
                debugmsg( "Tried to declare failure using the code for success." );
                return make_failure( default_failure );
            }
            return ret_val( val, msg );
        }

        static ret_val make_failure( T val = default_failure ) {
            return make_failure( std::string(), val );
        }

        bool success() const {
            return val == default_success;
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
        ret_val( T val, const std::string &msg ) : msg( msg ), val( val ) {}

    private:
        std::string msg;
        T val;
};

#endif // RET_VAL_H
