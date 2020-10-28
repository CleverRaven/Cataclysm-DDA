#pragma once
#ifndef CATA_SRC_ENUM_INT_OPERATORS_H
#define CATA_SRC_ENUM_INT_OPERATORS_H

#define DEFINE_INTEGER_OPERATORS(Type) \
    \
    constexpr inline bool operator>=( const Type &lhs,\
                                      const Type &rhs )\
    {\
        return static_cast<int>( lhs ) >= static_cast<int>( rhs );\
    }\
    \
    constexpr inline bool operator<( const Type &lhs,\
                                     const Type &rhs )\
    {\
        return static_cast<int>( lhs ) < static_cast<int>( rhs );\
    }\
    template<typename T>\
    constexpr inline bool operator>=( const T &lhs, const Type &rhs )\
    {\
        return lhs >= static_cast<T>( rhs );\
    }\
    \
    template<typename T>\
    constexpr inline bool operator>( const T &lhs, const Type &rhs )\
    {\
        return lhs > static_cast<T>( rhs );\
    }\
    \
    template<typename T>\
    constexpr inline bool operator<=( const T &lhs, const Type &rhs )\
    {\
        return lhs <= static_cast<T>( rhs );\
    }\
    \
    template<typename T>\
    constexpr inline bool operator<( const T &lhs, const Type &rhs )\
    {\
        return lhs < static_cast<T>( rhs );\
    }\
    \
    template<typename T>\
    constexpr inline int operator/( const Type &lhs, const T &rhs )\
    {\
        return static_cast<T>( lhs ) / rhs;\
    }\
    \
    template<typename T>\
    constexpr inline int operator+( const Type &lhs, const T &rhs )\
    {\
        return static_cast<T>( lhs ) + rhs;\
    }\
    \
    template<typename T>\
    constexpr inline int operator-( const Type &lhs, const T &rhs )\
    {\
        return static_cast<T>( lhs ) - rhs;\
    }\
    \
    template<typename T>\
    constexpr inline int operator-( const T &lhs, const Type &rhs )\
    {\
        return lhs - static_cast<T>( rhs );\
    }\
    \
    /* For easy casting: +t is static_cast<int>(t) */\
    constexpr int operator+(Type t) noexcept\
    {\
        return static_cast<int>(t);\
    }

#endif
