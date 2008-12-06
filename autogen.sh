#!/bin/sh

aclocal
#autoheader
autoconf

if test -f ./configure; then
echo "Now you are ready to run ./configure"
fi

