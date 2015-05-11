#ifndef MAP_ITERATOR_H
#define MAP_ITERATOR_H

#include "enums.h" 

class tripoint_range {
private:
    /**
     * Generates points in a rectangle.
     */
    class point_generator {
    friend class tripoint_range;
    private:
        tripoint p;
        const tripoint_range &range;
    public:
        point_generator( const tripoint &_p, const tripoint_range &_range )
            : p( _p ), range( _range )
        { }

        // Incement x, then if it goes outside range, "wrap around" and increment y
        // Same for y and z
        point_generator &operator++();
        const tripoint &operator*() const;
        bool operator!=( const point_generator &other ) const;
    };

    int minx;
    int miny;
    int minz;
    int maxx;
    int maxy;
    int maxz;
public:
    tripoint_range( int minx, int miny, int minz, int maxx, int maxy, int maxz );

    point_generator begin() const;
    point_generator end() const;
};

#endif
