#!/bin/sh

base_dir=`echo $1 | sed -e 's@\(.*\)/\(.*.c\)\$@\1/@'`

gcc -M -MG $* | sed -e 's@ /[^ ]*@@g' -e "s@^\(.*\)\.o:@$base_dir\1.d $base_dir\1.o:@"

