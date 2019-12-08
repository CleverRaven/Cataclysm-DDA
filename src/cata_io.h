#pragma once
#ifndef CATA_IO_H
#define CATA_IO_H

#include <functional>
#include <string>
#include <type_traits>

#include "json.h"

/**
 * @name Serialization and deserialization
 *
 * Basic classes are the input and output archives. Each of them comes in two flavors: one stores
 * data in a JSON array and the other stores them in a JSON object.
 *
 * The input archive is used to deserialize (ie.e read from the archive, write to a C++ variable),
 * the output archive does the inverse.
 *
 * Input and output archives (that refer to the same JSON type, i.e. @ref JsonArrayInputArchive
 * and @ref JsonArrayOutputArchive) have (nearly) the same interface (mostly the `io` function),
 * they only differ in what the functions do. This is an important design decision, it allows
 * the archive type to be a template parameter. The `io` functions in both archive classes should
 * therefor have a compatible signature.
 *
 * Archive classes also have a `is_input` member type, which is either @ref std::true_type or
 * if(the class is an input archive) or @ref std::false_type (it's an output archive). This can be
 * used to do additional things only after loading the data, but not after storing it.
 *
 * Usage: implement a tempted function `io` in the class that is to be serialized and add a typedef
 *
 *     class MySerializeableType {
 *         ...
 *         template<typename Archive>
 *         void item::io( Archive& archive ) {
 *             archive.io( "member1", member1, default_value_of_member1 );
 *             archive.io( "member2", member2, default_value_of_member2 );
 *             ...
 *         }
 *         using archive_type_tag = io::object_archive_tag;
 *         ...
 *     };
 *
 * The tag types are defined in io_tags.h so that this header need not be
 * included to use them.
 *
 * The function must be declared with the archive type as template parameter, but it can be
 * implemented outside of the header (e.g. in savegame_json.cpp). It will be instantiated with
 * JsonArrayInputArchive and JsonArrayOutputArchive, or with JsonObjectInputArchive and
 * JsonObjectOutputArchive as archive type, depending on the type of the archive_type_tag.
 *
 * Note that the array based archives have a much simpler interface, they only support
 * reading/writing sequentially.
 *
 * The `io` function works for input and output archives. One can call it like this:
 *
 *     item& tmp = ...
 *     JsonOutputArchive out( ... );
 *     tmp.io( out );
 *     JsonInputArchive in( ... );
 *     tmp.io( in );
 *
 * The archive classes use the Json classes (see json.h) to write and read data. This works fine
 * for most built-in types (including string and certain container types). Specific classes, like
 * @ref item or @ref monster, need their own logic. They used to inherit from @ref JsonDeserializer
 * and @ref JsonSerializer, which would grant them functions to serialize/deserialize. The Json
 * classes will automatically use those if available, but they prefer the `io` function.
 *
 * Classes that only use the (templated) `io` function (and which do not inherit from
 * JsonDeserializer and JsonSerializer), must announce that. This allows the Archive classes to
 * avoid calling Json functions, which would not work.
 *
 * If a class only uses the `io` function defined like this:
 *
 *     template<typename Archive> void io( Archive& );
 *
 * It should also have a type tag that indicates 1) the`io` function can be used and 2) whether
 * the data of the class is stored in a JSON array (@ref io::array_archive_tag) or in a
 * JSON object (@ref io::object_archive_tag).
 *
 *     using archive_type_tag = io::object_archive_tag;
 *
 * Note that the *archive_tag structs only need to be forward declared to work in the using
 * statement. One does not need to include "io.h" to allow that statement.
 */
namespace io
{

/**
 * Tag that indicates the default value for io should be the default value of the type
 * itself, i.e. the result of default initialization: `T()`
 * The type must support a comparison, so serializing can check whether the value is equal to the
 * default value.
 *
 * An instance of this tag can be used as default value parameter in the io functions.
 */
struct default_tag { };
/**
 * Tag that indicates the value of the io is some kind of container and its default value is to
 * be empty. This does not require it to have a comparison function (e.g. suitable for @ref item).
 * It requires the type to have `bool empty()` and `void clear()` functions (i.e. works for maps).
 *
 * An instance of this tag can be used as default value parameter in the io functions.
 */
struct empty_default_tag { };

/**
 * Tag that indicates the value is required and must exists in the JSON data.
 */
struct required_tag { };

/**
 * The namespace contains classes that do write/read to/from JSON via either the Json classes in
 * "json.h" or via the `io` function of the object to be written/read.
 * Which of those classes is actually used determined via the template parameter and use of SFINAE.
 * Usage:
 *
 *     io::detail::has_archive_tag<SomeType>::write( stream, instance_of_SomeType );
 */
namespace detail
{

template<class T, class R = void>
struct enable_if_type {
    using type = R;
};

/**
 * Implementation for classes that don't have an archive_type_tag defined. They use the
 * normal JsonSerializer / JsonDeserializer interface, which is handled directly by the Json
 * classes. Therefor the functions here simply forward to those.
 */
template<class T, class E = void>
struct has_archive_tag : std::false_type {
    static void write( JsonOut &stream, const T &value ) {
        stream.write( value );
    }
    static bool read( JsonObject &obj, const std::string &key, T &value ) {
        return obj.read( key, value );
    }
    static bool read( JsonArray &arr, T &value ) {
        return arr.read_next( value );
    }
};

/**
 * Implementation for classes that use the `io` function. The function herein create Archive
 * instances and call the `io` function of the object to be written/read.
 */
template<class T>
struct has_archive_tag<T, typename enable_if_type<typename T::archive_type_tag>::type> :
    std::true_type {
    using InArchive = typename T::archive_type_tag::InputArchive;

    static void write( JsonOut &stream, const T &value ) {
        typename T::archive_type_tag::OutputArchive archive( stream );
        const_cast<T &>( value ).io( archive );
    }
    static bool read( JsonObject &obj, const std::string &key, T &value ) {
        if( !obj.has_member( key ) ) {
            return false;
        }
        InArchive archive( obj, key );
        value.io( archive );
        return true;
    }
    static bool read( JsonArray &arr, T &value ) {
        if( !arr.has_more() ) {
            return false;
        }
        InArchive archive( arr );
        value.io( archive );
        return true;
    }
};

} // namespace detail

/**
 * Input archive reading data from a Json object.
 *
 * Data can be loaded from this archive using the name of the member as key.
 */
class JsonObjectInputArchive : public JsonObject
{
    public:
        using is_input = std::true_type;

        JsonObjectInputArchive( const JsonObject &jo )
            : JsonObject( jo ) {
        }
        /** Create archive from next object in the given Json array. */
        JsonObjectInputArchive( JsonArray & );
        /** Create archive from named member object in the given Json object. */
        JsonObjectInputArchive( JsonObject &, const std::string &key );

        /**
         * @name Deserialization
         *
         * The io functions read a value from the archive and store it in the reference parameter.
         *
         * @throw JsonError (via the Json classes) if the value in the archive is of an incompatible
         * type (e.g. reading a string, but the member is a Json object).
         * @return `false` if the archive did not contain the requested member, otherwise `true`.
         */
        /*@{*/
        /**
         * If the archive does not have the requested member, the value is not changed at all.
         */
        template<typename T>
        bool io( const std::string &name, T &value ) {
            return io::detail::has_archive_tag<T>::read( *this, name, value );
        }
        /**
         * If the archive does not have the requested member, the given default value is assigned.
         * The function still returns false if the default value had been used.
         */
        template<typename T>
        bool io( const std::string &name, T &value, const T &default_value ) {
            if( io( name, value ) ) {
                return true;
            }
            value = default_value;
            return false;
        }
        /**
         * Like the other io function, this uses a default constructed default value:
         * Roughly equivalent to \code io<T>( name, value, T() ); \endcode
         */
        template<typename T>
        bool io( const std::string &name, T &value, default_tag ) {
            static const T default_value = T();
            return io( name, value, default_value );
        }
        /**
         * If the archive does not have the requested member, the function `clear` is called on the
         * value, this may be used for containers (e.g. std::map).
         */
        template<typename T>
        bool io( const std::string &name, T &value, empty_default_tag ) {
            if( io( name, value ) ) {
                return true;
            }
            value.clear();
            return false;
        }
        /**
         * Special function to load pointers. If the archive does not have the requested member,
         * `nullptr` is assigned to the pointer. Otherwise the value is loaded (which must be a
         * `std::string`) and the load function is called.
         *
         * Example:
         *
         *     class Dummy {
         *         int *pointer;
         *         template<typename A> void io( A& ar ) {
         *             ar.io( "ptr", pointer,
         *                 [this](const std::string&s) {
         *                     pointer = new int( atoi( s.c_str() ) );
         *                 },
         *                 [](const int &v) {
         *                     return std::to_string( v );
         *                 }
         *             );
         *         }
         *     };
         *
         * @param name Name of requested member
         * @param pointer
         * @param load The function gets the string that was loaded from JSON and should set the pointer
         * in the object accordingly. One would usually use a lambda for this function.
         * @param save The inverse of the load function, it converts the pointer to a string which is
         * later stored in the JSON.
         * @param required If `true`, an error will be raised if the requested member does not exist
         * in the JSON data.
         * JsonOutputArchive, so it can be used when the archive type is a template parameter.
         */
        template<typename T>
        bool io( const std::string &name, T *&pointer,
                 const std::function<void( const std::string & )> &load,
                 const std::function<std::string( const T & )> &save, bool required = false ) {
            ( void ) save; // Only used by the matching function in the output archive classes.
            std::string ident;
            if( !io( name, ident ) ) {
                if( required ) {
                    JsonObject::throw_error( std::string( "required member is missing: " ) + name );
                }
                pointer = nullptr;
                return false;
            }
            load( ident );
            return true;
        }
        template<typename T>
        bool io( const std::string &name, T *&pointer,
                 const std::function<void( const std::string & )> &load,
                 const std::function<std::string( const T & )> &save, required_tag ) {
            return io<T>( name, pointer, load, save, true );
        }
        /*@}*/
};

/**
 * Input archive reading data from a Json array.
 *
 * This archive is quite simple, it loads the data sequentially from the Json array. The first
 * call to @ref io loads the first value, the next call loads the second and so on.
 *
 * Reading a value from a specific index is on purpose not supported.
 */
class JsonArrayInputArchive : public JsonArray
{
    public:
        using is_input = std::true_type;

        JsonArrayInputArchive( const JsonArray &jo )
            : JsonArray( jo ) {
        }
        /** Create archive from next object in the given Json array. */
        JsonArrayInputArchive( JsonArray & );
        /** Create archive from named member object in the given Json object. */
        JsonArrayInputArchive( JsonObject &, const std::string &key );

        template<typename T>
        bool io( T &value ) {
            return io::detail::has_archive_tag<T>::read( *this, value );
        }
};

inline JsonArrayInputArchive::JsonArrayInputArchive( JsonArray &arr )
    : JsonArray( arr.next_array() ) { }
inline JsonArrayInputArchive::JsonArrayInputArchive( JsonObject &obj, const std::string &key )
    : JsonArray( obj.get_array( key ) ) { }

inline JsonObjectInputArchive::JsonObjectInputArchive( JsonArray &arr )
    : JsonObject( arr.next_object() ) { }
inline JsonObjectInputArchive::JsonObjectInputArchive( JsonObject &obj, const std::string &key )
    : JsonObject( obj.get_object( key ) ) { }

/**
 * Output archive matching the input archive @ref JsonObjectInputArchive.
 *
 * It has the same function as the input archive, they do the exact inverse.
 *
 * The class creates a valid JSON value in the output stream (that means it adds the initial
 * '{' and '}' to the stream).
 */
class JsonObjectOutputArchive
{
    public:
        using is_input = std::false_type;

        JsonOut &stream;

        JsonObjectOutputArchive( JsonOut &stream )
            : stream( stream ) {
            stream.start_object();
        }
        ~JsonObjectOutputArchive() {
            stream.end_object();
        }

        /**
         * @name Serialization
         *
         * The io functions store a value (given as parameter) in the archive.
         *
         * @throw JsonError (via the Json classes) on any kind of error.
         *
         * @return All functions return `false`. Their signature should be compatible with the
         * functions in @ref JsonObjectInputArchive, so they can be used when the archive type is a
         * template parameter.
         */
        /*@{*/
        template<typename T>
        bool io( const std::string &name, const T &value ) {
            stream.member( name );
            io::detail::has_archive_tag<T>::write( stream, value );
            return false;
        }
        template<typename T>
        bool io( const std::string &name, const T &value, const T &default_value ) {
            if( value == default_value ) {
                return false;
            }
            return io( name, value );
        }
        template<typename T>
        bool io( const std::string &name, const T &value, default_tag ) {
            static const T default_value = T();
            return io<T>( name, value, default_value );
        }
        template<typename T>
        bool io( const std::string &name, const T &value, empty_default_tag ) {
            if( !value.empty() ) {
                io<T>( name, value );
            }
            return false;
        }
        /**
         * Special function to store pointers.
         *
         * If the pointer is `nullptr`, it will not be stored at all (`nullptr` is the default value).
         * Otherwise the save function is called to translate the pointer into a string (which can be
         * the id of the pointed to object or similar). The returned string is stored in the archive.
         *
         * The function signature is compatible with the similar function in the
         * @ref JsonObjectInputArchive, so it can be used when the archive type is a template parameter.
         */
        template<typename T>
        bool io( const std::string &name, const T *pointer,
                 const std::function<void( const std::string & )> &,
                 const std::function<std::string( const T & )> &save, bool required = false ) {
            if( pointer == nullptr ) {
                if( required ) {
                    throw JsonError( "a required member is null: " + name );
                }
                return false;
            }
            return io( name, save( *pointer ) );
        }
        template<typename T>
        bool io( const std::string &name, const T *pointer,
                 const std::function<void( const std::string & )> &load,
                 const std::function<std::string( const T & )> &save, required_tag ) {
            return io<T>( name, pointer, load, save, true );
        }
        /*@}*/
        /**
         * For compatibility with the input archive. Output archives obliviously never read anything
         * and always return false.
         */
        template<typename T>
        bool read( const std::string &, T & ) {
            return false;
        }
};

/**
 * Output archive matching the input archive @ref JsonArrayInputArchive.
 *
 * The class creates a valid JSON value in the output stream (that means it adds the initial
 * '[' and ']' to the stream).
 */
class JsonArrayOutputArchive
{
    public:
        using is_input = std::false_type;

        JsonOut &stream;

        JsonArrayOutputArchive( JsonOut &stream )
            : stream( stream ) {
            stream.start_array();
        }
        ~JsonArrayOutputArchive() {
            stream.end_array();
        }

        template<typename T>
        bool io( const T &value ) {
            io::detail::has_archive_tag<T>::write( stream, value );
            return false;
        }
};

} // namespace io

#endif
