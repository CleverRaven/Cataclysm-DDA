#include "graffiti.h"
#include "output.h"

graffiti::graffiti()
{
 this->contents = NULL;
}

graffiti::graffiti(std::string newstr)
{
 this->contents = new std::string(newstr);
}

graffiti::~graffiti()
{
// delete contents;
}

graffiti graffiti::operator=(graffiti rhs)
{
 if(rhs.contents)
  this->contents = new std::string(*rhs.contents);
 else
  this->contents = 0;
 return *this;
}
