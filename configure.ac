# Require autoconf version
AC_PREREQ(2.57)
# Initialize
AC_INIT([libmimiio],[3.0.0],[contact@fairydevices.jp],[libmimiio],[http://www.fairydevices.jp/])
AC_CONFIG_SRCDIR(src/mimiio.h)
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")
#AM_SILENT_RULES([yes])
m4_include([m4/pkg.m4])
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXX
if test "$CXX" = no || test "$CXX:$GXX" = "g++:"; then
  AC_MSG_ERROR([no C++ compiler found])
fi
# Do not check for F77
define([AC_LIBTOOL_LANG_F77_CONFIG],[:])

# AC_PROG_MKDIR_P
# is a backport of autoconf-2.60's AC_PROG_MKDIR_P.
# Remove this macro when we can assume autoconf >= 2.60.
m4_ifdef([AC_PROG_MKDIR_P], [], [
   AC_DEFUN([AC_PROG_MKDIR_P],
     [AC_REQUIRE([AM_PROG_MKDIR_P])dnl defined by automake
      MKDIR_P='$(mkdir_p)'
      AC_SUBST([MKDIR_P])])])
AC_PROG_MKDIR_P

# htmldir is not defined prior to autoconf 2.59c, so on earlier versions
# set an equivalent value.
AC_PREREQ([2.59c], [], [AC_SUBST([htmldir], [m4_ifset([AC_PACKAGE_TARNAME],
	  ['${datadir}/doc/${PACKAGE_TARNAME}'],
	  ['${datadir}/doc/${PACKAGE}'])
])])

#------------------------------------------------------------------------------------        
# Rules for library version information:                           
# 
#  1. Start with version information of `0:0:0' for each libtool library. 
#  2. Update the version information only immediately before a public release of
#     your software. More frequent updates are unnecessary, and only guarantee
#     that the current interface number gets larger faster.
#  3. If the library source code has changed at all since the last update, then
#     increment revision (`c:r:a' becomes `c:r+1:a').                                     
#  4. If any interfaces have been added, removed, or changed since the last update,
#     increment current, and set revision to 0.
#  5. If any interfaces have been added since the last public release, then increment
#     age.
#  6. If any interfaces have been removed since the last public release, then set age
#     to 0.

SHARED_VERSION_INFO="3:0:0"
AC_PROG_LIBTOOL
LT_PROG_RC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_HEADER_STDC
AC_LANG([C])
# Support for mingw (experimental)
if test "x-$build_os-$host_os-$target_os" = x-mingw32-mingw32-mingw32 ; then
  AC_CHECK_PROG(DLLTOOL, dlltool, dlltool)
  AC_CHECK_PROG(DLLWRAP, dllwrap, dllwrap)
  AC_CHECK_PROG(PEXPORTS, pexports, pexports)
fi
# Handle options
AC_ARG_ENABLE(gcc-opt,
              AC_HELP_STRING([--disable-gcc-opt], [disable gcc optimisations]),
              ac_arg_gcc_opt="N", ac_arg_gcc_opt="Y")

# Enable debug
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Set default client cert
AC_ARG_WITH(ssl-default-cert,
AC_HELP_STRING([--with-ssl-default-cert=/path/to/defaultcert],
	       [set client default cert, default: bulit-in ssl librarie's default cert]),
	       [
	       SSL_DEFAULT_CERT="$withval"
	       WITH_SSL_DEFAULT_CERT=1
	       AC_DEFINE_UNQUOTED([WITH_SSL_DEFAULT_CERT],${WITH_SSL_DEFAULT_CERT}, [Define to 1 if you specified default cert])
	       AC_DEFINE_UNQUOTED([SSL_DEFAULT_CERT], "${SSL_DEFAULT_CERT}", [Path to default cert, which is override for built-in default cert.])
	       ],
	       [
	       SSL_DEFAULT_CERT=""
	       WITH_SSL_DEFAULT_CERT=0
	       ]
)
AC_SUBST(SSL_DEFAULT_CERT)
AC_SUBST(WITH_SSL_DEFAULT_CERT)

# Check types and sizes
AC_CHECK_SIZEOF(int,0)
AC_CHECK_SIZEOF(long,0)
AC_CHECK_SIZEOF(float,4)
AC_CHECK_SIZEOF(double,8)
# Determine endian-ness of target processor
#AC_C_FIND_ENDIAN
#AC_DEFINE_UNQUOTED(CPU_IS_BIG_ENDIAN, ${ac_cv_c_big_endian},
#        [Target processor is big endian.])
#AC_DEFINE_UNQUOTED(CPU_IS_LITTLE_ENDIAN, ${ac_cv_c_little_endian},
#        [Target processor is little endian.])
# Flags
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
CFLAGS="-Wall"
# Include directories needed by all implementations
CFLAGS="$CFLAGS -I\$(top_srcdir)/include -I\$(top_srcdir)/src" 
CXXFLAGS=$CXXFLAGS
LDFLAGS="$LDFLAGS -lm"
# Check for functions
# AC_CHECK_FUNCS(malloc calloc free memcpy memmove alarm signal)
# AC_CHECK_LIB([m],floor)
# Check for pkg-config outsize if statement
PKG_PROG_PKG_CONFIG

# Check for POCO 1.7.0
AX_POCO_BASE([], AC_MSG_ERROR([Poco C++ library Complete Edition >= 1.7.0 is not found in standard locations. Please specify Poco installed directory with --with-poco-prefix=<prefix>. See http://pocoproject.org/ about Poco C++ library.]))

# Check for portaudio
PKG_CHECK_MODULES(PORTAUDIO, portaudio-2.0 >= 19, ac_cv_portaudio=1, ac_cv_portaudio=0)
AC_DEFINE_UNQUOTED([HAVE_PORTAUDIO], $ac_cv_portaudio, [Set 1 if you have portaudio.])
if test x$ac_cv_portaudio = x1; then
   ac_cv_portaudio=yes
   HAVE_PORTAUDIO=1
else
   ac_cv_portaudio=no
   HAVE_PORTAUDIO=0
fi
AM_CONDITIONAL(PA_DEP_BUILD, test x$ac_cv_portaudio = xyes)
AC_SUBST(HAVE_PORTAUDIO)

PKG_CHECK_MODULES(FLAC, flac++, ac_cv_flac=1, ac_cv_flac=0)
AC_DEFINE_UNQUOTED([HAVE_FLAC], $ac_cv_flac, [Set 1 if you have libflac.])
if test x$ac_cv_flac = x1; then
   ac_cv_flac=yes
   HAVE_FLAC=1
else
   ac_cv_flac=0
   HAVE_FLAC=0
   AC_MSG_ERROR([libFLAC++ is not found in standard locations.])
fi
AM_CONDITIONAL(FLAC_DEP_BUILD, test x$ac_cv_flac = xyes)
AC_SUBST(HAVE_FLAC)

# Check for libmimixfe
#AX_MIMIXFE_BASE(AC_MSG_ERROR([OK]), AC_MSG_ERROR(NG))
AX_MIMIXFE_BASE(ac_cv_mimixfe=1, ac_cv_mimixfe=0)
AC_DEFINE_UNQUOTED([HAVE_MIMIXFE], $ac_cv_mimixfe, [Set 1 if you have mimixfe.])
if test x$ac_cv_mimixfe = x1; then
   ac_cv_mimixfe=yes
   HAVE_MIMIXFE=1
else
   ac_cv_mimixfe=no
   HAVE_MIMIXFE=0
fi
AM_CONDITIONAL(MIMIXFE_DEP_BUILD, test x$ac_cv_mimixfe = xyes)
AC_SUBST(HAVE_MIMIXFE)

# GCC misc
if test $ac_cv_c_compiler_gnu = yes; then
   if test "$ac_arg_gcc_opt" = "N" ; then
      temp_CFLAGS=`echo $CFLAGS | sed "s/O2/O0/"`
      CFLAGS=$temp_CFLAGS
      AC_MSG_WARN([[*** Compiler optimisations switched off. ***]])
      fi
                  
case "$target_os" in
      #darwin* | rhapsody*)
      #	    temp_CFLAGS=`echo $CFLAGS | sed "s/-Wall//"`
      #     CFLAGS=$temp_CFLAGS
      #     ;;

      linux*|kfreebsd*-gnu*|gnu*)
            SHLIB_VERSION_ARG="-Wl,--version-script=Version_script"
            ;;
      *)
            ;;
      esac

      AC_DEFINE([COMPILER_IS_GCC],1, [Set to 1 if the compile is GNU GCC.])
      GCC_MAJOR_VERSION=`$CC -dumpversion | sed "s/\..*//"`
      AC_DEFINE_UNQUOTED([GCC_MAJOR_VERSION],${GCC_MAJOR_VERSION}, [Major version of GCC or 3 otherwise.])
      fi
# Target OS specific something
OS_SPECIFIC_INCLUDES=""
os_is_win32=0
case "$target_os" in
        darwin* | rhapsody*)
		#no specific include so far
                #OS_SPECIFIC_INCLUDES="-fpascal-strings -I/Developer/Headers/FlatCarbon"
                #OS_SPECIFIC_LINKS="-framework CoreAudio"
                ;;
        mingw32*)
                OS_SPECIFIC_LINKS="-lwinmm"
                os_is_win32=1
                ;;
        *)
                OS_SPECIFIC_INCLUDES=""
                OS_SPECIFIC_LINKS=""
                ;;
        esac

# subst variables
AC_DEFINE_UNQUOTED(OS_IS_WIN32, ${os_is_win32}, [Set to 1 if compiling for Win32])

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)
AC_SUBST(OS_SPECIFIC_INCLUDES)
AC_SUBST(OS_SPECIFIC_LINKS)

AC_SUBST(COMPILER_IS_GCC)
AC_SUBST(GCC_MAJOR_VERSION)

AC_SUBST(HAVE_POCO)
AC_SUBST(POCO_CPPFLAGS)
AC_SUBST(POCO_LIBS)

AC_SUBST(HAVE_PORTAUDIO)
AC_SUBST(PORTAUDIO_CPPFLAGS)
AC_SUBST(PORTAUDIO_LDFLAGS)

AC_CONFIG_FILES([
 Makefile
 m4/Makefile
 src/Makefile
 examples/Makefile
 examples/mimiio_file/Makefile
 examples/mimiio_pa/Makefile
 examples/mimiio_tumbler/Makefile
 examples/mimiio_tumbler/mimiio_tumbler_ex1/Makefile
 examples/mimiio_tumbler/mimiio_tumbler_ex2/Makefile
 examples/mimiio_tumbler/mimiio_tumbler_ex3/Makefile
 doc/Makefile
 mimiio.pc
])
AC_OUTPUT

AC_MSG_RESULT([
-=-=-=-=-=-=-=-=-=-= Configuration Complete =-=-=-=-=-=-=-=-=-=-=-

  Configuration summary :

    Version : ..................... ${VERSION}

    Host CPU : .................... ${host_cpu}
    Host Vendor : ................. ${host_vendor}
    Host OS : ..................... ${host_os}
])

if test x$ac_cv_c_compiler_gnu = xyes ; then
echo "  Tools :\n"
echo "    Compiler is GCC : ............. ${ac_cv_c_compiler_gnu}"
echo "    GCC major version : ........... ${GCC_MAJOR_VERSION}"
echo ""
fi

if test ${WITH_SSL_DEFAULT_CERT} = 1 ; then
echo "  SSL Client Cert:  ............... ${SSL_DEFAULT_CERT}\n"
else
echo "  SSL Client Cert:  ............... system built-in default cert\n"
fi

if test x"$debug" = x"true" ; then
echo "  Debug Build:  ................... yes (-g -O0 )\n"
else
echo "  Debug Build:  ................... no (-g -O2 )\n"
fi 

AC_MSG_RESULT([  Dependent libraries : 

    Poco C++ library LDFLAGS : .... ${POCO_LDFLAGS}
    Poco C++ library CFLAGS : ..... ${POCO_CPPFLAGS}
    libFLAC++ : ................... ${FLAC_LIBS}
]) 

AC_MSG_RESULT([  Optional libraries for usage examples: ])

AC_MSG_RESULT([
    Portaudio > v19        ........ ${ac_cv_portaudio}
    mimixfe > v0           ........ ${ac_cv_mimixfe}
]) 

AC_MSG_RESULT([  Installation directories :

    Library directory : ........... ${prefix}/lib
    Program directory : ........... ${prefix}/bin
    Pkgconfig directory : ......... ${prefix}/lib/pkgconfig
])

if test x$prefix != "x/usr" ; then
echo "Compiling some other packages against ${PACKAGE} may require "
echo "the addition of \"${prefix}/lib/pkgconfig\" to the "
echo "PKG_CONFIG_PATH environment variable.\n"
fi
