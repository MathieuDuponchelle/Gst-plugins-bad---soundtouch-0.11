plugin_LTLIBRARIES = libgstmpegdemux.la

libgstmpegdemux_la_SOURCES = \
	flumpegdemux.c \
	gstmpegdemux.c \
	gstpesfilter.c 
#	flutspatinfo.c 
#	flutspmtinfo.c 
#	flutspmtstreaminfo.c
#	gstmpegdesc.c \
#	gstmpegtsdemux.c \
#	gstsectionfilter.c \
#	mpegtsparse.c \
#	mpegtspacketizer.c

libgstmpegdemux_la_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS)
libgstmpegdemux_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgsttag-$(GST_MAJORMINOR) \
	$(GST_BASE_LIBS) $(GST_LIBS)
libgstmpegdemux_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstmpegdemux_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gstmpegdefs.h   \
	gstmpegdesc.h   \
	gstmpegdemux.h  \
	gstpesfilter.h  \
	gstmpegtsdemux.h \
	flutspatinfo.h \
	flutspmtinfo.h \
	flutspmtstreaminfo.h \
	gstsectionfilter.h \
	mpegtspacketizer.h \
	mpegtsparse.h

Android.mk: Makefile.am $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libgstmpegdemux -:SHARED libgstmpegdemux \
	 -:TAGS eng debug \
         -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(libgstmpegdemux_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(libgstmpegdemux_la_CFLAGS) \
	 -:LDFLAGS $(libgstmpegdemux_la_LDFLAGS) \
	           $(libgstmpegdemux_la_LIBADD) \
	           -ldl \
	 -:PASSTHROUGH LOCAL_ARM_MODE:=arm \
		       LOCAL_MODULE_PATH:='$$(TARGET_OUT)/lib/gstreamer-0.10' \
	> $@
