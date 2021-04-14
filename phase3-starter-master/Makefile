
TOP_PHASE = phase3
SUBDIRS=$(wildcard $(TOP_PHASE)[a-d])

HDRS=$(TOP_PHASE).h $(TOP_PHASE)Int.h

.PHONY: $(SUBDIRS) all clean install subdirs

all: $(SUBDIRS)

subdirs: $(SUBDIRS)

clean: $(SUBDIRS)
	rm -f term*.out

install: $(SUBDIRS)
	install $(HDRS) ~/include

tests: $(SUBDIRS)

tar:
	(cd ..; gnutar cvzf ~/Downloads/$(TOP_PHASE)-starter.tgz --exclude=.git --exclude="*.dSYM" $(TOP_PHASE)-starter)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)