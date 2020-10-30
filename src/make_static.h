#ifndef CATA_SRC_MAKE_STATIC_H
#define CATA_SRC_MAKE_STATIC_H

/**
 *  The purpose of this macro is to provide a concise syntax for
 *  creation of inline literals (std::string, string_id, etc) that
 *  also have static scope.
 *
 *  Usage:
 *    instead of:  some_method( "string_constant" );
 *    use:         some_method( STATIC ( "string_constant" ) );
 *
 *    instead of:  some_method( string_id<T>( "id_constant" ) );
 *    use:         some_method( STATIC ( string_id<T>( "id_constant" ) ) );
 *
 *    const V &v = STATIC( expr );
 *    // is an equivalent of:
 *    static const V v = expr;
 *
 *    Note: `expr` being char* (i.e. "a string literal") is a special case:
 *      for such type of argument,  STATIC(expr) has `const std::string &` result type.
 *      This is to support expressions like STATIC("...") instead of STATIC(std::string("..."))
 */
#define STATIC(expr) \
    (([]()-> const auto &{ \
        using CachedType = std::conditional_t<std::is_same<std::decay_t<decltype(expr)>, const char*>::value, \
                           std::string, std::decay_t<decltype(expr)>>; \
        static const CachedType _cached_expr = (expr); \
        return _cached_expr; \
    })())

#endif // CATA_SRC_MAKE_STATIC_H
