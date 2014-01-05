#ifndef _OVERMAPBUFFER_H_
#define _OVERMAPBUFFER_H_

#include "overmap.h"

class overmapbuffer
{
public:
    overmapbuffer();

    overmap &get( const int x, const int y );
    void save();
    void clear();

private:
    std::list<overmap> overmap_list;
};

extern overmapbuffer overmap_buffer;

#endif /* _OVERMAPBUFFER_H_ */
