#include "output_base.h"

#include "debug.h"

#include <cstring>
#include <cstdio>

//--------------------------------------------------------------------------------------------------
namespace {

#if defined(_MSC_VER)
void string_copy(char* const dst, size_t const dst_size, char const* src) {
    strncpy_s(dst, dst_size, src, _TRUNCATE);
}
#else
void string_copy(char* const dst, size_t const dst_size, char const* src) {
    auto const src_size = strlen(src);
    strncpy(dst, src, std::min(dst_size, src_size));
}
#endif

void set_error_string(char *const buffer, size_t const buffer_size,
    char const* const format, int const e_code)
{
    string_copy(buffer, buffer_size, "Error when formatting string.");

    DebugLog(D_WARNING, D_GAME) << "Error '" << strerror(e_code) <<
        "' when formatting string '" << format << "'.";
}
} //namespace

#if defined(_MSC_VER)
//--------------------------------------------------------------------------------------------------
// MSVC implementation.
//--------------------------------------------------------------------------------------------------
size_t detail::sprintf_result_base::try_format(char *const buffer, size_t const buffer_size,
    char const *const format, va_list args)
{
    int const result = _vscprintf_p(format, args);
    if (result == -1) {
        set_error_string(buffer, buffer_size, format, errno);
        return 0;
    }

    auto const required_size = static_cast<size_t>(result + 1);

    if (required_size > buffer_size) {
        return required_size;
    }

    _vsprintf_p(buffer, buffer_size, format, args);

    return 0;
}
#else
//--------------------------------------------------------------------------------------------------
// General implementation; no positional arguments on MSVC
//--------------------------------------------------------------------------------------------------
size_t detail::sprintf_result_base::try_format(char *const buffer, size_t const buffer_size,
    char const *const format, va_list args)
{
    errno = 0;

    va_list args_copy;
    va_copy(args_copy, args);
    auto const result = vsnprintf(buffer, buffer_size, format, args_copy);
    va_end(args_copy);

    if (result < 0) {
        if (errno) {
            set_error_string(buffer, buffer_size, format, errno);
            return 0;
        } else {
            return buffer_size * 2;
        }
    }

    auto const size = static_cast<size_t>(result);
    if (size >= buffer_size) {
        return size + 1;
    }

    return 0;
}
#endif

//--------------------------------------------------------------------------------------------------
std::string string_format(char const *pattern, ...)
{
    va_list ap;
    va_start(ap, pattern);
    auto result = vstring_format(pattern, ap);
    va_end(ap);

    return result;
}

//--------------------------------------------------------------------------------------------------
std::string vstring_format(char const *pattern, va_list argptr)
{
    return sprintf_string_backed {pattern, argptr}.to_string();
}


//--------------------------------------------------------------------------------------------------
sprintf_array_backed buffer_format(char const *pattern, ...)
{
    va_list ap;
    va_start(ap, pattern);
    auto result = vbuffer_format(pattern, ap);
    va_end(ap);

    return result;
}

//--------------------------------------------------------------------------------------------------
sprintf_array_backed vbuffer_format(char const *pattern, va_list argptr)
{
    return sprintf_array_backed {pattern, argptr};
}
