@SET_MAKE@

SUBDIRS = panel-plugin 

EXTRA_DIST = AUTHORS ChangeLog NEWS README THANKS TODO autogen.sh debian/rules debian/changelog debian/copyright debian/control autogen.sh configure.ac

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
