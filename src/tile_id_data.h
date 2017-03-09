#pragma once
#ifndef TILE_ID_DATA_H
#define TILE_ID_DATA_H

#include <string>
#include <array>

#define toString(x) #x

const std::array<std::string, 8> multitile_keys = {{
        toString( center ),
        toString( corner ),
        toString( edge ),
        toString( t_connection ),
        toString( end_piece ),
        toString( unconnected ),
        toString( open ),
        toString( broken )
    }
};

#undef toString

#endif
