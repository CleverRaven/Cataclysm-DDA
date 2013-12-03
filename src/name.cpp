#include <map>
#include <fstream>

#include "json.h"
#include "name.h"
#include "output.h"
#include "translations.h"
#include "rng.h"

NameGenerator::NameGenerator() {

}

void NameGenerator::load_name(JsonObject &jo)
{
    std::string name = jo.get_string("name");
    std::string usage = jo.get_string("usage");
    uint32_t flags = 0;

    if (usage == "given") {
        flags |= nameIsGivenName;
        name = pgettext("Given Name", name.c_str());
    } else if (usage == "family") {
        flags |= nameIsFamilyName;
        name = pgettext("Family Name", name.c_str());
    } else if (usage == "universal") {
        flags |= nameIsGivenName | nameIsFamilyName;
        name = pgettext("Either Name", name.c_str());
    } else if (usage == "backer") {
        flags |= nameIsFullName;
        name = pgettext("Full Name", name.c_str());
    } else if (usage == "city") {
        flags |= nameIsTownName;
        name = pgettext("City Name", name.c_str());
    } else if (usage == "world") {
        flags |= nameIsWorldName;
        name = pgettext("World Name", name.c_str());
    }

    // Gender is optional
    if(jo.has_member("gender"))
    {
        std::string gender = jo.get_string("gender");

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
  return theseNames[rng(0, theseNames.size()-1)];
}

std::string NameGenerator::generateName(bool male) {
  uint32_t baseSearchFlags = male ? nameIsMaleName : nameIsFemaleName;
  //One in four chance to pull from the backer list, otherwise generate a name from the parts list
  if (one_in(4)){
    return getName(baseSearchFlags | nameIsFullName);
  } else {
    //~ used for constructing names. swapping these will put family name first.
    return string_format(pgettext("Full Name", "%1$s %2$s"),
        getName(baseSearchFlags | nameIsGivenName).c_str(),
        getName(baseSearchFlags | nameIsFamilyName).c_str()
        );
  }
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

void load_names_from_file(const std::string &filename)
{
    std::ifstream data_file;
    data_file.open(filename.c_str(), std::ifstream::in | std::ifstream::binary);
    if(!data_file.good()) {
        throw "Could not read " + filename;
    }

    NameGenerator &gen = NameGenerator::generator();
    JsonIn jsin(&data_file);

    // load em all
    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject json_name = jsin.get_object();
        gen.load_name(json_name);
    }
}

