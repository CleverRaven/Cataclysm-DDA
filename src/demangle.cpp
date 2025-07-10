#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
#    include <cstdlib>  // for free
#    include <cxxabi.h>
#endif

#include "demangle.h"

std::string demangle( const char *symbol )
{
#if defined(_MSC_VER)
    // TODO: implement demangling on MSVC
#elif defined(__GNUC__) || defined(__clang__)
    int status = -1;
    char *demangled = abi::__cxa_demangle( symbol, nullptr, nullptr, &status );
    if( status == 0 ) {
        std::string demangled_str( demangled );
        free( demangled );
        return demangled_str;
    }
#if defined(_WIN32)
    // https://stackoverflow.com/questions/54333608/boost-stacktrace-not-demangling-names-when-cross-compiled
    // libbacktrace may strip leading underscore character in the symbol name returned
    // so if demangling failed, try again with an underscore prepended
    std::string prepend_underscore( "_" );
    prepend_underscore = prepend_underscore + symbol;
    demangled = abi::__cxa_demangle( prepend_underscore.c_str(), nullptr, nullptr, &status );
    if( status == 0 ) {
        std::string demangled_str( demangled );
        free( demangled );
        return demangled_str;
    }
#endif // defined(_WIN32)
#endif // compiler macros
    return std::string( symbol );
}
