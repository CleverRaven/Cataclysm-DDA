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
