EXTRA_DIST = LICENSE.GPL LICENSE.POLICY LICENSE.NON-FREE ChangeLog.pre5.0

pkgconfig_DATA = acovea.pc

benchmarksdir = $(pkgdatadir)/benchmarks
dist_benchmarks_DATA = \
	benchmarks/almabench.c \
	benchmarks/distbench.c \
	benchmarks/evobench.c \
	benchmarks/fftbench.c \
	benchmarks/huffbench.c \
	benchmarks/linbench.c \
	benchmarks/linsmall.c \
	benchmarks/mat1bench.c \
	benchmarks/treebench.c

configdir = $(pkgdatadir)/config
dist_config_DATA = \
	config/g++34_opteron.acovea \
	config/g++34_pentium3.acovea \
	config/g++34_pentium4.acovea \
	config/g++40_opteron.acovea \
	config/g++40_pentium4.acovea \
	config/g9540_opteron.acovea \
	config/g9540_pentium3.acovea \
	config/g9540_pentium4.acovea \
	config/gcc34_intel.acovea \
	config/gcc34_opteron.acovea \
	config/gcc34_opteron_makefile.acovea \
	config/gcc34_opteron_size.acovea \
	config/gcc34_pentium3.acovea \
	config/gcc34_pentium4.acovea \
	config/gcc40_opteron.acovea \
	config/gcc40_opteron_makefile.acovea \
	config/gcc40_opteron_nofm.acovea \
	config/gcc40_opteron_size.acovea \
	config/gcc40_pentium4.acovea \
	config/gcc41_opteron_nofm.acovea \
	config/Makefile.example

###########
# cmdline #
###########
AM_CPPFLAGS = \
	-DACOVEA_VERSION=\"$(VERSION)\" \
	-DACOVEA_CONFIG_DIR=\""$(pkgdatadir)/config/"\" \
	-DACOVEA_BENCHMARK_DIR=\""$(pkgdatadir)/benchmarks/"\"

bin_PROGRAMS = cmdline/runacovea

cmdline_runacovea_SOURCES = cmdline/runacovea.cpp
cmdline_runacovea_LDADD = libacovea/libacovea.la $(LIBCOYOTL_LIBS)

#############
# libacovea #
#############

pkginclude_HEADERS = libacovea/acovea.h

lib_LTLIBRARIES = libacovea/libacovea.la

libacovea_libacovea_la_SOURCES = $(pkginclude_HEADERS) libacovea/acovea.cpp
libacovea_libacovea_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)
libacovea_libacovea_la_LIBADD = $(EXPAT_LIBS) $(LIBEVOCOSM_LIBS)
