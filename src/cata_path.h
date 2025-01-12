#pragma once
#ifndef CATA_SRC_CATA_PATH_H
#define CATA_SRC_CATA_PATH_H

#include <iosfwd>

#include <filesystem>

/**
 * One of the problems of filesystem paths is they lack contextual awareness
 * of 'where' they are. They're just a list of segments pointing into the
 * filesystem. There are cases where it's useful to have a path relative to
 * some logical root which isn't the actual filesystem root, and work with /
 * manipulate that path independently of the logical root.
 *
 * cata_path combines a logical root concept and an std::filesystem::path. The logical
 * root is an enum value containing well known cataclysm 'roots'. It only
 * exposes operator / for concatenating segments to the relative path.
 * The constructor requires passing a logical root value which can be queried,
 * and when (explicitly) converting to std::filesystem::path will prepend the logical root
 * with the relative path to give a final absolute path. Callers can know this
 * absolute path is relative to the logical root without having to perform any
 * further tests.
 *
 * Attempting to construct a path that 'escapes' the logical root will result
 * in a path rooted at the logical root, just like what happens if you try to
 * resolve '/../'.
 *
 * All string values are assumed to be utf-8 encoded, but std::filesystem::path values are
 * used as-is without checking for encoding, so std::filesystem::u8path should be used if the
 * path is utf-8 encoded.
 */
class cata_path
{
    public:
        // All of these represent configurable filesystem locations for various pieces
        // of data. The default locations may all be subfolders of `base`, but they can
        // be overridden or default to other filesystem locations.
        enum class root_path {
            base,
            config,
            data,
            memorial,
            save,
            user,
            unknown, // assumed relative to cwd, path is "."
            // fun fact: std::filesystem::path{} / "foo" == "foo", no "/foo".
        };

        cata_path() : logical_root_{ root_path::unknown } {}

        template<typename T>
        cata_path( root_path logical_root, T &&relative_path ) : logical_root_{ logical_root },
            relative_path_{ as_fs_path( std::forward<T>( relative_path ) ) } { }

        cata_path( const cata_path & ) = default;
        cata_path &operator=( const cata_path & ) = default;

        cata_path( cata_path && ) = default;
        cata_path &operator=( cata_path && ) = default;

        // Returns an std::filesystem::path constructed by concatenating the 'actual' path
        // value for the logical root with the relative path stored in this.
        // path with root_path::unknown are returned as-is.
        inline std::filesystem::path get_unrelative_path() const {
            std::filesystem::path result = get_logical_root_path();
            result /= relative_path_;
            return result;
        }

        explicit inline operator std::filesystem::path() const {
            return get_unrelative_path();
        }

        inline root_path get_logical_root() const {
            return logical_root_;
        }

        // Returns the actual path to the logical root this cata_path is relative to.
        std::filesystem::path get_logical_root_path() const;

        // Returns a direct reference to the relative path component. Useful for
        // sanity checking the path eg. does not being with '..'.
        inline const std::filesystem::path &get_relative_path() const {
            return relative_path_;
        }

        // Below are named-alike functions from std::filesystem::path which do similar things
        // to this object.

        template<typename T>
        cata_path operator/( T &&segment ) const {
            cata_path ret = *this;
            ret /= as_fs_path( std::forward<T>( segment ) );
            return ret;
        }

        template<typename T>
        cata_path &operator/=( T &&segment ) {
            relative_path_ /= as_fs_path( std::forward<T>( segment ) );
            return *this;
        }

        template<typename T>
        cata_path operator+( T &&segment ) const {
            cata_path ret = *this;
            ret += as_fs_path( std::forward<T>( segment ) );
            return ret;
        }

        template<typename T>
        cata_path &operator+=( T &&segment ) {
            relative_path_ += as_fs_path( std::forward<T>( segment ) );
            return *this;
        }

        inline cata_path lexically_normal() const {
            cata_path ret{ logical_root_, std::filesystem::path{} };
            ret.relative_path_ = relative_path_.lexically_normal();
            return ret;
        }

        inline bool empty() const {
            return logical_root_ == root_path::unknown && relative_path_.empty();
        }

        inline bool operator==( const cata_path &rhs ) const {
            return logical_root_ == rhs.logical_root_ && relative_path_ == rhs.relative_path_;
        }

        template< class CharT, class Traits >
        friend std::basic_ostream<CharT, Traits> &
        operator<<( std::basic_ostream<CharT, Traits> &os, const cata_path &p ) {
            os << p.get_unrelative_path();
            return os;
        }

        inline cata_path parent_path() const {
            return cata_path{ logical_root_, relative_path_.parent_path() };
        }

        inline std::string generic_u8string() const {
            return get_unrelative_path().generic_u8string();
        }

    private:
        root_path logical_root_;
        std::filesystem::path relative_path_;

        template<typename T>
        static std::enable_if_t < std::is_same_v<std::decay_t<T>, std::filesystem::path>, T && >
        as_fs_path( T &&path ) {
            return std::forward<T>( path );
        }

        template<typename T>
        static std::enable_if_t < !std::is_same_v<std::decay_t<T>, std::filesystem::path>,
               std::filesystem::path >
        as_fs_path( T &&path ) {
            return std::filesystem::u8path( std::forward<T>( path ) );
        }
};

#endif // CATA_SRC_CATA_PATH_H
