@SET_MAKE@

SUBDIRS = 								\
	panel-plugin							\
	pixmaps								\
	po

EXTRA_DIST =								\
	AUTHORS								\
	ChangeLog							\
	COPYING								\
	README

distclean-local:
	rm -rf *.cache *~ *.spec

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

