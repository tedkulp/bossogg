#!/bin/sh
#
echo "Generating build information using aclocal, automake and autoconf"
echo "This may take a while ..."

export WANT_AUTOCONF_2_5=1
export WANT_AUTOCONF=2.5

chmod 755 bossogg boshell bosync

rm -f config.cache
rm -fr autom4te.cache/
rm -f configure

libtoolize --force

# Regenerate configuration files
aclocal-1.7
automake-1.7 --foreign --include-deps --add-missing
autoconf

# Run configure for this platform
#./configure $*
echo "Now you are ready to run ./configure"
