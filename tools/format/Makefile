FORMATTER := ./format.pl
EXAMPLES := $(wildcard examples/*.json)

check: $(FORMATTER)
	@for file in $(EXAMPLES) ; do \
		$(FORMATTER) $$file | diff -q $$file - || exit 1; \
	done
