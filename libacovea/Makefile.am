INCLUDES = -I$(top_srcdir) \
	-DACOVEA_VERSION=\"$(VERSION)\" \
	-DACOVEA_CONFIG_DIR=\""$(pkgdatadir)/config/"\" \
	-DACOVEA_BENCHMARK_DIR=\""$(pkgdatadir)/benchmarks/"\"

h_sources = acovea.h

cpp_sources = acovea.cpp

lib_LTLIBRARIES = libacovea.la

libacovea_la_SOURCES = $(h_sources) $(cpp_sources)
libacovea_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

DEFS=-I. -I$(srcdir)
