include $(top_srcdir)/common/check.mak

CHECK_REGISTRY = $(top_builddir)/tests/check/test-registry.reg
TEST_FILES_DIRECTORY = $(top_srcdir)/tests/files

GST_TOOLS_DIR = $(top_builddir)/tools

REGISTRY_ENVIRONMENT = \
	GST_REGISTRY=$(CHECK_REGISTRY)

# GST_PLUGINS_XYZ_DIR is only set in an uninstalled setup
TESTS_ENVIRONMENT = \
	$(REGISTRY_ENVIRONMENT)                                 \
	GST_PLUGIN_SYSTEM_PATH=					\
	GST_PLUGIN_PATH=$(top_builddir)/gst:$(top_builddir)/sys:$(top_builddir)/ext:$(GST_PLUGINS_FFMPEG_DIR):$(GST_PLUGINS_UGLY_DIR):$(GST_PLUGINS_GOOD_DIR):$(GST_PLUGINS_BASE_DIR):$(GST_PLUGINS_DIR) \
	GST_PLUGIN_LOADING_WHITELIST="gstreamer@$(GST_PLUGINS_DIR):gst-plugins-base@$(GSTPB_PLUGINS_DIR):gst-plugins-good:gst-plugins-ugly:gst-ffmpeg:gst-plugins-bad@$(top_builddir)" \
	GST_STATE_IGNORE_ELEMENTS="apexsink camerabin camerabin2 cdaudio dc1394src \
	    dccpclientsrc dccpclientsink dccpserversrc dccpserversink decklinksrc \
	    decklinksink dvbsrc dvbbasebin dfbvideosink festival gsettingsvideosrc \
	    gsettingsvideosink gsettingsaudiosrc gsettingsaudiosink linsyssdisrc linsyssdisink nassink \
	    rsndvdbin sdlaudiosink sdlvideosink vcdsrc rfbsrc vdpauyuvvideo vdpauvideoyuv \
	    vdpaumpegdec vdpaumpeg4dec vdpauh264dec vdpauvideopostprocess vdpausink neonhttpsrc"

plugindir = $(libdir)/gstreamer-@GST_MAJORMINOR@

# override to _not_ install the test plugins
install-pluginLTLIBRARIES:

# the core dumps of some machines have PIDs appended
CLEANFILES = core.* test-registry.*

SUPPRESSIONS = $(top_srcdir)/common/gst.supp $(srcdir)/gst-plugins-bad.supp

clean-local: clean-local-check clean-local-orc

if USE_ASSRENDER
check_assrender = elements/assrender
else
check_assrender =
endif

if USE_FAAC
check_faac = elements/faac
else
check_faac =
endif

if USE_FAAD
check_faad = elements/faad
else
check_faad =
endif

if USE_VOAACENC
check_voaacenc = elements/voaacenc
else
check_voaacenc =
endif

if USE_VOAMRWBENC
check_voamrwbenc = elements/voamrwbenc
else
check_voamrwbenc =
endif

if USE_EXIF
check_jifmux = elements/jifmux
else
check_jifmux =
endif

if USE_COG
check_logoinsert = elements/logoinsert
else
check_logoinsert =
endif

if USE_MPEG2ENC
check_mpeg2enc = elements/mpeg2enc
else
check_mpeg2enc =
endif

if USE_MPLEX
check_mplex = elements/mplex
else
check_mplex =
endif

if USE_NEON
check_neon = elements/neonhttpsrc
else
check_neon =
endif

if USE_OFA
check_ofa = elements/ofa
else
check_ofa =
endif

if USE_SCHRO
check_schro=elements/schroenc
else
check_schro=
endif

if USE_TIMIDITY
check_timidity=elements/timidity
else
check_timidity=
endif

if USE_KATE
check_kate=elements/kate
else
check_kate=
endif

if USE_MIMIC
check_mimic=pipelines/mimic
else
check_mimic=
endif

if USE_VP8
check_vp8=elements/vp8enc elements/vp8dec
else
check_vp8=
endif

if HAVE_ORC
check_orc = orc/cog
else
check_orc =
endif

if USE_ZBAR
check_zbar = elements/zbar
else
check_zbar =
endif

if USE_OPUS
check_opus = elements/opus
else
check_opus =
endif

VALGRIND_TO_FIX = \
	elements/mpeg2enc \
	elements/mplex    \
	elements/zbar     \
	pipeline/colorspace

# valgrind testing
VALGRIND_TESTS_DISABLE = \
	$(VALGRIND_TO_FIX)

# these tests don't even pass
# neon: too flaky (almost always fails 'the first time')
# colorspace: bad memory accesses in orc code for odd width buffers - https://bugzilla.gnome.org/show_bug.cgi?id=663248
noinst_PROGRAMS = \
	pipelines/colorspace \
	$(check_neon)

check_PROGRAMS = \
	generic/states \
	$(check_assrender) \
	$(check_faac)  \
	$(check_faad)  \
	$(check_voaacenc) \
	$(check_voamrwbenc) \
	$(check_mpeg2enc)  \
	$(check_mplex)     \
	$(check_ofa)        \
	$(check_timidity)  \
	$(check_kate)  \
	$(check_opus)  \
	elements/autoconvert \
	elements/autovideoconvert \
	elements/asfmux \
	elements/baseaudiovisualizer \
	elements/camerabin \
        elements/camerabin2 \
	elements/dataurisrc \
	elements/legacyresample \
        $(check_jifmux) \
	elements/jpegparse \
	$(check_logoinsert) \
	elements/h263parse \
	elements/h264parse \
	elements/mpegtsmux \
	elements/mpegvideoparse \
	elements/mpeg4videoparse \
	elements/multifdsink \
	elements/mxfdemux \
	elements/mxfmux \
	elements/id3mux \
	pipelines/mxf \
	$(check_mimic) \
	elements/rtpmux \
	libs/mpegvideoparser \
	libs/h264parser \
	libs/vc1parser \
	$(check_schro) \
	$(check_vp8) \
        elements/viewfinderbin \
	$(check_zbar) \
	$(check_orc) \
	$(EXPERIMENTAL_CHECKS)

noinst_HEADERS = elements/mxfdemux.h

TESTS = $(check_PROGRAMS)

AM_CFLAGS = $(GST_CFLAGS) $(GST_CHECK_CFLAGS) $(GST_OPTION_CFLAGS) \
		-DGST_TEST_FILES_PATH="\"$(TEST_FILES_DIRECTORY)\"" \
		-UG_DISABLE_ASSERT -UG_DISABLE_CAST_CHECKS
LDADD = $(GST_CHECK_LIBS)

# parser unit test convenience lib
noinst_LTLIBRARIES = libparser.la
libparser_la_SOURCES = elements/parser.c elements/parser.h
libparser_la_CFLAGS = \
	-I$(top_srcdir)/tests/check \
	$(GST_CFLAGS) $(GST_CHECK_CFLAGS) $(GST_OPTION_CFLAGS)

elements_mpegvideoparse_LDADD = libparser.la $(LDADD)

elements_mpeg4videoparse_LDADD = libparser.la $(LDADD)

elements_h263parse_LDADD = libparser.la $(LDADD)

elements_h264parse_LDADD = libparser.la $(LDADD)

libs_mpegvideoparser_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) \
	-DGST_USE_UNSTABLE_API \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)

libs_mpegvideoparser_LDADD = \
	$(top_builddir)/gst-libs/gst/codecparsers/libgstcodecparsers-@GST_MAJORMINOR@.la \
	$(GST_PLUGINS_BAD_LIBS) -lgstcodecparsers-@GST_MAJORMINOR@ \
	$(GST_BASE_LIBS) $(GST_LIBS) $(LDADD)

libs_h264parser_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) \
	-DGST_USE_UNSTABLE_API \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)

libs_h264parser_LDADD = \
	$(top_builddir)/gst-libs/gst/codecparsers/libgstcodecparsers-@GST_MAJORMINOR@.la \
	$(GST_PLUGINS_BAD_LIBS) -lgstcodecparsers-@GST_MAJORMINOR@ \
	$(GST_BASE_LIBS) $(GST_LIBS) $(LDADD)

libs_vc1parser_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) \
	-DGST_USE_UNSTABLE_API \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)

libs_vc1parser_LDADD = \
	$(top_builddir)/gst-libs/gst/codecparsers/libgstcodecparsers-@GST_MAJORMINOR@.la \
	$(GST_PLUGINS_BAD_LIBS) -lgstcodecparsers-@GST_MAJORMINOR@ \
	$(GST_BASE_LIBS) $(GST_LIBS) $(LDADD)

elements_faad_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)
elements_faad_LDADD = \
	$(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) $(GST_LIBS) $(LDADD) \
	-lgstaudio-@GST_MAJORMINOR@

elements_faac_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)
elements_faac_LDADD = \
	$(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) $(GST_LIBS) $(LDADD) \
	-lgstaudio-@GST_MAJORMINOR@

elements_legacyresample_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)
elements_legacyresample_LDADD = \
	$(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) $(GST_LIBS) $(LDADD) \
	-lgstaudio-@GST_MAJORMINOR@

elements_voaacenc_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)
elements_voaacenc_LDADD = \
	$(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) $(GST_LIBS) $(LDADD) \
	-lgstaudio-@GST_MAJORMINOR@

elements_baseaudiovisualizer_SOURCES = elements/baseaudiovisualizer.c \
	$(top_srcdir)/gst/audiovisualizers/gstbaseaudiovisualizer.c \
	$(top_srcdir)/gst/audiovisualizers/gstbaseaudiovisualizer.h
elements_baseaudiovisualizer_CFLAGS = $(GST_PLUGINS_BAD_CFLAGS) \
	-I$(top_srcdir)/gst/audiovisualizers $(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CONTROLLER_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS)
elements_baseaudiovisualizer_LDADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgstaudio-@GST_MAJORMINOR@  \
	-lgstvideo-@GST_MAJORMINOR@ 	$(GST_BASE_LIBS) $(GST_CONTROLLER_LIBS) \
	$(GST_LIBS) $(LDADD)

elements_camerabin_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS) -DGST_USE_UNSTABLE_API
elements_camerabin_LDADD = \
	$(top_builddir)/gst-libs/gst/interfaces/libgstphotography-@GST_MAJORMINOR@.la \
	$(GST_PLUGINS_BASE_LIBS) -lgstinterfaces-@GST_MAJORMINOR@ \
	$(GST_BASE_LIBS) $(GST_LIBS) $(LDADD)

elements_camerabin_SOURCES = elements/camerabin.c

elements_camerabin2_CFLAGS = \
	$(GST_PLUGINS_BAD_CFLAGS) $(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS) $(AM_CFLAGS) -DGST_USE_UNSTABLE_API
elements_camerabin2_LDADD = \
        $(top_builddir)/gst-libs/gst/interfaces/libgstphotography-@GST_MAJORMINOR@.la \
        $(top_builddir)/gst-libs/gst/basecamerabinsrc/libgstbasecamerabinsrc-@GST_MAJORMINOR@.la \
        -lgstpbutils-$(GST_MAJORMINOR) \
        $(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) $(GST_LIBS) $(LDADD)
elements_camerabin2_SOURCES = elements/camerabin2.c

elements_jifmux_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(EXIF_CFLAGS) $(AM_CFLAGS)
elements_jifmux_LDADD = $(GST_PLUGINS_BASE_LIBS) -lgsttag-$(GST_MAJORMINOR) $(GST_CHECK_LIBS) $(EXIF_LIBS) $(LDADD)
elements_jifmux_SOURCES = elements/jifmux.c

elements_timidity_CFLAGS = $(GST_BASE_CFLAGS) $(AM_CFLAGS)
elements_timidity_LDADD = $(GST_BASE_LIBS) $(LDADD)

elements_kate_CFLAGS = $(GST_BASE_CFLAGS) $(AM_CFLAGS)
elements_kate_LDADD = $(GST_BASE_LIBS) $(LDADD)

elements_rtpmux_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(AM_CFLAGS)
elements_rtpmux_LDADD = $(GST_PLUGINS_BASE_LIBS) -lgstrtp-$(GST_MAJORMINOR) $(GST_BASE_LIBS) $(LDADD)

elements_assrender_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(AM_CFLAGS)
elements_assrender_LDADD = $(GST_PLUGINS_BASE_LIBS) -lgstvideo-$(GST_MAJORMINOR) -lgstapp-$(GST_MAJORMINOR) $(GST_BASE_LIBS) $(LDADD)

elements_mpegtsmux_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(AM_CFLAGS)
elements_mpegtsmux_LDADD = $(GST_PLUGINS_BASE_LIBS) -lgstvideo-$(GST_MAJORMINOR) $(GST_BASE_LIBS) $(LDADD)


EXTRA_DIST = gst-plugins-bad.supp

orc_cog_CFLAGS = $(ORC_CFLAGS)
orc_cog_LDADD = $(ORC_LIBS) -lorc-test-0.4

orc/cog.c: $(top_srcdir)/ext/cog/gstcogorc.orc
	$(MKDIR_P) orc
	$(ORCC) --test -o $@ $<

clean-local-orc:
	rm -rf orc

