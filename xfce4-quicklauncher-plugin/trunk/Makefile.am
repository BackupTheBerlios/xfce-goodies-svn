@SET_MAKE@

SUBDIRS = po panel-plugin

distclean-local:
	rm -rf *.cache *~

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
	  bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
	  bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
	
EXTRA_DIST =	\
	ChangeLog \
	TODO	\
	config.rpath \
	mkinstalldirs
	
ACLOCAL_AMFLAGS = -I m4

