#ifndef _NAME_H_
#define _NAME_H_

#include <string>
#include <vector>

typedef enum {
  nameIsMaleName = 1,
  nameIsFemaleName = 2,
  nameIsUnisexName = 3,
  nameIsGivenName = 4,
  nameIsFamilyName = 8
} nameFlags;

class NameGenerator;

class Name {
 public:
  Name();
  Name(std::string name, uint32_t flags);

  static NameGenerator& generator();
  static std::string generate(bool male);

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

  std::string generateName(bool male);
 private:
  NameGenerator();

  NameGenerator(NameGenerator const&);
  void operator=(NameGenerator const&);

  std::vector<Name> names;
};

#endif
