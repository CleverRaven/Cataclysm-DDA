#include "setvector.h"

void setvector(std::vector<int> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 int tmp;
 while ((tmp = (int)va_arg(ap, int)))
  vec->push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<mon_id> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 mon_id tmp;
 while ((tmp = (mon_id)va_arg(ap, int)))
  vec->push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<items_location_and_chance> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 const char* tmploc;
 int tmpchance;
 while ((tmploc = va_arg(ap, const char*))) {
  tmpchance = (int)va_arg(ap, int);
  vec->push_back(items_location_and_chance(std::string(tmploc), tmpchance));
 }
 va_end(ap);
}

void setvector(std::vector<mission_origin> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 mission_origin tmp;
 while ((tmp = (mission_origin)va_arg(ap, int)))
  vec->push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<std::string> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 char *tmp;
 while ((tmp = va_arg(ap, char *)))
  vec->push_back((std::string)(tmp));
 va_end(ap);
}


template <class T> void setvec(std::vector<T> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 T tmp;
 while ((tmp = (T)va_arg(ap, int)))
  vec->push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<m_flag> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 m_flag tmp;
 while ((tmp = (m_flag)va_arg(ap, int)))
  vec->push_back(tmp);
 va_end(ap);
}

void setvector(std::vector<monster_trigger> *vec, ... )
{
 va_list ap;
 va_start(ap, vec);
 monster_trigger tmp;
 while ((tmp = (monster_trigger)va_arg(ap, int)))
  vec->push_back(tmp);
 va_end(ap);
}

