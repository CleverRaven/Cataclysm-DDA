#ifndef _PROFESSION_H_
#define _PROFESSION_H_

#include <string>
#include <vector>
#include <map>

class profession;

typedef std::map<std::string, profession> profmap;

class profession {
private:
 std::string _ident;
 std::string _name;
 std::string _description;
 signed int _point_cost;
 std::vector<std::string> _starting_items;

 void add_item(std::string item);
public:
 //these three aren't meant for external use, but had to be made public regardless
 profession();
 profession(std::string ident, std::string name, std::string description, signed int points);
 static profmap _all_profs;

 static profmap load_professions();

 // these should be the only ways used to get at professions
 static profession* prof(std::string ident);
 static profession* generic(); // points to the generic, default profession
 static bool exists(std::string ident);
 static profmap::const_iterator begin();
 static profmap::const_iterator end();

 static bool has_initialized();

 std::string ident() const;
 std::string name() const;
 std::string description() const;
 signed int point_cost() const;
 std::vector<std::string> items() const;
};

#endif
