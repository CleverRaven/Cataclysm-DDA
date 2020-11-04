#!/bin/sh

suffix=`date|md5sum|awk '{print $1}'`
tpchr=/tmp/_pchr_$suffix.hpp
tpch=/tmp/_pch_$suffix.hpp
pch=pch/main-pch.hpp

grep '#include <' -R src|grep -v 'NOPCH'|awk -F '[: ]' '{print $2 " " $3}' > $tpchr
grep -v '\.h' $tpchr|sort|uniq >> $tpch

if [[ `diff $tpch $pch` != '' ]]; then
 diff $pch $tpch
 cp $tpch $pch
 echo pch **updated**
else
 echo pch up to date
fi

rm $tpch $tpchr
