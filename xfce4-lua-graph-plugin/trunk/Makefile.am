@SET_MAKE@

SUBDIRS = panel-plugin

EXTRA_DIST =								\
	AUTHORS								\
	ChangeLog							\
	COPYING								\
	NEWS								\
	README								\
	scripts/*.lua

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
