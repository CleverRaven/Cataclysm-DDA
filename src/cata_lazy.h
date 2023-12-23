#pragma once
#ifndef CATA_SRC_CATA_LAZY_H
#define CATA_SRC_CATA_LAZY_H

#include <optional>
#include <type_traits>

/**
 * A wrapper for a type that lazily initializes the underlying type as needed. Use to
 * save unnecessary allocation costs for eg. std containers in transiently allocated types.
 *
 * T must be default initializable.
 */
template<typename T, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
struct lazy {
    lazy() = default;

    // std::optional-like api for testing without allocating

    bool has_value() const {
        return actual.has_value();
    }

    // Transparent value operators that proxy to the actual value.

    template<typename Arg, typename ...Args>
    // NOLINTNEXTLINE(google-explicit-constructor)
    lazy( Arg &&arg, Args &&...args ) : actual{std::in_place, std::forward<Arg>( arg ), std::forward<Args>( args )...} {}

    template<typename U>
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    T &operator=( U &&u ) {
        get() = std::forward<U>( u );
        return get();
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T &() {
        return get();
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T const &() const {
        return get();
    }

    T &operator*() {
        return get();
    }

    const T &operator*() const {
        return get();
    }

    T *operator->() {
        return &get();
    }

    const T *operator->() const {
        return &get();
    }

    template<typename Arg>
    auto &&operator[]( Arg &&arg ) {
        return get()[std::forward<Arg>( arg )];
    }

private:
    // The constness is a lie because actual is mutable, but that's why it's private.
    T &get() const {
        if( !actual.has_value() ) {
            actual.emplace();
        }
        return actual.value();
    }

    mutable std::optional<T> actual;
};

#endif // CATA_SRC_CATA_LAZY_H
