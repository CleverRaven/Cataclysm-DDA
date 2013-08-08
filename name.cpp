#include <map>

#include "catajson.h"
#include "name.h"
#include "output.h"
#include "translations.h"

NameGenerator::NameGenerator() {
    catajson name_records("data/raw/names.json");

    for (name_records.set_begin(); name_records.has_curr() && json_good(); name_records.next())
    {
        catajson name_entry = name_records.curr();
        std::string name = name_entry.get("name").as_string();
        std::string usage = name_entry.get("usage").as_string();
        uint32_t flags = 0;

        name = rm_prefix(_(("<name>"+name).c_str()));

        if (usage == "given") {
            flags |= nameIsGivenName;
        } else if (usage == "family") {
            flags |= nameIsFamilyName;
        } else if (usage == "universal") {
            flags |= nameIsGivenName | nameIsFamilyName;
        } else if (usage == "city") {
            flags |= nameIsTownName;
        }

        // Gender is optional
        if(name_entry.has("gender"))
        {
            std::string gender = name_entry.get("gender").as_string();

            if (gender == "male") {
                flags |= nameIsMaleName;
            } else if (gender == "female") {
                flags |= nameIsFemaleName;
            } else if (gender == "unisex") {
                flags |= nameIsUnisexName;
            }
        }

        Name aName(name, flags);

        names.push_back(aName);
    }

    if(names.empty())
    {
        Name aName;
        names.push_back(aName);
    }
    if(!json_good())
    {
        picojson::set_last_error("");
    }
}

std::vector<std::string> NameGenerator::filteredNames(uint32_t searchFlags) {

  std::vector<std::string> retval;

  for (std::vector<Name>::const_iterator aName = names.begin(); aName != names.end(); ++aName) {
    if ((aName->flags() & searchFlags) == searchFlags)
      retval.push_back(aName->value());
  }

  return retval;
}

std::string NameGenerator::getName(uint32_t searchFlags) {
  std::vector<std::string> theseNames = filteredNames(searchFlags);

  return theseNames[std::rand() % theseNames.size()];
}

std::string NameGenerator::generateName(bool male) {
  uint32_t baseSearchFlags = male ? nameIsMaleName : nameIsFemaleName;

  return rmp_format(_("<name>%s %s"),
      getName(baseSearchFlags | nameIsGivenName).c_str(),
      getName(baseSearchFlags | nameIsFamilyName).c_str());
}

NameGenerator& Name::generator() {
  return NameGenerator::generator();
}

std::string Name::generate(bool male) {
  return NameGenerator::generator().generateName(male);
}

std::string Name::get(uint32_t searchFlags) {
  return NameGenerator::generator().getName(searchFlags);
}

Name::Name() {
  _value = _("Tom");
  _flags = 15;
}

Name::Name(std::string name, uint32_t flags) {
  _value = name;
  _flags = flags;
}
