SUBDIRS = libacovea cmdline

EXTRA_DIST = LICENSE.GPL LICENSE.POLICY LICENSE.NON-FREE ChangeLog.pre5.0 reconf
             
install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/config; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/config; \
	  for cfgfile in $(srcdir)/config/*; do \
	    if test -f $$cfgfile; then \
	      $(INSTALL_DATA) $$cfgfile $(DESTDIR)$(pkgdatadir)/config; \
	    fi \
	  done \
	fi

	if test -d $(srcdir)/benchmarks; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/benchmarks; \
	  for benchfile in $(srcdir)/benchmarks/*; do \
	    if test -f $$benchfile; then \
	      $(INSTALL_DATA) $$benchfile $(DESTDIR)$(pkgdatadir)/benchmarks; \
	    fi \
	  done \
	fi

uninstall-local:
	if test -d $(DESTDIR)$(pkgdatadir); then \
	  rm -rf $(DESTDIR)$(pkgdatadir); \
	fi

dist-hook:
	if test -d config; then \
	  mkdir $(distdir)/config; \
	  for cfgfile in config/*; do \
	    if test -f $$cfgfile; then \
	      cp -p $$cfgfile $(distdir)/config; \
	    fi \
	  done \
	fi

	if test -d benchmarks; then \
	  mkdir $(distdir)/benchmarks; \
	  for benchfile in benchmarks/*; do \
	    if test -f $$benchfile; then \
	      cp -p $$benchfile $(distdir)/benchmarks; \
	    fi \
	  done \
	fi

