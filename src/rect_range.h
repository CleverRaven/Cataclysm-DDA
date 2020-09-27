#pragma once
#ifndef CATA_SRC_RECT_RANGE_H
#define CATA_SRC_RECT_RANGE_H

#include "point.h"

// This is a template parameter, it's usually SDL_Rect, but that way the class
// can be used without include any SDL header.
template<typename RectType>
class rect_range
{
    private:
        int width;
        int height;
        point count;

    public:
        rect_range( const int w, const int h, const point &c ) : width( w ), height( h ),
            count( c ) {
        }

        class iterator
        {
            private:
                friend class rect_range;
                const rect_range *range;
                int index;

                iterator( const rect_range *const r, const int i ) : range( r ), index( i ) {
                }

            public:
                bool operator==( const iterator &rhs ) const {
                    return range == rhs.range && index == rhs.index;
                }
                bool operator!=( const iterator &rhs ) const {
                    return !operator==( rhs );
                }
                RectType operator*() const {
                    return { ( index % range->count.x ) *range->width, ( index / range->count.x ) *range->height, range->width, range->height };
                }

                iterator operator+( const int offset ) const {
                    return iterator( range, index + offset );
                }
                int operator-( const iterator &rhs ) const {
                    return index - rhs.index;
                }
                iterator &operator++() {
                    ++index;
                    return *this;
                }
        };

        iterator begin() const {
            return iterator( this, 0 );
        }
        iterator end() const {
            return iterator( this, count.x * count.y );
        }
};

#endif // CATA_SRC_RECT_RANGE_H
