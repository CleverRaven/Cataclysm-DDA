#ifndef _PROFESSION_H_
#define _PROFESSION_H_

#include <string>
#include <vector>
#include <map>
#include "pldata.h"
#include "addiction.h"

class profession;

typedef std::map<std::string, profession> profmap;

class profession
{
private:
    unsigned int _id; // used when we care about precise order, starts at 1
    std::string _ident;
    std::string _name;
    std::string _description;
    signed int _point_cost;
    std::vector<std::string> _starting_items;
    std::vector<addiction> _starting_addictions;

    void add_item(std::string item);
    void add_addiction(add_type, int);
public:
    //these three aren't meant for external use, but had to be made public regardless
    profession();
    profession(unsigned int id, std::string ident, std::string name, std::string description, signed int points);
    static profmap _all_profs;

    static profmap load_professions();

    // these should be the only ways used to get at professions
    static profession* prof(std::string ident);
    static profession* generic(); // points to the generic, default profession
    static bool exists(std::string ident);
    static profmap::const_iterator begin();
    static profmap::const_iterator end();
    static int count();

    static bool has_initialized();

    unsigned int id() const;
    std::string ident() const;
    std::string name() const;
    std::string description() const;
    signed int point_cost() const;
    std::vector<std::string> items() const;
    std::vector<addiction> addictions() const;
};

#endif
