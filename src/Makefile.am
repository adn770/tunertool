# plugindir is set in configure

# change libgstplugin.la to something more suitable
noinst_LTLIBRARIES = libgstpitch.la libgsttonesrc.la

# for the next set of variables, rename the prefix if you renamed the .la

# sources used to compile this plug-in
libgstpitch_la_SOURCES = gstpitch.c kiss_fft.c
# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstpitch_la_CFLAGS = $(GSTPB_BASE_CFLAGS) $(GST_BASE_CFLAGS) \
	$(GSTCTRL_CFLAGS) $(GST_CFLAGS) -DFIXED_POINT=16
libgstpitch_la_LIBADD = $(GSTPB_BASE_LIBS) $(GST_BASE_LIBS) \
	$(GSTCTRL_LIBS) $(GST_LIBS)
libgstpitch_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

libgsttonesrc_la_SOURCES = gsttonesrc.c
libgsttonesrc_la_CFLAGS = $(GSTPB_BASE_CFLAGS) $(GST_BASE_CFLAGS) \
	$(GSTCTRL_CFLAGS) $(GST_CFLAGS) -DFIXED_POINT=16
libgsttonesrc_la_LIBADD = $(GSTPB_BASE_LIBS) $(GST_BASE_LIBS) \
	$(GSTCTRL_LIBS) $(GST_LIBS)
libgsttonesrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstpitch.h gsttonesrc.h kiss_fft.h _kiss_fft_guts.h settings.h
EXTRA_DIST = tuner26.png  tuner40.png  tuner64.png

if HAVE_GTK
noinst_PROGRAMS = demo-audiotest demo-tuner tuner
endif

demo_audiotest_SOURCES = demo-audiotest.c
demo_audiotest_CFLAGS  = $(GST_CFLAGS) $(GTK_CFLAGS) 
demo_audiotest_LDFLAGS = $(GST_LIBS) $(GTK_LIBS)

demo_tuner_SOURCES = demo-tuner.c
demo_tuner_CFLAGS  = $(GST_CFLAGS) $(GTK_CFLAGS) 
demo_tuner_LDFLAGS = $(GST_LIBS) $(GTK_LIBS)

tuner_SOURCES = tuner.c settings.c
tuner_DEPENDENCIES = $(top_builddir)/src/libgstpitch.la $(top_builddir)/src/libgsttonesrc.la
tuner_CFLAGS  = $(GST_CFLAGS) $(GTK_CFLAGS) $(MAEMO_CFLAGS) $(GCONF_CFLAGS) -D_GNU_SOURCE
tuner_LDADD = $(GST_LIBS) $(GTK_LIBS) $(MAEMO_LIBS) $(GCONF_LIBS) $(top_builddir)/src/libgstpitch.la $(top_builddir)/src/libgsttonesrc.la


