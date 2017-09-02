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

        static ret_val success( const std::string &msg = std::string() ) {
            return ret_val( default_success, msg );
        }

        static ret_val failure( const std::string &msg, T code = default_failure ) {
            if( code == default_success ) {
                debugmsg( "Tried to declare failure using the code for success." );
                return failure( default_failure );
            }
            return ret_val( code, msg );
        }

        static ret_val failure( T code = default_failure ) {
            return failure( std::string(), code );
        }

        operator bool() const {
            return code == default_success;
        }

        T operator *() const {
            return code;
        }

        const std::string &str() const {
            return msg;
        }

        const char *c_str() const {
            return msg.c_str();
        }

    protected:
        ret_val( T code, const std::string &msg ) : msg( msg ), code( code ) {}

    private:
        std::string msg;
        T code;
};

#endif // RET_VAL_H
