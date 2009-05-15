#!/bin/sh

aclocal -I m4 || exit 1
autoheader || exit 1
autoconf   || exit 1
rm -rf autom4te.cache

echo "Now you are ready to run ./configure"
