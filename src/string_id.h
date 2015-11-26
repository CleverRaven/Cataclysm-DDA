#ifndef STRING_ID_H
#define STRING_ID_H

#include <string>
#include <type_traits>

template<typename T>
class int_id;

struct null_id_type;

/**
 * This represents an identifier (implemented as std::string) of some object.
 * It can be used for all type of objects, one just needs to specify a type as
 * template parameter T, which separates the different identifier types.
 *
 * The constructor is explicit on purpose, you have to write
 * \code
 * auto someid = mtype_id("mon_dog");
 * if( critter.type->id == mtype_id("mon_cat") ) { ...
 * \endcode
 * This allows to find all ids that are initialized from static literals (and not
 * through the json loading). The can than easily be checked against the json
 * definition to see whether they are actually defined there (or whether they are
 * defined only in a mod).
 *
 * Example:
 * \code
 * struct itype;
 * using itype_id = string_id<itype>;
 * struct mtype;
 * using mtype_id = string_id<mtype>;
 * \endcode
 * The types mtype_id and itype_id declared here are separate, the compiler will not
 * allow assignment / comparison of mtype_id and itype_id.
 * Note that for this to work, the template parameter type does note even need to be
 * known when the string_id is used. In fact, it does not even need to be defined at all,
 * a declaration is just enough.
 */
template<typename T>
class string_id
{
    public:
        typedef string_id<T> This;

        /**
         * Forwarding constructor, forwards any parameter to the std::string
         * constructor to create the id string. This allows plain C-strings,
         * and std::strings to be used.
         */
        // Beautiful C++11: enable_if makes sure that S is always something that can be used to constructor
        // a std::string, otherwise a "no matching function to call..." error is generated.
        template<typename S, class = typename
                 std::enable_if< std::is_convertible<S, std::string >::value>::type >
        explicit string_id( S && id ) : _id( std::forward<S>( id ) ) {
        }
        /**
         * Default constructor constructs an empty id string.
         * Note that this id class does not enforce empty id strings (or any specific string at all)
         * to be special. Every string (including the empty one) may be a valid id.
         */
        string_id() : _id() {
        }
        /**
         * Create a copy of the @ref NULL_ID. See @ref null_id_type.
         */
        string_id( const null_id_type & ) : _id( NULL_ID._id ) {
        }
        /* This is here to appease clang, which thinks there is some ambiguity in
        `string_id<T> X = NULL_ID;`, gcc accepts it, but clang can not decide between implicit
        move assignment operator and implicit copy assignment operator. */
        This &operator=( const null_id_type & ) {
            return *this = NULL_ID;
        }
        /**
         * Comparison, only useful when the id is used in std::map or std::set as key. Compares
         * the string id as with the strings comparison.
         */
        bool operator<( const This &rhs ) const {
            return _id < rhs._id;
        }
        /**
         * The usual comparator, compares the string id as usual.
         */
        bool operator==( const This &rhs ) const {
            return _id == rhs._id;
        }
        /**
         * The usual comparator, compares the string id as usual.
         */
        bool operator!=( const This &rhs ) const {
            return _id != rhs._id;
        }

        /**
         * Interface to the plain C-string of the id. This function mimics the std::string
         * object. Ids are often used in debug messages, where they are forwarded as C-strings
         * to be included in the format string, e.g. debugmsg("invalid id: %s", id.c_str())
         */
        const char *c_str() const {
            return _id.c_str();
        }
        /**
         * Returns the identifier as plain std::string. Use with care, the plain string does not
         * have any information as what type of object it refers to (the T template parameter of
         * the class).
         */
        const std::string &str() const {
            return _id;
        }

        // Those are optional, you need to implement them on your own if you want to use them.
        // If you don't implement them, but use them, you'll get a linker error.
        /**
         * Translate the string based it to the matching integer based id.
         * This may issue a debug message if the string is not a valid id.
         */
        int_id<T> id() const;
        /**
         * Returns the actual object this id refers to. May show a debug message if the id is invalid.
         */
        const T &obj() const;
        /**
         * Returns whether this id is valid, that means whether it refers to an existing object.
         */
        bool is_valid() const;
        /**
         * The null-id itself. `NULL_ID.is_null()` must always return true. See @ref is_null.
         */
        static const string_id<T> NULL_ID;
        /**
         * Returns whether this represents the id of the null-object (in which case it's the null-id).
         * Note that not all types @ref T may have a null-object. As such, there won't be a
         * definition of @ref NULL_ID and if you use any of the related functions, you'll get
         * errors during the linking.
         *
         * Example: "mon_null" is the id of the null-object of monster type.
         *
         * Note: per definition the null-id shall be valid. This allows to use it in places
         * that require a (valid) id, but it can still represent a "don't use it" value.
         */
        bool is_null() const {
            return operator==( NULL_ID );
        }
        /**
         * Same as `!is_null`, basically one can use it to check for the id referring to an actual
         * object. This avoids explicitly comparing it with NULL_ID. The id may still be invalid,
         * but that should have been checked when the world data was loaded.
         * \code
         * string_id<X> id = ...;
         * if( id ) {
         *     apply_id( id );
         * } else {
         *     // was the null-id, ignore it.
         * }
         * \endcode
         */
        explicit operator bool() const {
            return !is_null();
        }
    private:
        std::string _id;
};

// Support hashing of string based ids by forwarding the hash of the string.
namespace std
{
template<typename T>
struct hash< string_id<T> > {
    std::size_t operator()( const string_id<T> &v ) const {
        return hash<std::string>()( v.str() );
    }
};
}

/**
 * Instances of this type are *implicitly* convertible to string_id<T> (with any kind of T).
 * There is also the global constant @ref NULL_ID, which should be the only instance of this
 * struct you'll ever need.
 * Together they allow this neat code:
 * \code
 * string_id<Foo> foo_id( NULL_ID );
 * string_id<Bar> bar_id( NULL_ID );
 *
 * string_id<X> x_id = NULL_ID;
 * string_id<Y> get_id() { return NULL_ID; }
 * \endcode
 *
 * The neat thing is that NULL_ID works for *all* types of string_id, without explicitly stating
 * what the template parameter should be. The compiler should figure it out on its own.
 *
 * However, note that you can not call string_id functions on a NULL_ID object. The object doesn't
 * known which actual string_id it refers to. In that case, use the @ref string_id<T>::NULL_ID
 * directly.
 */
struct null_id_type {
    template<typename T>
    operator const string_id<T> &() const {
        return string_id<T>::NULL_ID;
    }
};

namespace
{
const null_id_type NULL_ID{};
}

#endif
