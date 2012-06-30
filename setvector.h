#ifndef _SETVECTOR_H_
#define _SETVECTOR_H_
#include <vector>
#include "itype.h"
#include "mongroup.h"
#include "mapitems.h"
#include "crafting.h"
#include "mission.h"
void setvector(std::vector <itype_id> &vec, ... );
void setvector(std::vector <component> &vec, ... );
void setvector(std::vector <mon_id> &vec, ... );
void setvector(std::vector <items_location_and_chance> &vec, ... );
void setvector(std::vector <mission_origin> &vec, ... );
void setvector(std::vector <std::string> &vec, ... );
void setvector(std::vector <pl_flag> &vec, ... );
void setvector(std::vector <m_flag> &vec, ... );
void setvector(std::vector <monster_trigger> &vec, ... );
void setvector(std::vector <moncat_id> &vec, ... );
void setvector(std::vector <style_move> &vec, ... );
template <class T> void setvec(std::vector<T> &vec, ... );
#endif
