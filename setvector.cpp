#include "setvector.h"

void setvector(std::vector<itype_id> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 itype_id tmp;
 while (tmp = (itype_id)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<component> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 itype_id it_tmp;
 int n_tmp;
 while (it_tmp = (itype_id)va_arg(ap, int)) {
  n_tmp = (int)va_arg(ap, int);
  component tmp(it_tmp, n_tmp);
  vec.push_back(tmp);
 }
 va_end(ap);
}

void setvector(std::vector<mon_id> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 mon_id tmp;
 while (tmp = (mon_id)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<items_location_and_chance> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 items_location tmploc;
 int tmpchance;
 while (tmploc = (items_location)va_arg(ap, int)) {
  tmpchance = (int)va_arg(ap, int);
  vec.push_back(items_location_and_chance(tmploc, tmpchance));
 }
 va_end(ap);
}

void setvector(std::vector<mission_origin> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 mission_origin tmp;
 while (tmp = (mission_origin)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<std::string> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 char *tmp;
 while (tmp = (char *)va_arg(ap, int))
  vec.push_back((std::string)(tmp));
 va_end(ap);
}


template <class T> void setvec(std::vector<T> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 T tmp;
 while (tmp = (T)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}
 
void setvector(std::vector<pl_flag> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 pl_flag tmp;
 while (tmp = (pl_flag)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<m_flag> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 m_flag tmp;
 while (tmp = (m_flag)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<monster_trigger> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 monster_trigger tmp;
 while (tmp = (monster_trigger)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<moncat_id> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 moncat_id tmp;
 while (tmp = (moncat_id)va_arg(ap, int))
  vec.push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<style_move> &vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 char *tmpname;
 technique_id tmptech;
 int tmplevel;

 while (tmpname = (char *)va_arg(ap, int)) {
  tmptech = (technique_id)va_arg(ap, int);
  tmplevel = (int)va_arg(ap, int);
  vec.push_back( style_move(tmpname, tmptech, tmplevel) );
 }
 va_end(ap);
}
