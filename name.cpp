#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>

#include "name.h"
#include "picojson.h"

NameGenerator::NameGenerator() {
  std::ifstream rawFile;
  picojson::value rawData;

  rawFile.open("data/raw/names.json");

  rawFile >> rawData;

  rawFile.close();

  const picojson::array& rawNames = rawData.get<picojson::array>();
  for (picojson::array::const_iterator i = rawNames.begin(); i != rawNames.end(); ++i) {
    std::map<std::string,picojson::value> aNameData = i->get<std::map<std::string,picojson::value> >();

    std::string name, gender, usage;
    uint32_t flags = 0;

    name = aNameData["name"].get<std::string>();

    gender = aNameData["gender"].get<std::string>();

    if (gender == "male") {
      flags |= nameIsMaleName;
    } else if (gender == "female") {
      flags |= nameIsFemaleName;
    } else if (gender == "unisex") {
      flags |= nameIsUnisexName;
    }

    usage = aNameData["usage"].get<std::string>();

    if (usage == "given") {
      flags |= nameIsGivenName;
    } else if (usage == "family") {
      flags |= nameIsFamilyName;
    } else if (usage == "universal") {
      flags |= nameIsGivenName | nameIsFamilyName;
    }

    Name aName(name, flags);

    names.push_back(aName);
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

  return getName(baseSearchFlags | nameIsGivenName) + " " + getName(baseSearchFlags | nameIsFamilyName);
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
  _value = "Tom";
  _flags = 15;
}

Name::Name(std::string name, uint32_t flags) {
  _value = name;
  _flags = flags;
}
