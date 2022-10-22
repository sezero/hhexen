dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([hhexen],[1.6.4])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([base/a_action.c])

dnl == Default values
CFLAGS_ALL="-Wall"
CFLAGS_DBG="-g"
CFLAGS_OPT="-O2 -ffast-math -fomit-frame-pointer"

GLHEXEN="true"

dnl == Optimization
AC_ARG_ENABLE(debug,
[AS_HELP_STRING([--enable-debug], [debug build (no optimizations) [default=no]])],
  [], [enable_debug=no])
if test x$enable_debug = xyes; then
	CFLAGS="$CFLAGS $CFLAGS_ALL $CFLAGS_DBG"
else
	CFLAGS="$CFLAGS $CFLAGS_ALL $CFLAGS_OPT"
fi

dnl == Command-line args. Check for mutually-exclusive options.
AC_ARG_ENABLE(fullscreen,
[AS_HELP_STRING([--enable-fullscreen], [Run fullscreen by default [default=no]])],
  [], [enable_fullscreen=no])
if test x$enable_fullscreen = xyes; then
	AC_DEFINE(FULLSCREEN_DEFAULT, 1, [Define if want to run fullscreen by default instead of windowed])
fi

AC_ARG_ENABLE(userdirs,
[AS_HELP_STRING([--disable-userdirs], [Disable user directories [default=no]])],
  [], [enable_userdirs=yes])
if test x$enable_userdirs != xyes; then
	AC_DEFINE(_NO_USERDIRS, 1, [Define if want to disable user directories])
fi

AC_ARG_ENABLE(gl,
[AS_HELP_STRING([--disable-gl], [Disable OpenGL version [default=no]])],
  [], [enable_gl=yes])
if test x$enable_gl = xyes; then
	AC_DEFINE(RENDER3D, 1, [Define if building with OpenGL support])
else
	GLHEXEN="false"
fi

AC_SUBST(GLHEXEN)

AC_ARG_ENABLE(demowad,
[AS_HELP_STRING([--enable-demowad], [Build for the demo version wad only [default=no]])],
  [], [enable_demowad=no])
if test x$enable_demowad = xyes; then
	AC_DEFINE(DEMO_VERSION, 1, [Define if building for the 4 level shareware wad files only])
fi

AC_ARG_ENABLE(oldwad,
[AS_HELP_STRING([--enable-oldwad], [Build for the version 1.0 wad only [default=no]])],
  [], [enable_oldwad=no])
if test x$enable_oldwad = xyes; then
	AC_DEFINE(VERSION10_WAD, 1, [Define if building for version 1.0 (4-player) wadfiles only])
fi

AC_ARG_ENABLE(dossaves,
[AS_HELP_STRING([--enable-dossaves], [DOS-Hexen saved game compatibility (experimental) [default=no]])],
  [], [enable_dossaves=no])
if test x$enable_dossaves = xyes; then
	AC_DEFINE(_DOSSAVE_COMPAT, 1, [Define for saved game compatibility with DOS-Hexen (experimental)])
fi

AC_ARG_ENABLE(rangecheck,
[AS_HELP_STRING([--enable-rangecheck], [Enable parameter validation debugging code [default=no]])],
  [], [enable_rangecheck=no])
if test x$enable_rangecheck = xno; then
	AC_DEFINE(NORANGECHECKING, 1, [Define to not compile most parameter validation debugging code])
fi

AC_ARG_WITH(datapath,
[AS_HELP_STRING([--with-datapath=<DIR>],[Use DIR for shared game data. Not defined by default.])],
  [datapath="$withval"], [datapath=""])
AC_DEFINE_UNQUOTED(SHARED_DATAPATH, "$datapath", [Define this to the shared game directory root])
SHARED_DATAPATH="$datapath"

default_audio="sdl"

AC_ARG_WITH([audio],
[AS_HELP_STRING([--with-audio=<driver>], [If set, will change the audio driver.
Valid values are sdl and sdlmixer.  sdl is the default.
sdlmixer adds the ability of midi music playback.])],
  [audio_drv="$withval"], [audio_drv="$default_audio"])

case "$audio_drv" in
	sdl|sdlmixer)
		SND_MODULE="$audio_drv"
		;;
	*)
		AC_MSG_ERROR([*** invalid value for --with-audio])
		;;
esac

AC_SUBST(SND_MODULE)

dnl == Checks for programs.
AC_PROG_CC
AC_PROG_CPP

dnl == Checks for header files.
AC_CHECK_HEADERS(stdint.h inttypes.h strings.h)

dnl == Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_C_VOLATILE
AC_C_INLINE
AC_C_CHAR_UNSIGNED
AC_TYPE_SIZE_T

AC_C_BIGENDIAN

dnl == Check for gcc strength-reduce bug
if test "x${GCC}" = "xyes"; then
  AC_CACHE_CHECK( [for gcc strength-reduce bug], ac_cv_c_gcc_strength_bug,
                  AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main(void) {
  static int Array[[3]];
  unsigned int B = 3;
  int i;
  for(i=0; i<B; i++) Array[[i]] = i - 3;
  exit( Array[[1]] != -2 );
}]])],
    [ac_cv_c_gcc_strength_bug="no"],
    [ac_cv_c_gcc_strength_bug="yes"],
    [ac_cv_c_gcc_strength_bug="no"]) )
  if test "$ac_cv_c_gcc_strength_bug" = "yes"; then
    CFLAGS="$CFLAGS -fno-strength-reduce"
  fi
fi

AC_CHECK_LIB(m, pow,, AC_MSG_ERROR([*** math library (-lm) appears broken]))

dnl == Check for SDL
SDL_VERSION="2.0.2"
AM_PATH_SDL2($SDL_VERSION,[HAVESDL="yes"],[HAVESDL="no"])
if test "$HAVESDL" = "no"; then
    AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
fi

saved_libs="$LIBS"
AC_CHECK_LIB(SDL2_mixer, Mix_LoadMUS, [HAVE_SDLMIXER="yes"], [HAVE_SDLMIXER="no"], [$SDL_LIBS])
LIBS="$saved_libs"
if test "$SND_MODULE" = "sdlmixer";then
  if test $HAVE_SDLMIXER = "no";then
	AC_MSG_ERROR([*** SDL_mixer not found!])
  else
	SDL_LIBS="$SDL_LIBS -lSDL2_mixer"
  fi
fi

AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

dnl == Check for GL libraries
case "$host" in
    *-*-cygwin* | *-*-mingw32*)
        GL_LIBS="-lopengl32 -lglu32"
        ;;
dnl FIXME: handle Mac OS X properly.
dnl *-*-darwin*)
dnl     GL_LIBS=""
dnl     ;;
    *)
        GL_LIBS="-lGL -lGLU"
        ;;
esac

saved_libs="$LIBS"
LIBS="$LIBS $GL_LIBS"

AC_MSG_CHECKING(for OpenGL support)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <GL/gl.h>
#include <GL/glu.h>
]],[])],[HAVEGL="yes"],[HAVEGL="no"])
AC_MSG_RESULT($HAVEGL)
LIBS="$saved_libs"
if test "$GLHEXEN" = "true"; then
  if test "$HAVEGL" = "no"; then
	AC_MSG_ERROR([*** OpenGL not found!])
  fi
fi
AC_SUBST(GL_LIBS)

dnl == Generate output files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

dnl == Make a short report
echo
echo "HHexen configuration finished."

if test "$GLHEXEN" = "true"; then
    report="OpenGL (SDL)"
else
    report="Software (SDL)"
fi
echo "Enabled target       : $report"

echo "Audio driver         : $SND_MODULE"

if test x$enable_rangecheck = xno; then
    report="disabled"
else
    report="enabled"
fi
echo "Range checks (debug) : $report"

if test x$datapath = x; then
    report="(not set)"
else
    report="$datapath"
fi
echo "Shared game data path: $report"

if test x$enable_fullscreen = xyes; then
    report="fullscreen"
else
    report="windowed"
fi

echo "Default window mode  : $report"
echo

if test x$enable_dossaves = xyes; then
   if test x$enable_oldwad = xyes; then
	echo "WARNING: DOS-Hexen 1.0 saves are not compatible with HHexen, yet."
	echo
   fi
fi

echo "Type 'make' to compile."
echo

