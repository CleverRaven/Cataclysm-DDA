#pragma once
#ifndef CATA_SRC_MAKE_STATIC_H
#define CATA_SRC_MAKE_STATIC_H

template<typename T>
inline const T &static_argument_identity( const T &t )
{
    return t;
}

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
        using ExprType = std::decay_t<decltype(static_argument_identity( expr ))>; \
        using CachedType = std::conditional_t<std::is_same_v<ExprType, const char*>, \
                           std::string, ExprType>; \
        static const CachedType _cached_expr = static_argument_identity( expr ); \
        return _cached_expr; \
    })())

#endif // CATA_SRC_MAKE_STATIC_H
