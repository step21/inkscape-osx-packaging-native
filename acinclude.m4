dnl  Usage: RELAYTOOL(LIBRARY_NAME, LIBS, CFLAGS, ACTION-IF-WEAK-LINK-IS-POSSIBLE)

dnl  Example:
dnl  RELAYTOOL("gtkspell", GTKSPELL_LIBS, GTKSPELL_CFLAGS, gtkspell_weak=yes)
dnl  Will modify GTKSPELL_LIBS to include a call to relaytool if available
dnl  or if not, will modify GTKSPELL_CFLAGS to include -D switches to define
dnl  libgtkspell_is_present=1 and libgtkspell_symbol_is_present=1

AC_DEFUN([RELAYTOOL], [
    if test -z "$RELAYTOOL_PROG"; then
        AC_PATH_PROG(RELAYTOOL_PROG, relaytool, no)
    fi

    AC_MSG_CHECKING(whether we can weak link $1)

    _RELAYTOOL_PROCESSED_NAME=`echo "$1" | sed 's/-/_/g;s/\./_/g;'`
    _RELAYTOOL_UPPER_NAME=`echo $_RELAYTOOL_PROCESSED_NAME | tr '[[:lower:]]' '[[:upper:]]'`

    if test "$RELAYTOOL_PROG" = "no"; then
        AC_MSG_RESULT(no)
        $3="-DRELAYTOOL_${_RELAYTOOL_UPPER_NAME}='static const int lib${_RELAYTOOL_PROCESSED_NAME}_is_present = 1; static int __attribute__((unused)) lib${_RELAYTOOL_PROCESSED_NAME}_symbol_is_present(char *m) { return 1; }' $$3"
    else
        AC_MSG_RESULT(yes)
        $2=`echo $$2|sed 's/\`/\\\\\`/g;'`
        $2="-Wl,--gc-sections \`relaytool --relay $1 $$2\`"
	$3="-DRELAYTOOL_${_RELAYTOOL_UPPER_NAME}='extern int lib${_RELAYTOOL_PROCESSED_NAME}_is_present; extern int lib${_RELAYTOOL_PROCESSED_NAME}_symbol_is_present(char *s);' $$3"
        $4
    fi
])

# ===========================================================================
#               http://autoconf-archive.cryp.to/ax_openmp.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_OPENMP([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro tries to find out how to compile programs that use OpenMP a
#   standard API and set of compiler directives for parallel programming
#   (see http://www-unix.mcs/)
#
#   On success, it sets the OPENMP_CFLAGS/OPENMP_CXXFLAGS/OPENMP_F77FLAGS
#   output variable to the flag (e.g. -omp) used both to compile *and* link
#   OpenMP programs in the current language.
#
#   NOTE: You are assumed to not only compile your program with these flags,
#   but also link it with them as well.
#
#   If you want to compile everything with OpenMP, you should set:
#
#       CFLAGS="$CFLAGS $OPENMP_CFLAGS"
#       #OR#  CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS"
#       #OR#  FFLAGS="$FFLAGS $OPENMP_FFLAGS"
#
#   (depending on the selected language).
#
#   The user can override the default choice by setting the corresponding
#   environment variable (e.g. OPENMP_CFLAGS).
#
#   ACTION-IF-FOUND is a list of shell commands to run if an OpenMP flag is
#   found, and ACTION-IF-NOT-FOUND is a list of commands to run it if it is
#   not found. If ACTION-IF-FOUND is not specified, the default action will
#   define HAVE_OPENMP.
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Steven G. Johnson <stevenj@alum.mit.edu>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Macro Archive. When you make and
#   distribute a modified version of the Autoconf Macro, you may extend this
#   special exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_OPENMP], [
AC_PREREQ(2.59) dnl for _AC_LANG_PREFIX

AC_CACHE_CHECK([for OpenMP flag of _AC_LANG compiler], ax_cv_[]_AC_LANG_ABBREV[]_openmp, [save[]_AC_LANG_PREFIX[]FLAGS=$[]_AC_LANG_PREFIX[]FLAGS
ax_cv_[]_AC_LANG_ABBREV[]_openmp=unknown
# Flags to try:  -fopenmp (gcc), -openmp (icc), -mp (SGI & PGI),
#                -xopenmp (Sun), -omp (Tru64), -qsmp=omp (AIX), none
ax_openmp_flags="-fopenmp -openmp -mp -xopenmp -omp -qsmp=omp none"
if test "x$OPENMP_[]_AC_LANG_PREFIX[]FLAGS" != x; then
  ax_openmp_flags="$OPENMP_[]_AC_LANG_PREFIX[]FLAGS $ax_openmp_flags"
fi
for ax_openmp_flag in $ax_openmp_flags; do
  case $ax_openmp_flag in
    none) []_AC_LANG_PREFIX[]FLAGS=$save[]_AC_LANG_PREFIX[] ;;
    *) []_AC_LANG_PREFIX[]FLAGS="$save[]_AC_LANG_PREFIX[]FLAGS $ax_openmp_flag" ;;
  esac
  AC_TRY_LINK_FUNC(omp_set_num_threads,
	[ax_cv_[]_AC_LANG_ABBREV[]_openmp=$ax_openmp_flag; break])
done
[]_AC_LANG_PREFIX[]FLAGS=$save[]_AC_LANG_PREFIX[]FLAGS
])
if test "x$ax_cv_[]_AC_LANG_ABBREV[]_openmp" = "xunknown"; then
  m4_default([$2],:)
else
  if test "x$ax_cv_[]_AC_LANG_ABBREV[]_openmp" != "xnone"; then
    OPENMP_[]_AC_LANG_PREFIX[]FLAGS=$ax_cv_[]_AC_LANG_ABBREV[]_openmp
  fi
  m4_default([$1], [AC_DEFINE(HAVE_OPENMP,1,[Define if OpenMP is enabled])])
fi
])dnl AX_OPENMP
