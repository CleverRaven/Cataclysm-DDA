#pragma once
#ifndef CATA_SRC_RECT_RANGE_H
#define CATA_SRC_RECT_RANGE_H

// This is a template parameter, it's usually SDL_Rect, but that way the class
// can be used without include any SDL header.
template<typename RectType>
class rect_range
{
    private:
        int width;
        int height;
        int xcount;
        int ycount;

    public:
        rect_range( const int w, const int h, const int xc, const int yc ) : width( w ), height( h ),
            xcount( xc ), ycount( yc ) {
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
                    return { ( index % range->xcount ) *range->width, ( index / range->xcount ) *range->height, range->width, range->height };
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
            return iterator( this, xcount * ycount );
        }
};

#endif // CATA_SRC_RECT_RANGE_H
