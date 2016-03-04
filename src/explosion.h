#ifndef EXPLOSION_H
#define EXPLOSION_H

#include <unordered_map>

#include "enums.h"

class explosion
{
	/** all affected tiles and sum damage received (if any) from blast and shrapnel */
	using result = std::unordered_map<tripoint,std::pair<int,int>> data;
};

#endif

