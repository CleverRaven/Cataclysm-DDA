#ifndef CATA_OUTPUT_BASE_H
#define CATA_OUTPUT_BASE_H

#include <array>
#include <string>
#include <cstdarg>
#include <cassert>

namespace detail {

//--------------------------------------------------------------------------------------------------
//! A buffer optimised for making into a std::string.
//! Use a string's internal buffer.
//--------------------------------------------------------------------------------------------------
struct sprintf_string_buffer {
    enum : size_t { buffer_size = 15 }; // size of the small string optimization on MSVC.

    void resize(size_t const new_size) {        
        str.resize(new_size, '\0');
    }
    
    char* data() noexcept {
        return &str[0];
    }

    char const* data() const noexcept {
        return &str[0];
    }

    size_t size() const noexcept {
        return str.size();
    }

    std::string to_string() {
        return std::move(str);
    }

    explicit sprintf_string_buffer(size_t const new_size = buffer_size)
      : str(new_size, '\0')
    {
    }

    std::string str;
};

//--------------------------------------------------------------------------------------------------
//! A buffer optimised for use as a C string. Relies on RVO to be most efficient.
//! First creates a std::array on the stack which, once full, is "swapped" out for a std::string's
//! internal buffer. Implemented as a discriminated union.
//--------------------------------------------------------------------------------------------------
struct sprintf_array_buffer {
    enum : size_t { buffer_size = 512 };

    using array_t = std::array<char, buffer_size>;

    void resize(size_t const new_size) {
        assert(new_size >= cur_size);

        if (cur_size > buffer_size) {
            str.resize(new_size, '\0');
            data_ptr = &str[0];
        } else if (new_size > buffer_size) {
            new (&str) std::string(new_size, '\0');
            data_ptr = &str[0];
        }

        cur_size = new_size;
    }
    
    char* data() noexcept {
        return data_ptr;
    }

    char const* data() const noexcept {
        return data_ptr;
    }

    size_t size() const noexcept {
        return cur_size;
    }

    std::string to_string() {
        if (cur_size <= buffer_size) {
            return std::string(&arr[0], cur_size);
        } else {
            return std::move(str);
        }
    }

    explicit sprintf_array_buffer(size_t const new_size = buffer_size) noexcept
      : cur_size {new_size}
    {
        new (&arr) array_t; // uninitialized memory;
        arr[0] = '\0';      // make sure to null terminate it.
        data_ptr = &arr[0];
    }

    // Not meant to be used outside the implementation
    void construct(sprintf_array_buffer &&other) noexcept {
        if (other.cur_size > buffer_size) {
            new (&str) std::string(std::move(other.str));
            data_ptr = &str[0];
        } else {
            new (&arr) array_t(std::move(other.arr));
            data_ptr = &arr[0];
        }

        cur_size = other.cur_size;
    }

    sprintf_array_buffer(sprintf_array_buffer &&other) noexcept {
        construct(std::move(other));
    }

    // Very likely to be dead code in an optimized build; needed anyway to compile.
    sprintf_array_buffer& operator=(sprintf_array_buffer &&rhs) noexcept {
        assert(this != &rhs);
               
        this->~sprintf_array_buffer();
        construct(std::move(rhs));

        return *this;
    }

    sprintf_array_buffer(sprintf_array_buffer const&) = delete;
    sprintf_array_buffer& operator=(sprintf_array_buffer const&) = delete;

    ~sprintf_array_buffer() {
        if (cur_size > buffer_size) {
            str.~basic_string();
        }
    }

    size_t cur_size; // cur_size <= buffer_size indicated arr is in use; otherwise str.
    char*  data_ptr; // just to avoid a branch on every call to data().

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable:4624) //destructor implicitly deleted
#endif
    union {
        array_t     arr;
        std::string str;
    };
#if defined(_MSC_VER)
#   pragma warning(pop)
#endif
};

//--------------------------------------------------------------------------------------------------
//! Only here as a namespace of sorts.
//--------------------------------------------------------------------------------------------------
struct sprintf_result_base {
    // Tries to format using buffer.
    // Returns 0 on error or success, otherwise, returns a bigger buffer size to attempt.
    static size_t try_format(char *buffer, size_t buffer_size, char const *format, va_list args);
};

//--------------------------------------------------------------------------------------------------
//! An efficient and safe wrapper around (v)s(n)printf function calls.
//! Can be configured via Buffer for two use cases: an efficient std::string result,
//! or an efficient C string result (just a buffer, really).
//--------------------------------------------------------------------------------------------------
template <typename Buffer>
struct sprintf_result {
    sprintf_result() = default;                                // Default default.
    sprintf_result(sprintf_result&&) = default;                // Default move.
    sprintf_result& operator=(sprintf_result&&) = default;     // Default move.
    sprintf_result(sprintf_result const&) = delete;            // No copy.
    sprintf_result& operator=(sprintf_result const&) = delete; // No copy.

    sprintf_result(char const* const format, va_list args)
      : buffer {}
    {
        while (auto const result = sprintf_result_base::try_format(buffer.data(), buffer.size(),
            format, args))
        {
            buffer.resize(result);
        }
    }

    char const* c_str() const noexcept {
        return buffer.data();
    }

    std::string to_string() {
        return buffer.to_string();
    }

    template <typename Stream>
    friend Stream& operator<<(Stream &lhs, sprintf_result const &rhs) {
        //TODO: could restrict this with a static_cast, but would need another include...
        lhs << rhs.c_str();
        return lhs;
    }

    Buffer buffer;
};

} //namespace detail

using sprintf_array_backed  = detail::sprintf_result<detail::sprintf_array_buffer>;
using sprintf_string_backed = detail::sprintf_result<detail::sprintf_string_buffer>;

std::string string_format(char const *pattern, ...);
std::string vstring_format(char const *pattern, va_list argptr);

sprintf_array_backed buffer_format(char const *pattern, ...);
sprintf_array_backed vbuffer_format(char const *pattern, va_list argptr);

#endif // CATA_OUTPUT_BASE_H
