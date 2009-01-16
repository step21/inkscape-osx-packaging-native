/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Use binreloc thread support? */
#define BR_PTHREADS 0

/* Use AutoPackage? */
/* #undef ENABLE_BINRELOC */

/* Use LittleCms color management */
#define ENABLE_LCMS 1

/* always defined to indicate that i18n is enabled */
#define ENABLE_NLS 1

/* Build with OSX .app data dir paths? */
/* #undef ENABLE_OSX_APP_LOCATIONS */

/* SVG Fonts should be used */
#define ENABLE_SVG_FONTS 1

/* Translation domain used */
#define GETTEXT_PACKAGE "inkscape"

/* Define to 1 if you have the `bind_textdomain_codeset' function. */
#define HAVE_BIND_TEXTDOMAIN_CODESET 1

/* Define to 1 if you have the <boost/concept_check.hpp> header file. */
#define HAVE_BOOST_CONCEPT_CHECK_HPP 1

/* Whether the Cairo PDF backend is available */
#define HAVE_CAIRO_PDF 1

/* define to 1 if Carbon is available */
#define HAVE_CARBON 1

/* Define to 1 if you have the `dcgettext' function. */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the `ecvt' function. */
#define HAVE_ECVT 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `fpsetmask' function. */
/* #undef HAVE_FPSETMASK */

/* Define to 1 if you have the <gc/gc.h> header file. */
/* #undef HAVE_GC_GC_H */

/* Define to 1 if you have the <gc.h> header file. */
#define HAVE_GC_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `gtk_window_fullscreen' function. */
#define HAVE_GTK_WINDOW_FULLSCREEN 1

/* Define to 1 if you have the `gtk_window_set_default_icon_from_file'
   function. */
#define HAVE_GTK_WINDOW_SET_DEFAULT_ICON_FROM_FILE 1

/* Define to 1 if you have the <ieeefp.h> header file. */
/* #undef HAVE_IEEEFP_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if your <locale.h> file defines LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if you have the <libintl.h> header file. */
#define HAVE_LIBINTL_H 1

/* Define to 1 if you have the `m' library (-lm). */
/* #undef HAVE_LIBM */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the `mallinfo' function. */
/* #undef HAVE_MALLINFO */

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mkdir' function. */
#define HAVE_MKDIR 1

/* Use libpoppler for direct PDF import */
#define HAVE_POPPLER 1

/* Use libpoppler-cairo for rendering PDF preview */
#define HAVE_POPPLER_CAIRO 1

/* Use libpoppler-glib and Cairo-SVG for PDF import */
#define HAVE_POPPLER_GLIB 1

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strspn' function. */
#define HAVE_STRSPN 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if `fordblks' is member of `struct mallinfo'. */
/* #undef HAVE_STRUCT_MALLINFO_FORDBLKS */

/* Define to 1 if `fsmblks' is member of `struct mallinfo'. */
/* #undef HAVE_STRUCT_MALLINFO_FSMBLKS */

/* Define to 1 if `hblkhd' is member of `struct mallinfo'. */
/* #undef HAVE_STRUCT_MALLINFO_HBLKHD */

/* Define to 1 if `uordblks' is member of `struct mallinfo'. */
/* #undef HAVE_STRUCT_MALLINFO_UORDBLKS */

/* Define to 1 if `usmblks' is member of `struct mallinfo'. */
/* #undef HAVE_STRUCT_MALLINFO_USMBLKS */

/* Define to 1 if you have the <sys/filio.h> header file. */
#define HAVE_SYS_FILIO_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Base data directory -- only path-prefix.h should use it! */
#define INKSCAPE_DATADIR "/Users/florian/Documents/inkscape/packaging/macosx/../../Build/share"

/* Base library directory -- only path-prefix.h should use it! */
#define INKSCAPE_LIBDIR "/Users/florian/Documents/inkscape/packaging/macosx/../../Build/lib"

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
/* #undef LSTAT_FOLLOWS_SLASHED_SYMLINK */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "inkscape"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Localization directory */
#define PACKAGE_LOCALE_DIR "/Users/florian/Documents/inkscape/packaging/macosx/../../Build/share/locale"

/* Define to the full name of this package. */
#define PACKAGE_NAME "inkscape"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "inkscape 0.46+devel"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "inkscape"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.46+devel"

/* Poppler version of GfxFont to use (0.8.3 or higher versions of Poppler) */
#define POPPLER_NEW_GFXFONT 1

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Version number of package */
#define VERSION "0.46+devel"

/* Use gnome vfs file load functionality */
#define WITH_GNOME_VFS 1

/* enable gtk spelling widget */
/* #undef WITH_GTKSPELL */

/* Image Magick++ support for bitmap effects */
/* #undef WITH_IMAGE_MAGICK */

/* Build in Inkboard support */
/* #undef WITH_INKBOARD */

/* Build in SSL support for Inkboard */
/* #undef WITH_INKBOARD_SSL */

/* enable openoffice files (SVG jars) */
#define WITH_INKJAR 1

/* Build in libwpg */
/* #undef WITH_LIBWPG */

/* Use MMX optimizations, if CPU supports it */
/* #undef WITH_MMX */

/* Use experimental module support */
#define WITH_MODULES 1

/* use Perl for embedded scripting */
/* #undef WITH_PERL */

/* use Python for embedded scripting */
/* #undef WITH_PYTHON */

/* Use Xft font database */
#define WITH_XFT 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */





