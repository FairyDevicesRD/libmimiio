# SYNOPSIS
#
#    AX_MIMIXFE_BASE([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#    Test for the libmimixfe version 0 (or newer)
#    
#    If no path to the installed mimixfe library is given, the macro searchs under
#    /usr, /usr/local, /opt, /opt/local.
#
#    This macro calls;
# 
#       AC_SUBST(MIMIXFE_CPPFLAGS)
#       AC_SUBST(MIMIXFE_LDFLAGS)
#	    AC_SUBST(MIMIXFE_PREFIX)	
#
#    And sets;
#
#       HAVE_MIMIXFE
#
# LICENSE
#
#    Copyright (c) 2014 Fairy Devices Inc.
#
AC_DEFUN([AX_MIMIXFE_BASE],
[
    dnl Provide mimixfe-prefix
    AC_ARG_WITH([mimixfe-prefix],
        [AS_HELP_STRING([--with-mimixfe-prefix=<prefix>],
            [specify mimixfe base location @<:@default=standard locations@:>@])],
        [MIMIXFE_PREFIX="$with_mimixfe_prefix"],
        [MIMIXFE_PREFIX=]
    )

    AC_MSG_CHECKING([for mimixfe >= 0])

    dnl Search for mimixfe in some standard locations
    dnl FIXME: Check libraries also!
    if test "$MIMIXFE_PREFIX" = ""; then
        for MIMIXFE_PREFIX_TMP in /usr /usr/local /opt /opt/local ; do
	    if test -e "$MIMIXFE_PREFIX_TMP/include/XFERecorder.h" ; then
	       MIMIXFE_PREFIX="$MIMIXFE_PREFIX_TMP"
	    fi
	done
    fi

    if test ! -z "$MIMIXFE_PREFIX"; then
        AC_MSG_RESULT([yes])
        AC_SUBST([MIMIXFE_LDFLAGS], [-L$MIMIXFE_PREFIX/lib])
        AC_SUBST([MIMIXFE_CPPFLAGS], [-I$MIMIXFE_PREFIX/include])
	AC_SUBST([MIMIXFE_PREFIX], [$MIMIXFE_PREFIX])
        ifelse([$1], [], :, [$1])
    else
        AC_MSG_RESULT([no])
        ifelse([$2], [], :, [$2])
    fi
]) dnl AC_DEFUN
