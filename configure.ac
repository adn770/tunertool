AC_INIT

dnl versions of gstreamer and plugins-base
GST_MAJORMINOR=0.10
GST_REQUIRED=0.10.0
GSTPB_REQUIRED=0.10.0

dnl fill in your package name and version here
dnl the fourth (nano) number should be 0 for a release, 1 for CVS,
dnl and 2... for a prerelease

dnl when going to/from release please set the nano correctly !
AS_VERSION(tuner, TUNER_VERSION, 0, 1, 3, 0,
    RELEASE="yes", RELEASE="no")

dnl AM_MAINTAINER_MODE provides the option to enable maintainer mode
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

dnl make aclocal work in maintainer mode
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

AM_CONFIG_HEADER(config.h)

dnl check for tools
AC_PROG_CC
AC_PROG_LIBTOOL


dnl Enable Hildon GUI
AC_ARG_WITH(hildon,
    AC_HELP_STRING([--with-hildon],
        [Turn on/off use of Hildon GUI (default=no)]),
        [if test "x$withval" = "xyes"; then test_hildon=true; fi])

dnl Enable Maemo specific issues
AC_ARG_WITH(maemo,
    AC_HELP_STRING([--with-maemo],
        [Turn on/off use of Maemo related code (default=no)]),
        [if test "x$withval" = "xyes"; then enable_maemo=true; fi])


dnl decide on error flags
AS_COMPILER_FLAG(-Wall, GST_WALL="yes", GST_WALL="no")
                                                                                
if test "x$GST_WALL" = "xyes"; then
   GST_ERROR="$GST_ERROR -Wall"
                                                                                
   if test "x$GST_CVS" = "xyes"; then
     AS_COMPILER_FLAG(-Werror,GST_ERROR="$GST_ERROR -Werror",GST_ERROR="$GST_ERROR")
   fi
fi

dnl Check for pkgconfig first
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)

dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR(you need to have pkgconfig installed !)
fi

dnl Now we're ready to ask for gstreamer libs and cflags
dnl And we can also ask for the right version of gstreamer


PKG_CHECK_MODULES(GST, \
  gstreamer-$GST_MAJORMINOR >= $GST_REQUIRED,
  HAVE_GST=yes,HAVE_GST=no)

dnl Give error and exit if we don't have gstreamer
if test "x$HAVE_GST" = "xno"; then
  AC_MSG_ERROR(you need gstreamer development packages installed !)
fi


dnl Check for Maemo required packages

if test "x$enable_maemo" = "xtrue"; then
  test_hildon=true
  AC_DEFINE(MAEMO, 1, [Turn on Maemo dependecies])
fi

if test "x$test_hildon" = "xtrue"; then
  dnl try if hildon is available in the form previous IT2K8
  PKG_CHECK_MODULES(HILDON, hildon-libs >= 0.12.0,
    HAVE_HILDON=yes, HAVE_HILDON=no)

  if test "x$HAVE_HILDON" = "xno"; then
    dnl if fail try for IT2K8
    PKG_CHECK_MODULES(HILDON, hildon-1 >= 0.9.9,
      HAVE_HILDON=yes, HAVE_HILDON=no)
    AC_DEFINE(HILDON, 1, [Version of hildon libraries])
  else
    AC_DEFINE(HILDON, 0, [Version of hildon libraries])
  fi

  if test "x$HAVE_HILDON" = "xno"; then
    AC_MSG_ERROR(you need hildon development packages installed !)
  fi

  PKG_CHECK_MODULES(OSSO, libosso >= 0.9.19)

  MAEMO_CFLAGS="$HILDON_CFLAGS $OSSO_CFLAGS"
  MAEMO_LIBS="$HILDON_LIBS $OSSO_LIBS"
fi

dnl make MAEMO_CFLAGS and MAEMO_LIBS available
AC_SUBST(MAEMO_CFLAGS)
AC_SUBST(MAEMO_LIBS)

dnl append GST_ERROR cflags to GST_CFLAGS
GST_CFLAGS="$GST_CFLAGS $GST_ERROR"

dnl make GST_CFLAGS and GST_LIBS available
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

dnl make GST_MAJORMINOR available in Makefile.am
AC_SUBST(GST_MAJORMINOR)

dnl If we need them, we can also use the base class libraries
PKG_CHECK_MODULES(GST_BASE, gstreamer-base-$GST_MAJORMINOR >= $GST_REQUIRED,
                  HAVE_GST_BASE=yes, HAVE_GST_BASE=no)

dnl Give a warning if we don't have gstreamer libs
dnl you can turn this into an error if you need them
if test "x$HAVE_GST_BASE" = "xno"; then
  AC_MSG_NOTICE(no GStreamer base class libraries found (gstreamer-base-$GST_MAJORMINOR))
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GST_BASE_CFLAGS)
AC_SUBST(GST_BASE_LIBS)

dnl If we need them, we can also use the gstreamer-plugins-base libraries
PKG_CHECK_MODULES(GSTPB_BASE,
                  gstreamer-plugins-base-$GST_MAJORMINOR >= $GSTPB_REQUIRED,
                  HAVE_GSTPB_BASE=yes, HAVE_GSTPB_BASE=no)

dnl Give a warning if we don't have gstreamer libs
dnl you can turn this into an error if you need them
if test "x$HAVE_GSTPB_BASE" = "xno"; then
  AC_MSG_NOTICE(no GStreamer Plugins Base libraries found (gstreamer-plugins-base-$GST_MAJORMINOR))
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GSTPB_BASE_CFLAGS)
AC_SUBST(GSTPB_BASE_LIBS)

dnl If we need them, we can also use the gstreamer-controller libraries
PKG_CHECK_MODULES(GSTCTRL,
                  gstreamer-controller-$GST_MAJORMINOR >= $GSTPB_REQUIRED,
                  HAVE_GSTCTRL=yes, HAVE_GSTCTRL=no)

dnl Give a warning if we don't have gstreamer-controller
dnl you can turn this into an error if you need them
if test "x$HAVE_GSTCTRL" = "xno"; then
  AC_MSG_NOTICE(no GStreamer Controller libraries found (gstreamer-controller-$GST_MAJORMINOR))
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GSTCTRL_CFLAGS)
AC_SUBST(GSTCTRL_LIBS)

dnl GTK is optional and used in examples
HAVE_GTK=NO
PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= 2.2.0, HAVE_GTK_22=yes, HAVE_GTK_22=no)
if test "x$HAVE_GTK_22" = "xyes"; then
  HAVE_GTK=yes
  GTK_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
  AC_SUBST(GTK_VERSION)
  GTK_PREFIX=`$PKG_CONFIG --variable=prefix gdk-pixbuf-2.0`
  AC_SUBST(GTK_BASE_DIR)
  GDK_PIXBUF_LIBDIR=`$PKG_CONFIG --variable=libdir gdk-pixbuf-2.0`
  GDK_PIXBUF_PREFIXDIR=`$PKG_CONFIG --variable=prefix gdk-pixbuf-2.0`
  AC_SUBST(GTK_BASE_DIR)
else
  PKG_CHECK_MODULES(GTK2, gtk+-2.0, HAVE_GTK_20=yes, HAVE_GTK_20=no)
fi
if test "x$HAVE_GTK_20" = "xyes"; then
  HAVE_GTK=yes
fi
GTK_CFLAGS=$GTK2_CFLAGS
GTK_LIBS=$GTK2_LIBS
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(HAVE_GTK)
AM_CONDITIONAL(HAVE_GTK, test "x$HAVE_GTK" = "xyes")

dnl If we need them, we can also use the gstreamer-controller libraries
PKG_CHECK_MODULES(GCONF,
                  gconf-2.0,
                  HAVE_GCONF=yes, HAVE_GCONF=no)

dnl Give a warning if we don't have gstreamer-controller
dnl you can turn this into an error if you need them
if test "x$HAVE_GCONF" = "xno"; then
  AC_MSG_ERROR(no GConf libraries found)
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONFL_LIBS)

dnl set the plugindir where plugins should be installed
if test "x${prefix}" = "x$HOME"; then
  plugindir="$HOME/.gstreamer-$GST_MAJORMINOR/plugins"
else
  plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
fi
AC_SUBST(plugindir)

dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

AC_OUTPUT(Makefile m4/Makefile debian/Makefile src/Makefile)


