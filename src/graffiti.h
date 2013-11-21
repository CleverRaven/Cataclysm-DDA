#ifndef _GRAFFITI_H_
#define _GRAFFITI_H_

#include <string>
#include "itype.h"
#include "mtype.h"

class graffiti
{
public:
 graffiti();
 graffiti(std::string contents);
 ~graffiti();
 graffiti operator=(graffiti rhs);
 std::string *contents;
};

#endif
