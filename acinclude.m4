# Configure paths for libao
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_AO([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libao, and define AO_CFLAGS and AO_LIBS
dnl
AC_DEFUN(AM_PATH_AO,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ao-prefix,[  --with-ao-prefix=PFX   Prefix where libao is installed (optional)], ao_prefix="$withval", ao_prefix="")
AC_ARG_ENABLE(aotest, [  --disable-aotest       Do not try to compile and run a test ao program],, enable_aotest=yes)

  if test x$ao_prefix != x ; then
    ao_args="$ao_args --prefix=$ao_prefix"
    AO_CFLAGS="-I$ao_prefix/include"
    AO_LIBS="-L$ao_prefix/lib"
  fi

  AO_LIBS="$AO_LIBS -lao"

  AC_MSG_CHECKING(for ao)
  no_ao=""


  if test "x$enable_aotest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $AO_CFLAGS"
    LIBS="$LIBS $AO_LIBS"
dnl
dnl Now check if the installed ao is sufficiently new.
dnl
      rm -f conf.aotest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>

int main ()
{
  system("touch conf.aotest");
  return 0;
}

],, no_ao=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ao" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.aotest ; then
       :
     else
       echo "*** Could not run ao test program, checking why..."
       CFLAGS="$CFLAGS $AO_CFLAGS"
       LIBS="$LIBS $AO_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ao/ao.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding ao or finding the wrong"
       echo "*** version of ao. If it is not finding ao, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means ao was incorrectly installed"
       echo "*** or that you have moved ao since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     AO_CFLAGS=""
     AO_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(AO_CFLAGS)
  AC_SUBST(AO_LIBS)
  rm -f conf.aotest
])

# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_OGG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libogg, and define OGG_CFLAGS and OGG_LIBS
dnl
AC_DEFUN(AM_PATH_OGG,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ogg-prefix,[  --with-ogg-prefix=PFX   Prefix where libogg is installed (optional)], ogg_prefix="$withval", ogg_prefix="")
AC_ARG_ENABLE(oggtest, [  --disable-oggtest       Do not try to compile and run a test Ogg program],, enable_oggtest=yes)

  if test x$ogg_prefix != x ; then
    ogg_args="$ogg_args --prefix=$ogg_prefix"
    OGG_CFLAGS="-I$ogg_prefix/include"
    OGG_LIBS="-L$ogg_prefix/lib"
  fi

  OGG_LIBS="$OGG_LIBS -logg"

  AC_MSG_CHECKING(for Ogg)
  no_ogg=""


  if test "x$enable_oggtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $OGG_CFLAGS"
    LIBS="$LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Ogg is sufficiently new.
dnl
      rm -f conf.oggtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main ()
{
  system("touch conf.oggtest");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ogg" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.oggtest ; then
       :
     else
       echo "*** Could not run Ogg test program, checking why..."
       CFLAGS="$CFLAGS $OGG_CFLAGS"
       LIBS="$LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Ogg or finding the wrong"
       echo "*** version of Ogg. If it is not finding Ogg, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Ogg was incorrectly installed"
       echo "*** or that you have moved Ogg since it was installed. In the latter case, you"
       echo "*** may want to edit the ogg-config script: $OGG_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     OGG_CFLAGS=""
     OGG_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OGG_CFLAGS)
  AC_SUBST(OGG_LIBS)
  rm -f conf.oggtest
])

# Configure paths for libvorbis
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_VORBIS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libvorbis, and define VORBIS_CFLAGS and VORBIS_LIBS
dnl
AC_DEFUN(AM_PATH_VORBIS,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(vorbis-prefix,[  --with-vorbis-prefix=PFX   Prefix where libvorbis is installed (optional)], vorbis_prefix="$withval", vorbis_prefix="")
AC_ARG_ENABLE(vorbistest, [  --disable-vorbistest       Do not try to compile and run a test Vorbis program],, enable_vorbistest=yes)

  if test x$vorbis_prefix != x ; then
    vorbis_args="$vorbis_args --prefix=$vorbis_prefix"
    VORBIS_CFLAGS="-I$vorbis_prefix/include"
    VORBIS_LIBDIR="-L$vorbis_prefix/lib"
  fi

  VORBIS_LIBS="$VORBIS_LIBDIR -lvorbis -lm"
  VORBISFILE_LIBS="-lvorbisfile"
  VORBISENC_LIBS="-lvorbisenc"

  AC_MSG_CHECKING(for Vorbis)
  no_vorbis=""


  if test "x$enable_vorbistest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $VORBIS_CFLAGS"
    LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Vorbis is sufficiently new.
dnl
      rm -f conf.vorbistest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

int main ()
{
  system("touch conf.vorbistest");
  return 0;
}

],, no_vorbis=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_vorbis" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.vorbistest ; then
       :
     else
       echo "*** Could not run Vorbis test program, checking why..."
       CFLAGS="$CFLAGS $VORBIS_CFLAGS"
       LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <vorbis/codec.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Vorbis or finding the wrong"
       echo "*** version of Vorbis. If it is not finding Vorbis, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Vorbis was incorrectly installed"
       echo "*** or that you have moved Vorbis since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     VORBIS_CFLAGS=""
     VORBIS_LIBS=""
     VORBISFILE_LIBS=""
     VORBISENC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(VORBIS_CFLAGS)
  AC_SUBST(VORBIS_LIBS)
  AC_SUBST(VORBISFILE_LIBS)
  AC_SUBST(VORBISENC_LIBS)
  rm -f conf.vorbistest
])

dnl XIPH_PATH_SHOUT
dnl Jack Moffitt <jack@icecast.org> 08-06-2001
dnl Rewritten for libshout 2
dnl Brendan Cully <brendan@xiph.org> 20030612
dnl
dnl $Id: shout.m4,v 1.12 2003/07/11 01:26:31 brendan Exp $

# XIPH_PATH_SHOUT([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
# Test for libshout, and define SHOUT_CPPFLAGS SHOUT_CFLAGS SHOUT_LIBS, and
# SHOUT_THREADSAFE
AC_DEFUN([XIPH_PATH_SHOUT],
[dnl
xt_have_shout="no"
SHOUT_THREADSAFE="no"
SHOUT_CPPFLAGS=""
SHOUT_CFLAGS=""
SHOUT_LIBS=""

# NB: PKG_CHECK_MODULES exits if pkg-config is unavailable on the targe
# system, so we can't use it.

# seed pkg-config with the default libshout location
PKG_CONFIG_PATH=${PKG_CONFIG_PATH:-/usr/local/lib/pkgconfig}
export PKG_CONFIG_PATH

# Step 1: Use pkg-config if available
AC_PATH_PROG([PKGCONFIG], [pkg-config], [no])
if test "$PKGCONFIG" != "no" && `$PKGCONFIG --exists shout`
then
  SHOUT_CFLAGS=`$PKGCONFIG --variable=cflags_only shout`
  SHOUT_CPPFLAGS=`$PKGCONFIG --variable=cppflags shout`
  SHOUT_LIBS=`$PKGCONFIG --libs shout`
  xt_have_shout="maybe"
else
  if test "$PKGCONFIG" != "no"
  then
    AC_MSG_NOTICE([$PKGCONFIG couldn't find libshout. Try adjusting PKG_CONFIG_PATH.])
  fi
  # pkg-config unavailable, try shout-config
  AC_PATH_PROG([SHOUTCONFIG], [shout-config], [no])
  if test "$SHOUTCONFIG" != "no" && test `$SHOUTCONFIG --package` = "libshout"
  then
    SHOUT_CPPFLAGS=`$SHOUTCONFIG --cppflags`
    SHOUT_CFLAGS=`$SHOUTCONFIG --cflags-only`
    SHOUT_LIBS=`$SHOUTCONFIG --libs`
    xt_have_shout="maybe"
  fi
fi

# Now try actually using libshout
if test "$xt_have_shout" != "no"
then
  ac_save_CPPFLAGS="$CPPFLAGS"
  ac_save_CFLAGS="$CFLAGS"
  ac_save_LIBS="$LIBS"
  CPPFLAGS="$CPPFLAGS $SHOUT_CPPFLAGS"
  CFLAGS="$CFLAGS $SHOUT_CFLAGS"
  LIBS="$SHOUT_LIBS $LIBS"
  AC_CHECK_HEADERS([shout/shout.h], [
    AC_CHECK_FUNC([shout_new], [
      ifelse([$1], , :, [$1])
      xt_have_shout="yes"
    ])
    AC_EGREP_CPP([yes], [#include <shout/shout.h>
#if SHOUT_THREADSAFE
yes
#endif
], [SHOUT_THREADSAFE="yes"])
  ])
  CPPFLAGS="$ac_save_CPPFLAGS"
  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"
fi

if test "$xt_have_shout" != "yes"
then
  ifelse([$2], , :, [$2])
fi
AC_SUBST(SHOUT_CFLAGS)
AC_SUBST(SHOUT_LIBS)
AC_SUBST(SHOUT_CPPFLAGS)
])dnl XIPH_PATH_SHOUT

dnl Check for location of python.
AC_DEFUN(PY_PROG_PYTHON,
	[AC_ARG_WITH(python,
		[  --with-python=CMD       name of python executable],,[with_python=no])
	if test "x$with_python" != xno && test -x "$with_python"; then
		AC_MSG_CHECKING(for python)
		PYTHON="$with_python"
		AC_MSG_RESULT($PYTHON)
		AC_SUBST(PYTHON)
	else
		AC_PATH_PROG(PYTHON, python, python)
	fi]
)

dnl Check for python version
AC_DEFUN(PY_PYTHON_VERSION,
	[AC_REQUIRE([PY_PROG_PYTHON])
		AC_ARG_WITH(python-version,
		[  --with-python-version=VERSION
			overide guessed python version],,
			[with_python_version=no])
	if test "x$with_python_version" != xno; then
		PYTHON_VERSION="$with_python_version"
	fi
	AC_MSG_CHECKING(python version)
	if test -z "$PYTHON_VERSION"; then
		AC_CACHE_VAL(py_cv_python_version,
		[changequote((_,_))
			py_cv_python_version=`$PYTHON -c 'import sys; print sys.version[:3]'`
		changequote([,])])
	else
		py_cv_python_version="$PYTHON_VERSION"
	fi
	PYTHON_VERSION="$py_cv_python_version"
	AC_MSG_RESULT($PYTHON_VERSION)
	VERSION=$PYTHON_VERSION
	AC_SUBST(VERSION)
	AC_SUBST(PYTHON_VERSION)
])

# Configure paths for libFLAC
# "Inspired" by ogg.m4

dnl AM_PATH_LIBFLAC([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libFLAC, and define LIBFLAC_CFLAGS and LIBFLAC_LIBS
dnl
AC_DEFUN(AM_PATH_LIBFLAC,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(libFLAC,[  --with-libFLAC=PFX   Prefix where libFLAC is installed (optional)], libFLAC_prefix="$withval", libFLAC_prefix="")
AC_ARG_WITH(libFLAC-libraries,[  --with-libFLAC-libraries=DIR   Directory where libFLAC library is installed (optional)], libFLAC_libraries="$withval", libFLAC_libraries="")
AC_ARG_WITH(libFLAC-includes,[  --with-libFLAC-includes=DIR   Directory where libFLAC header files are installed (optional)], libFLAC_includes="$withval", libFLAC_includes="")
AC_ARG_ENABLE(libFLACtest, [  --disable-libFLACtest       Do not try to compile and run a test libFLAC program],, enable_libFLACtest=yes)

  if test "x$libFLAC_libraries" != "x" ; then
    LIBFLAC_LIBS="-L$libFLAC_libraries"
  elif test "x$libFLAC_prefix" != "x" ; then
    LIBFLAC_LIBS="-L$libFLAC_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    LIBFLAC_LIBS="-L$prefix/lib"
  fi

  LIBFLAC_LIBS="$LIBFLAC_LIBS -lFLAC -lm"

  if test "x$libFLAC_includes" != "x" ; then
    LIBFLAC_CFLAGS="-I$libFLAC_includes"
  elif test "x$libFLAC_prefix" != "x" ; then
    LIBFLAC_CFLAGS="-I$libFLAC_prefix/include"
  elif test "$prefix" != "xNONE"; then
    LIBFLAC_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for libFLAC)
  no_libFLAC=""


  if test "x$enable_libFLACtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $LIBFLAC_CFLAGS"
    CXXFLAGS="$CXXFLAGS $LIBFLAC_CFLAGS"
    LIBS="$LIBS $LIBFLAC_LIBS"
dnl
dnl Now check if the installed libFLAC is sufficiently new.
dnl
      rm -f conf.libFLACtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FLAC/format.h>

int main ()
{
  system("touch conf.libFLACtest");
  return 0;
}

],, no_libFLAC=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_libFLAC" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.libFLACtest ; then
       :
     else
       echo "*** Could not run libFLAC test program, checking why..."
       CFLAGS="$CFLAGS $LIBFLAC_CFLAGS"
       LIBS="$LIBS $LIBFLAC_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <FLAC/format.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding libFLAC or finding the wrong"
       echo "*** version of libFLAC. If it is not finding libFLAC, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means libFLAC was incorrectly installed"
       echo "*** or that you have moved libFLAC since it was installed. In the latter case, you"
       echo "*** may want to edit the libFLAC-config script: $LIBFLAC_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     LIBFLAC_CFLAGS=""
     LIBFLAC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(LIBFLAC_CFLAGS)
  AC_SUBST(LIBFLAC_LIBS)
  rm -f conf.libFLACtest
])
