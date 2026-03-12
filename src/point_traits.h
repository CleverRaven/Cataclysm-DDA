#pragma once
#ifndef CATA_SRC_POINT_TRAITS_H
#define CATA_SRC_POINT_TRAITS_H

#include <type_traits>

struct point;
struct tripoint;

template<typename Point, typename = void>
struct point_traits {
    static int &x( Point &p ) {
        return p.x();
    }
    static int x( const Point &p ) {
        return p.x();
    }
    static int &y( Point &p ) {
        return p.y();
    }
    static int y( const Point &p ) {
        return p.y();
    }
    static int &z( Point &p ) {
        return p.z();
    }
    static int z( const Point &p ) {
        return p.z();
    }
};

template<typename Point>
struct point_traits <
    Point,
    std::enable_if_t < std::is_same_v<Point, point> || std::is_same_v<Point, tripoint> >
    >  {
    static int &x( Point &p ) {
        return p.x;
    }
    static const int &x( const Point &p ) {
        return p.x;
    }
    static int &y( Point &p ) {
        return p.y;
    }
    static const int &y( const Point &p ) {
        return p.y;
    }
    static int &z( Point &p ) {
        return p.z;
    }
    static const int &z( const Point &p ) {
        return p.z;
    }
};

#endif // CATA_SRC_POINT_TRAITS_H
