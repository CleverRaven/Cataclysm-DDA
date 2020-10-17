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
 */
#define STATIC(expr) \
    ([]()-> const auto &{ \
        static const auto __ret = expr; \
        return __ret; \
    })()

#endif // CATA_SRC_MAKE_STATIC_H
