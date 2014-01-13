#ifndef _NAME_H_
#define _NAME_H_

#include <string>
#include <vector>
#include <stdint.h>

typedef enum {
  nameIsMaleName = 1,
  nameIsFemaleName = 2,
  nameIsUnisexName = 3,
  nameIsGivenName = 4,
  nameIsFamilyName = 8,
  nameIsTownName = 16,
  nameIsFullName = 32,
  nameIsWorldName = 64
} nameFlags;

class NameGenerator;

class Name {
 public:
  Name();
  Name(std::string name, uint32_t flags);

  static NameGenerator& generator();
  static std::string generate(bool male);

  static std::string get(uint32_t searchFlags);

  std::string value() const { return _value; }
  uint32_t flags() const { return _flags; }

  bool isFirstName();
  bool isLastName();

  bool isMaleName();
  bool isFemaleName();
 private:
  std::string _value;
  uint32_t _flags;
};

class NameGenerator {
 public:
  static NameGenerator& generator() {
    static NameGenerator generator;

    return generator;
  }

  void load_name(JsonObject &jo);

  std::string generateName(bool male);

  std::vector<std::string> filteredNames(uint32_t searchFlags);
  std::string getName(uint32_t searchFlags);
  void clear_names();
 private:
  NameGenerator();

  NameGenerator(NameGenerator const&);
  void operator=(NameGenerator const&);

  std::vector<Name> names;
};

void load_names_from_file(const std::string &filename);

#endif
