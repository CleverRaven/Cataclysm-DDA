  cd $(dirname $(readlink -f "$0") )
  if [ ! -e "chkjson.cpp" ]; then
    echo -n "wat, no chkjson.cpp in" `pwd`
    exit 1
  fi
  cat > catacharset.h << END
/*
    Stub out additional requirements and butt in before real catacharset is included
*/
#ifndef CATACHARSET_H
#define CATACHARSET_H 1
#include <stdint.h>
#include <string>
std::string utf32_to_utf8(unsigned ch) {
    return std::string("-stub-");
}
#endif
END
  if g++ -I. chkjson.cpp -I../../src ../../src/json.cpp -o chkjson; then
     ls -ld chkjson && echo run this from $(readlink -f ../..)
  fi
  rm catacharset.h