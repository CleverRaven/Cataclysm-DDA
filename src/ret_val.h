#pragma once
#ifndef RET_VAL_H
#define RET_VAL_H

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

template<typename T, T default_success = T(), T default_failure = T()>
class ret_val
{
    public:
        ret_val() = delete;

        static ret_val make_success( const std::string &msg, T val = default_success ) {
            return ret_val( msg, val, true );
        }

        static ret_val make_failure( const std::string &msg, T val = default_failure ) {
            return ret_val( msg, val, false );
        }

        static ret_val make_success( T val = default_success ) {
            return make_success( std::string(), val );
        }

        static ret_val make_failure( T val = default_failure ) {
            return make_failure( std::string(), val );
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
