all-local: docs
clean-local: clean-docs

STATIC_MANPAGES = \
		apt.8
SGML_MANPAGES = \
		apt-get.8 \
		apt-cache.8 \
		apt-cdrom.8 \
		apt-config.8 \
		apt.conf.5 \
		sources.list.5 \
		vendors.list.5 \
		apt_preferences.5

EXTRA_DIST = \
		$(STATIC_MANPAGES) \
		$(patsubst %,%.sgml, \
		$(SGML_MANPAGES)) \
		apt.ent \
		style.txt \
		cache.sgml \
		files.sgml \
		guide.sgml \
		offline.sgml \
		design.sgml \
		guide.it.sgml \
		method.sgml \
		examples/apt.conf \
		examples/configure-index \
		examples/sources.list \
		fr/apt-cache.8.fr.sgml \
		fr/apt-cdrom.8.fr.sgml \
		fr/apt.conf.5.fr.sgml \
		fr/apt-config.8.fr.sgml \
		fr/apt.ent.fr \
		fr/apt_preferences.5.fr.sgml \
		fr/sources.list.5.fr.sgml

CLEANFILES = manpage.links manpage.refs

if COMPILE_MANPAGES
man_MANS = $(STATIC_MANPAGES) $(SGML_MANPAGES)

docs: $(man_MANS)

%: %.sgml apt.ent
	@echo Creating man page $@
	@$(DOCBOOK2MAN) $<
else
docs:
endif

clean-docs:
	rm -f $(SGML_MANPAGES)

