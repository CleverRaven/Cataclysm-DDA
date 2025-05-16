// RUN: %check_clang_tidy -allow-stdinc %s cata-u8-path %t -- --load=%cata_plugin -- -isystem %cata_include/third-party

#include <filesystem>

void write_to_file( const std::filesystem::path &p );
std::filesystem::path get_root_dir();

static void call_fs_path_method( std::filesystem::path & ( std::filesystem::path::* method )(
                                     const char *const & ) )
{
    std::filesystem::path p;
    ( p.*method )( "foo" );
}

class path_inherit : public std::filesystem::path
{
};

class path_using : public std::filesystem::path
{
    public:
        using std::filesystem::path::path;
        using std::filesystem::path::operator=;
        using std::filesystem::path::assign;
};

void test_cases()
{
    const char *const foo = "foo";

    std::filesystem::path path_u8_cstr = std::filesystem::u8path( foo );
    std::filesystem::path path_u8_pchar = std::filesystem::u8path( foo, foo + 3 );
    write_to_file( path_u8_cstr );
    std::filesystem::path path_op_join_paths = path_u8_cstr / path_u8_pchar;
    static_cast<void>( path_op_join_paths );
    // Empty std::filesystem::path is acceptable
    std::filesystem::path();
    std::filesystem::path path_empty;
    // Construction from existing path is acceptable
    std::filesystem::path path_copy_ctor = path_u8_cstr;
    std::filesystem::path path_move_ctor = std::move( path_copy_ctor );
    std::filesystem::path path_elision_ctor = get_root_dir();
    static_cast<void>( path_elision_ctor );

    std::filesystem::path( "bar" );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    std::filesystem::path path_str_literal = "foo";
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    std::filesystem::path path_cstr = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    std::filesystem::path( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    using fs_path = std::filesystem::path;
    fs_path{ foo };
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    write_to_file( "baz" );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    std::filesystem::path path_op_join_path_cstr = path_u8_cstr / foo;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    std::filesystem::path path_op_join_str_literal_path = ".." / path_u8_pchar;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    static_cast<void>( path_op_join_path_cstr );
    static_cast<void>( path_op_join_str_literal_path );

    std::filesystem::path path_op_copy_assign;
    path_op_copy_assign = path_u8_cstr;
    std::filesystem::path path_op_move_assign;
    path_op_move_assign = std::move( path_op_copy_assign );

    std::filesystem::path path_op_assign_cstr;
    path_op_assign_cstr = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    fs_path() = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_op_assign_str_literal;
    path_op_assign_str_literal.operator = ( "foo" );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_assign_str_literal;
    path_assign_str_literal.assign( "foo" );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_assign_pchar;
    path_assign_pchar.assign( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    fs_path().assign( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.

    std::filesystem::path path_op_append_path;
    path_op_append_path /= path_assign_pchar;

    std::filesystem::path path_op_append_str_literal;
    path_op_append_str_literal /= "foo";
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_op_append_cstr;
    path_op_append_cstr.operator /= ( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_append_cstr;
    path_append_cstr.append( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_append_pchar;
    path_append_pchar.append( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.

    std::filesystem::path path_op_concat_path;
    path_op_concat_path += path_assign_pchar;

    std::filesystem::path path_op_concat_str_literal;
    path_op_concat_str_literal += "foo";
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_op_concat_cstr;
    path_op_concat_cstr.operator += ( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_concat_cstr;
    path_concat_cstr.concat( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_concat_pchar;
    path_concat_pchar.concat( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.

    std::filesystem::path &( std::filesystem::path::* fn_op_assign_path )(
        const std::filesystem::path & ) = &std::filesystem::path::operator=;

    std::filesystem::path &( std::filesystem::path::* fn_op_assign_cstr )(
        const char *const & ) = &std::filesystem::path::operator=;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs_path &( fs_path::* )( const char *const & )>
                       ( &fs_path::operator= ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path &( std::filesystem::path::* fn_assign_cstr )( const char *const & ) =
        &std::filesystem::path::assign;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs_path &( fs_path::* )( const char *const & )>
                       ( &fs_path::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    // Non-ambiguous
    std::filesystem::path &( std::filesystem::path::* fn_op_assign_cstr_expl )( const char *const & ) =
        &std::filesystem::path::operator+=<const char *>;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    // Ambiguous and resolved by the lhs type
    std::filesystem::path &( std::filesystem::path::* fn_assign_cstr_expl )( const char *const & ) =
        &std::filesystem::path::concat<const char *>;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    call_fs_path_method( &std::filesystem::path::operator/= );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    call_fs_path_method( &std::filesystem::path::append );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    std::filesystem::path path_assign_cstr;
    ( path_assign_cstr.*fn_op_assign_cstr )( foo );
    ( path_assign_cstr.*fn_assign_cstr )( foo );
    ( path_assign_cstr.*fn_op_assign_cstr_expl )( foo );
    ( path_assign_cstr.*fn_assign_cstr_expl )( foo );

    static_cast<void>( path_inherit{ foo } );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    path_inherit().assign( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<std::filesystem::path &( path_inherit::* )( const char *const & )>
                       ( &path_inherit::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>
    ( static_cast<std::filesystem::path &( std::filesystem::path::* )( const char *const & )>
      ( &path_inherit::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    // FIXME: Ideally this should also be detected, but I'm not sure if it is worth the effort.
    path_using{ foo };
    // CHECK-MESSAGES-NOT: [[@LINE-1]]:{{[0-9]+}}: warning: Construct `std::filesystem::path` by passing UTF-8 string to `std::filesystem::u8path` to ensure the correct path encoding.
    path_using() = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    path_using().operator = ( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    path_using().assign( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Modify `std::filesystem::path` using `=`, `/=`, and `+=` and parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<std::filesystem::path &( path_using::* )( const char *const & )>
                       ( &path_using::operator= ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<std::filesystem::path &( path_using::* )( const char *const & )>
                       ( &path_using::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>
    ( static_cast<std::filesystem::path &( std::filesystem::path::* )( const char *const & )>
      ( &path_using::operator= ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>
    ( static_cast<std::filesystem::path &( std::filesystem::path::* )( const char *const & )>
      ( &path_using::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:{{[0-9]+}}: warning: Use the `=`, `/=`, or `+=` operator overload with `std::filesystem::path` parameter and call it using parameter constructed with `std::filesystem::u8path` and UTF-8 string to ensure the correct path encoding.
}
