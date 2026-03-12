#if defined(EMSCRIPTEN)
#include <emscripten/bind.h>

std::string getExceptionMessage( intptr_t exceptionPtr )
{
    return std::string( reinterpret_cast<std::exception *>( exceptionPtr )->what() );
}

EMSCRIPTEN_BINDINGS( Bindings )
{
    emscripten::function( "getExceptionMessage", &getExceptionMessage );
}
#endif
