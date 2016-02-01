#!/bin/bash

# regexps for default entries
ENTRIES=( "\"to_hit\".*:[[:space:]]*0" \
          "\"bashing\".*:[[:space:]]*0" \
          "\"cutting\".*:[[:space:]]*0" \
          "\"phase\".*":.*\"solid\" )
error_code=0

# read whitelist to array
IFS=$'\n' read -d '' -r -a filenames < json_whitelist

for file in "${filenames[@]}"; do
  for entry in "${ENTRIES[@]}"; do
    if [ `grep -c $entry $file` -gt 0 ]; then
      echo "Default values of optional parameters found:"
      grep -nH $entry $file | sed 's/:/, line /'
      error_code=1
    fi
  done
done
exit $error_code
