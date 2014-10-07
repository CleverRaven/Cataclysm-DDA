#ifndef GRAFFITI_H
#define GRAFFITI_H

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
