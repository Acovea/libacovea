INCLUDES = -I$(top_srcdir) \
	-DACOVEA_VERSION=\"$(VERSION)\" \
	-DACOVEA_CONFIG_DIR=\""$(pkgdatadir)/config/"\" \
	-DACOVEA_BENCHMARK_DIR=\""$(pkgdatadir)/benchmarks/"\"
        
bin_PROGRAMS = runacovea

runacovea_SOURCES = runacovea.cpp

LIBS = -L../libacovea -lacovea -lcoyotl -levocosm -lexpat
