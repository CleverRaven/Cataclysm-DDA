#include <iostream>
#include <fstream>
#include <sstream>

#include "profession.h"
#include "output.h"

#include "picojson.h"

profession::profession() {
 _ident = "";
 _name = "null";
 _description = "null";
 _point_cost = 0;
}

profession::profession(std::string ident, std::string name, std::string description, signed int points) {
 _ident = ident;
 _name = name;
 _description = description;
 _point_cost = points;
}

profmap profession::_all_profs(profession::load_professions());

profmap profession::load_professions() {
 profmap allProfs;
 picojson::value profsRaw;
 std::ifstream profsFile;

 profsFile.open("data/raw/professions.json");

 profsFile >> profsRaw;

 if (profsRaw.is<picojson::array>()) {
  const picojson::array& profs = profsRaw.get<picojson::array>();
  for (picojson::array::const_iterator aProf = profs.begin(); aProf != profs.end(); ++aProf) {
   const picojson::object& object = aProf->get<picojson::object>();
   std::string ident, name, description;
   signed int points;

   ident = object.at("ident").get<std::string>();
   name = object.at("name").get<std::string>();
   description = object.at("description").get<std::string>();
   points = static_cast<int>(object.at("points").get<double>());
   
   profession newProfession(ident, name, description, points);

   const picojson::array& items = object.at("items").get<picojson::array>();
   for (picojson::array::const_iterator anItem = items.begin(); anItem != items.end(); ++anItem) {
    newProfession.add_item(anItem->get<std::string>());
   }

   allProfs[ident] = newProfession;
  }
 } else {
  std::cout << "Bad profession file:\n" << profsRaw << std::endl;
  exit(1);
 }
 
 return allProfs;
}

profession* profession::prof(std::string ident) {
 profmap::iterator prof = _all_profs.find(ident);
 if (prof != _all_profs.end()) {
  return &(prof->second);
 } else {
  debugmsg("Tried to get invalid profession: %s", ident.c_str());
 }
}

profession* profession::generic() {
 return profession::prof("unemployed");
}

bool profession::exists(std::string ident) {
 return _all_profs.find(ident) != _all_profs.end();
}

profmap::const_iterator profession::begin() {
 return _all_profs.begin();
}

profmap::const_iterator profession::end() {
 return _all_profs.end();
}

bool profession::has_initialized() {
 return exists("unemployed");
}

void profession::add_item(std::string item) {
 _starting_items.push_back(item);
}

std::string profession::ident() const {
 return _ident;
}

std::string profession::name() const {
 return _name;
}

std::string profession::description() const {
 return _description;
}

signed int profession::point_cost() const {
 return _point_cost;
}

std::vector<std::string> profession::items() const {
 return _starting_items;
}
