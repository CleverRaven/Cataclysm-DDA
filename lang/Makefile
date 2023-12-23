PO_DIR := ./po
MO_DIR := ./mo

ifndef LANGUAGES
	PO := $(patsubst $(PO_DIR)/%,%,$(patsubst %.po,%,$(wildcard $(PO_DIR)/*.po)))
else
	ifeq ($(LANGUAGES), all)
		PO := $(patsubst $(PO_DIR)/%,%,$(patsubst %.po,%,$(wildcard $(PO_DIR)/*.po)))
	else
		PO := $(LANGUAGES)
	endif
endif

MO := $(patsubst %,$(MO_DIR)/%/LC_MESSAGES/cataclysm-dda.mo,$(PO))

all: $(MO)

$(MO_DIR)/%/LC_MESSAGES/cataclysm-dda.mo: $(PO_DIR)/%.po
	mkdir -p $$(dirname $@)
	msgfmt -f -o $@ $<

list_po:
	@echo $(PO)

list_mo:
	@echo $(MO)

install: $(MO)
ifdef LOCALE_DIR
	mkdir -p $(LOCALE_DIR)
	cp -R --no-preserve=ownership $(MO_DIR)/* $(LOCALE_DIR)
endif

clean:
	rm -rf $(MO_DIR)

.PHONY: all list_po list_mo clean
