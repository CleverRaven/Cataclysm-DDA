#ifndef _INIT_H_
#define _INIT_H_

#include "json.h"

#include <string>
#include <vector>

//********** Functor Base, Static and Class member accessors
class TFunctor
{
public:
    virtual void operator ()(JsonObject &jo) = 0; // virtual () operator
    virtual void Call(JsonObject &jo) = 0; // what will be getting called
    virtual ~TFunctor() {};
};

class StaticFunctionAccessor : public TFunctor
{
private:
    void (*_fptr)(JsonObject &jo);

public:
    virtual void operator()(JsonObject &jo)
    {
        (*_fptr)(jo);
    }
    virtual void Call(JsonObject &jo)
    {
        (*_fptr)(jo);
    }

    StaticFunctionAccessor(void (*fptr)(JsonObject &jo))
    {
        _fptr = fptr;
    }

    ~StaticFunctionAccessor()
    {
        _fptr = NULL;
    }
};
template <class TClass> class ClassFunctionAccessor : public TFunctor
{
private:
    void (TClass::*_fptr)(JsonObject &jo);
    TClass *ptr_to_obj;

public:
    virtual void operator()(JsonObject &jo)
    {
        (*ptr_to_obj.*_fptr)(jo);
    }
    virtual void Call(JsonObject &jo)
    {
        (*ptr_to_obj.*_fptr)(jo);
    }

    ClassFunctionAccessor(TClass *ptr2obj, void (TClass::*fptr)(JsonObject &jo))
    {
        ptr_to_obj = ptr2obj;
        _fptr = fptr;
    }

    ~ClassFunctionAccessor()
    {
        _fptr = NULL;
        ptr_to_obj = NULL;
    }
};
//********** END - Functor Base, Static and Class member accessors

void load_object(JsonObject &jsobj);
void init_data_structures();
void release_data_structures();
void unload_active_json_data();

void load_json_dir(std::string const &dirname);
void load_json_files(std::vector<std::string> const &files);
void load_all_from_json(JsonIn &jsin);

void init_names();

#endif // _INIT_H_
