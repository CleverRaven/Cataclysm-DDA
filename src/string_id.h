#pragma once
#ifndef CATA_SRC_STRING_ID_H
#define CATA_SRC_STRING_ID_H

#include <string>
#include <type_traits>

static constexpr int64_t INVALID_VERSION = -1;
static constexpr int INVALID_CID = -1;

template<typename T>
class int_id;

template<typename T>
class generic_factory;

namespace Hash {
    namespace CompileTime {
        template <unsigned N>
        constexpr uint64_t fnv1_64(const char(&text)[N])
        {
            return (fnv1_64<N - 1>((const char(&)[N - 1])text) * 1099511628211ull) ^ text[N - 2];
        }

        template<>
        constexpr uint64_t fnv1_64<1>(const char(&text)[1])
        {
            return 14695981039346656037ull;
        }
    }

    namespace RunTime {
        constexpr uint64_t fnv1_64(const char* data, size_t length)
        {
            uint64_t hash = 14695981039346656037ull;
            for (int i = 0; i < length; i++) {
                hash *= 1099511628211ull;
                hash ^= data[i];
            }
            return hash;
        }
    }
}

constexpr uint64_t operator"" _id(const char *data, size_t length)
{
    // This allows us to compute the hash of all compiled IDs in
    // (hopefully) compile time. constexpr doesn't promise it...
    return Hash::RunTime::fnv1_64(data, length);
}

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
 * Note that a forward declaration is sufficient for the template parameter type.
 *
 * If an id is used locally in just one header & source file, then feel free to
 * define it in those files.  If it is used more widely (like mtype_id), then
 * please define it in type_id.h, a central light-weight header that defines all ids
 * people might want to use.  This prevents duplicate definitions in many
 * files.
 *
 * Notes on usage and performance (comparison between ids, ::obj lookup,
 *    assuming default implementation of generic_factory is used):
 *
 * `int_id` is fastest, but it can't be reused after game reload. Depending on the loaded
 *    combination of mods `int_id` might point to a different entity and there is no way to tell.
 *    Because of this NEVER define static int ids.
 *    But, it's safe to cache and use int_id within the game session.
 *
 * `string_id` is a bit slower than int_id, but it's safe to use the same instance between game reloads.
 *    That means that string_id can be static (and should be for maximal performance).
 *    Comparison of string ids is relatively slow (same as std::string comparison).
 *    for newly created string_id (i.e. inline constant or local variable), first method invocation:
 *     `::id` call is relatively slow (string hash map lookup)
 *     `::obj` lookup is slow (string hash map lookup + array read)
 *      note, after the first invocation, all subsequent calls on the same string_id instance are fast
 *    for old (or static) string_id, second and subsequent method invocations:
 *      conversion to int_id is extremely fast (just returns int field)
 *     `::obj` call is relatively fast (array read by int index), a bit slower than on int_id
 */
template<typename T>
class string_id
{
public:
    template<class U>
    friend class string_id;
    using This = string_id<T>;

    template<unsigned N>
    explicit constexpr string_id(const char (&str)[N]) : _hash(Hash::CompileTime::fnv1_64(str)), _is_char_ptr(true), _char_ptr(str) {}

    template<typename U>
    string_id(const string_id<U>& other) : _hash(other.hash()), _char_ptr(other._char_ptr), _is_char_ptr(other._is_char_ptr) {}

    template<typename S, class = typename
        std::enable_if_t< std::is_convertible_v< S, std::string > >
    >
    explicit string_id(S&& id) {
        auto str = std::make_unique<std::string>(std::forward<S>(id));
        uint64_t hash = Hash::RunTime::fnv1_64(str->c_str(), str->size());
        _hash = hash;

        auto insertion = cached_strings().try_emplace(hash, std::move(str));
        auto cached_string = insertion.first->second.get();

        // Verify we don't have hash collisions
        //assert(insertion.second || *cached_string == id);

        _string = cached_string;
    }

     /**
      * Default constructor constructs an empty id string.
      * Note that this id class does not enforce empty id strings (or any specific string at all)
      * to be special. Every string (including the empty one) may be a valid id.
      */
    constexpr string_id() : string_id("") {};
    /**
     * Comparison, only useful when the id is used in std::map or std::set as key. Compares
     * the string id as with the strings comparison.
     */
    bool operator<(const This& rhs) const {
        // Not str() compares to prevent needless tier ups to std::string
        return strcmp(c_str(), rhs.c_str()) < 0;
    }
    /**
     * The usual comparator, compares the string id as usual.
     */
    bool operator==(const This& rhs) const {
        return _hash == rhs._hash;
    }
    /**
     * Allows to quickly compare hashes, when caching the hash would be faster
     */
    bool operator==(uint64_t hash) const {
        return _hash == hash;
    }
    /**
     * The usual comparator, compares the string id as usual.
     */
    bool operator!=(const This& rhs) const {
        return !operator==(rhs);
    }
    /**
     * Interface to the plain C-string of the id. This function mimics the std::string
     * object. Ids are often used in debug messages, where they are forwarded as C-strings
     * to be included in the format string, e.g. debugmsg("invalid id: %s", id.c_str())
     */
    const char* c_str() const {
        return _is_char_ptr ? _char_ptr : _string->c_str();
    }
    /**
     * Returns the identifier as plain std::string. Use with care, the plain string does not
     * have any information as what type of object it refers to (the T template parameter of
     * the class).
     * Note: The function is declared with safebuffers as MSVC insert a cookie check here
     * for some reason, and it takes up 25% of str()'s runtime.
     */
    __declspec(safebuffers)
    const std::string& str() const {
        if (_is_char_ptr) {
            // We don't want to break the constness of str(), and this change is 100% internal with
            // observable side effects, therefore we break the constness of this to cache the string.
            auto wthis = const_cast<This*>(this);
            auto string = std::make_unique<std::string>(_char_ptr);
            wthis->_string = cached_strings().try_emplace(_hash, std::move(string)).first->second.get();
            wthis->_is_char_ptr = false;
        }
        return *_string;
    }

    explicit operator std::string() const {
        return str();
    }

    // Those are optional, you need to implement them on your own if you want to use them.
    // If you don't implement them, but use them, you'll get a linker error.
    /**
     * Translate the string based it to the matching integer based id.
     * This may issue a debug message if the string is not a valid id.
     */
    int_id<T> id() const;
    /**
     * Translate the string based it to the matching integer based id.
     * If this string_id is not valid, returns `fallback`.
     * Does not produce debug message.
     */
    int_id<T> id_or(const int_id<T>& fallback) const;
    /**
     * Returns the actual object this id refers to. May show a debug message if the id is invalid.
     */
    const T& obj() const;

    const T& operator*() const {
        return obj();

    }
    const T* operator->() const {
        return &obj();
    }

    /**
     * Returns whether this id is valid, that means whether it refers to an existing object.
     */
    bool is_valid() const;
    /**
     * Returns whether this id is empty. An empty id can still be valid,
     * and emptiness does not mean that it's null. Named is_empty() to
     * keep consistency with the rest is_.. functions
     */
    bool is_empty() const {
        return _hash == ""_id;
    }
    /**
     * Returns a null id whose `string_id<T>::is_null()` must always return true. See @ref is_null.
     * Specializations are defined in string_id_null_ids.cpp to avoid instantiation ordering issues.
     */
    static const string_id<T>& NULL_ID();
    /**
     * Returns whether this represents the id of the null-object (in which case it's the null-id).
     * Note that not all types assigned to T may have a null-object. As such, there won't be a
     * definition of @ref NULL_ID and if you use any of the related functions, you'll get
     * errors during the linking.
     *
     * Example: "mon_null" is the id of the null-object of monster type.
     *
     * Note: per definition the null-id shall be valid. This allows to use it in places
     * that require a (valid) id, but it can still represent a "don't use it" value.
     */
    bool is_null() const {
        return operator==(NULL_ID());
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

    uint64_t hash() const {
        return _hash;
    }

private:
    std::map < uint64_t, std::unique_ptr< std::string > >& cached_strings() const {
        static std::map < uint64_t, std::unique_ptr< std::string > > cached_strings_{};
        return cached_strings_;
    }

    uint64_t _hash = ""_id;

    union {
        std::string* _string;
        const char* _char_ptr;
    };

    // cached int_id counterpart of this string_id
    mutable int _cid = INVALID_CID;
    // generic_factory version that corresponds to the _cid
    mutable int _version = INVALID_VERSION;
    // To save these extra 8 bytes, move the bool into the MSb of _string
    bool _is_char_ptr = false;

    inline void set_cid_version(int cid, int version) const {
        _cid = cid;
        _version = version;
    }

    friend class generic_factory<T>;
};

// Support hashing of string based ids by forwarding the hash of the string.
namespace std
{
    template<typename T>
    struct hash< string_id<T> > {
        std::size_t operator()(const string_id<T>& v) const noexcept {
            return static_cast<size_t>(v.hash());
        }
    };
} // namespace std

#endif // CATA_SRC_STRING_ID_H
