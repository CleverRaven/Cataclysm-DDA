// RUN: %check_clang_tidy %s cata-u8-path %t -- --load=%cata_plugin --

// check_clang_tidy uses -nostdinc++, so we add dummy declarations here
namespace std
{
namespace filesystem
{
class path
{
    public:
        enum format {
            native_foramt,
            generic_format,
            auto_format
        };

        path() noexcept;
        path( const path &p );
        path( path &&p ) noexcept;

        template<typename Source>
        path( const Source &source, format fmt = auto_format );
        template<typename InputIt>
        path( InputIt first, InputIt last, format fmt = auto_format );

        path &operator=( const path &p );
        path &operator=( path &&p ) noexcept;
        template<typename Source>
        path &operator=( const Source &source );
        template<typename Source>
        path &assign( const Source &source );
        template<typename InputIt>
        path &assign( InputIt first, InputIt last );

        path &operator/=( const path &p );
        template<typename Source>
        path &operator/=( const Source &source );
        template<typename Source>
        path &append( const Source &source );
        template<typename InputIt>
        path &append( InputIt first, InputIt last );

        path &operator+=( const path &p );
        template<typename Source>
        path &operator+=( const Source &source );
        template<typename Source>
        path &concat( const Source &source );
        template<typename InputIt>
        path &concat( InputIt first, InputIt last );
};

path operator/( const path &lhs, const path &rhs );

template<typename Source>
path u8path( const Source &source )
{
    // Not the correct implementation, but we only care about checking that
    // warnings are not generated inside the filesystem namespace.
    return path{ source };
}
template<typename InputIt>
path u8path( InputIt first, InputIt last )
{
    // Not the correct implementation, but we only care about checking that
    // warnings are not generated inside the filesystem namespace.
    return path{ first, last };
}
} // namespace filesystem

template<typename T>
struct remove_reference {
    using type = T;
};

template<typename T>
struct remove_reference<T &> {
    using type = T;
};

template<typename T>
struct remove_reference < T && > {
    using type = T;
};

template<typename T>
constexpr typename remove_reference<T>::type &&move( T &&t ) noexcept;

} // namespace std

namespace fs = std::filesystem;

void write_to_file( const fs::path &p );
fs::path get_root_dir();

static void call_fs_path_method( fs::path & ( fs::path::* method )( const char *const & ) )
{
    fs::path p;
    ( p.*method )( "foo" );
}

class path_inherit : public fs::path
{
};

class path_using : public fs::path
{
    public:
        using fs::path::path;
        using fs::path::operator=;
        using fs::path::assign;
};

void test_cases()
{
    const char *const foo = "foo";

    fs::path path_u8_cstr = fs::u8path( foo );
    fs::path path_u8_pchar = fs::u8path( foo, foo + 3 );
    write_to_file( path_u8_cstr );
    fs::path path_op_join_paths = path_u8_cstr / path_u8_pchar;
    static_cast<void>( path_op_join_paths );
    // Empty fs::path is acceptable
    fs::path();
    fs::path path_empty;
    // Construction from existing path is acceptable
    fs::path path_copy_ctor = path_u8_cstr;
    fs::path path_move_ctor = std::move( path_copy_ctor );
    fs::path path_elision_ctor = get_root_dir();
    static_cast<void>( path_elision_ctor );

    fs::path( "bar" );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    fs::path path_str_literal = "foo";
    // CHECK-MESSAGES: [[@LINE-1]]:33: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    fs::path path_cstr = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:26: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    fs::path( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    using fs_path = fs::path;
    fs_path{ foo };
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    write_to_file( "baz" );
    // CHECK-MESSAGES: [[@LINE-1]]:20: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    fs::path path_op_join_path_cstr = path_u8_cstr / foo;
    // CHECK-MESSAGES: [[@LINE-1]]:54: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    fs::path path_op_join_str_literal_path = ".." / path_u8_pchar;
    // CHECK-MESSAGES: [[@LINE-1]]:46: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    std::filesystem::operator/( "foo", "bar" );
    // CHECK-MESSAGES: [[@LINE-1]]:33: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    // CHECK-MESSAGES: [[@LINE-2]]:40: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    static_cast<void>( path_op_join_path_cstr );
    static_cast<void>( path_op_join_str_literal_path );

    fs::path path_op_copy_assign;
    path_op_copy_assign = path_u8_cstr;
    fs::path path_op_move_assign;
    path_op_move_assign = std::move( path_op_copy_assign );

    fs::path path_op_assign_cstr;
    path_op_assign_cstr = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs_path() = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:17: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_op_assign_str_literal;
    path_op_assign_str_literal.operator = ( "foo" );
    // CHECK-MESSAGES: [[@LINE-1]]:45: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_assign_str_literal;
    path_assign_str_literal.assign( "foo" );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_assign_pchar;
    path_assign_pchar.assign( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs_path().assign( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.

    fs::path path_op_append_path;
    path_op_append_path /= path_assign_pchar;

    fs::path path_op_append_str_literal;
    path_op_append_str_literal /= "foo";
    // CHECK-MESSAGES: [[@LINE-1]]:35: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_op_append_cstr;
    path_op_append_cstr.operator /= ( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:39: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_append_cstr;
    path_append_cstr.append( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_append_pchar;
    path_append_pchar.append( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.

    fs::path path_op_concat_path;
    path_op_concat_path += path_assign_pchar;

    fs::path path_op_concat_str_literal;
    path_op_concat_str_literal += "foo";
    // CHECK-MESSAGES: [[@LINE-1]]:35: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_op_concat_cstr;
    path_op_concat_cstr.operator += ( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:39: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_concat_cstr;
    path_concat_cstr.concat( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_concat_pchar;
    path_concat_pchar.concat( foo, foo + 3 );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.

    fs::path &( fs::path::* fn_op_assign_path )( const fs::path & ) = &fs::path::operator=;

    fs::path &( fs::path::* fn_op_assign_cstr )( const char *const & ) = &fs::path::operator=;
    // CHECK-MESSAGES: [[@LINE-1]]:75: warning: Use the operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs_path &( fs_path::* )( const char *const & )>
                       ( &fs_path::operator= ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path &( fs::path::* fn_assign_cstr )( const char *const & ) = &fs::path::assign;
    // CHECK-MESSAGES: [[@LINE-1]]:72: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs_path &( fs_path::* )( const char *const & )>
                       ( &fs_path::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    // Non-ambiguous
    fs::path &( fs::path::* fn_op_assign_cstr_expl )( const char *const & ) =
        &fs::path::operator+=<const char *>;
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: Use the operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    // Ambiguous and resolved by the lhs type
    fs::path &( fs::path::* fn_assign_cstr_expl )( const char *const & ) =
        &fs::path::concat<const char *>;
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    call_fs_path_method( &fs::path::operator/= );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    call_fs_path_method( &fs::path::append );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    fs::path path_assign_cstr;
    ( path_assign_cstr.*fn_op_assign_cstr )( foo );
    ( path_assign_cstr.*fn_assign_cstr )( foo );
    ( path_assign_cstr.*fn_op_assign_cstr_expl )( foo );
    ( path_assign_cstr.*fn_assign_cstr_expl )( foo );

    static_cast<void>( path_inherit{ foo } );
    // CHECK-MESSAGES: [[@LINE-1]]:38: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    path_inherit().assign( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs::path &( path_inherit::* )( const char *const & )>
                       ( &path_inherit::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs::path &( fs::path::* )( const char *const & )>
                       ( &path_inherit::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    // FIXME: Ideally this should also be detected, but I'm not sure if it is worth the effort.
    path_using{ foo };
    // CHECK-MESSAGES-NOT: [[@LINE-1]]:5: warning: Construct `fs::path` by passing UTF-8 string to `fs::u8path` to ensure the correct path encoding.
    path_using() = foo;
    // CHECK-MESSAGES: [[@LINE-1]]:20: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    path_using().operator = ( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:31: warning: Modify `fs::path` using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    path_using().assign( foo );
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: Modify `fs::path` using `=`, `/=`, and `+=` and parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs::path &( path_using::* )( const char *const & )>
                       ( &path_using::operator= ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs::path &( path_using::* )( const char *const & )>
                       ( &path_using::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs::path &( fs::path::* )( const char *const & )>
                       ( &path_using::operator= ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
    static_cast<void>( static_cast<fs::path &( fs::path::* )( const char *const & )>
                       ( &path_using::assign ) );
    // CHECK-MESSAGES: [[@LINE-1]]:27: warning: Use the `=`, `/=`, or `+=` operator overload with `fs::path` parameter and call it using parameter constructed with `fs::u8path` and UTF-8 string to ensure the correct path encoding.
}
