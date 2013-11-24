  cd $(dirname $(readlink -f "$0") )
  if [ ! -e "chkjson.cpp" ]; then
    echo -n "wat, no chkjson.cpp in" `pwd`
    exit 1
  fi
  if g++ -I. chkjson.cpp -I../../src ../../src/json.cpp -o chkjson; then
     ls -ld chkjson && echo run this from $(readlink -f ../..)
  fi
