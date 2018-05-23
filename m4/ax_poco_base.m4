# SYNOPSIS
#
#    AX_POCO_BASE([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#    Test for the POCO C++ libraries of version 1.5.2 (or newer)
#    
#    If no path to the installed Poco library is given, the macro searchs under
#    /usr, /usr/local, /opt, /opt/local.
#
#    This macro calls;
# 
#       AC_SUBST(POCO_CPPFLAGS)
#       AC_SUBST(POCO_LDFLAGS)
#	AC_SUBST(POCO_PREFIX)	
#
#    And sets;
#
#       HAVE_POCO
#
# LICENSE
#
#    Copyright (c) 2014 Fairy Devices Inc.
#
AC_DEFUN([AX_POCO_BASE],
[
    dnl Provide poco-prefix
    AC_ARG_WITH([poco-prefix],
        [AS_HELP_STRING([--with-poco-prefix=<prefix>],
            [specify Poco base location @<:@default=standard locations@:>@])],
        [POCO_PREFIX="$with_poco_prefix"],
        [POCO_PREFIX=]
    )

    AC_MSG_CHECKING([for Poco >= 1.7.5])

    dnl Search for Poco in some standard locations
    dnl FIXME: Check libraries also!
    if test "$POCO_PREFIX" = ""; then
        for POCO_PREFIX_TMP in /usr /usr/local /opt /opt/local ; do
	    if test -d "$POCO_PREFIX_TMP/include/Poco" && test -r "$POCO_PREFIX_TMP/include/Poco"; then
	       POCO_PREFIX="$POCO_PREFIX_TMP"
	    fi
	done
    fi

    ORIGINAL_LDFLAGS="$LDFLAGS"
    ORIGINAL_LIBS="$LIBS"
    ORIGINAL_CPPFLAGS="$CPPFLAGS"

    dnl Poco version and edition check
    CPPFLAGS="-I$POCO_PREFIX/include $ORIGINAL_CPPFLAGS"
    LDFLAGS="-L$POCO_PREFIX/lib $ORIGINAL_LDFLAGS"
    AC_LANG_PUSH([C++])
    LIBS="-lPocoFoundation -lPocoNetSSL -pthread"
    AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
           [[
		#include <Poco/Foundation.h>
		#include <Poco/Version.h>
		#include <Poco/URI.h>
		#include <Poco/Net/SSLManager.h>
           ]],
           [[		
	        Poco::URI uri;
		#if POCO_VERSION >= 0x01070000
		// Everything is OK
		#else
    		# error Poco version is too old
                #endif
           ]]) 
       ], [], [NO_POCO=yes]
    ) dnl AC_LINK_IFELSE
    AC_LANG_POP([C++])

    LIBS="$ORIGINAL_LIBS"
    LDFLAGS="$ORIGINAL_LDFLAGS"
    CPPFLAGS="$ORIGINAL_CPPFLAGS"

    if test -z "$NO_POCO"; then
        AC_MSG_RESULT([yes])
        AC_SUBST([POCO_LDFLAGS], [-L$POCO_PREFIX/lib])
        AC_SUBST([POCO_CPPFLAGS], [-I$POCO_PREFIX/include])
	AC_SUBST([POCO_PREFIX], [$POCO_PREFIX])
        ifelse([$1], [], :, [$1])
    else
        AC_MSG_RESULT([no])
        ifelse([$2], [], :, [$2])
    fi
]) dnl AC_DEFUN
