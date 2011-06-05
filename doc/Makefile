
DOCFILES = $(patsubst %.rst,%.html,$(wildcard *.rst))
CONVERTER = $(shell which rst2html 2>/dev/null || which rst2html.py)


all: $(DOCFILES) readme

%.html: %.rst
	$(CONVERTER) $< > $@

readme.html: readme
	$(CONVERTER) $< > $@
