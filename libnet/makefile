BASE = .

ifeq ($(wildcard port.mak),)
error:
	@echo You have not made a port.mak file!  See the installation instructions in
	@echo the readme.txt file for details.
endif

-include $(BASE)/port.mak

.PHONY: all lib install tests examples clean cleaner veryclean rebuild reinstall dist


-include testdirs.mak


all: lib tests examples


lib:
	$(MAKE) -C lib/ lib
install:
	$(MAKE) -C lib/ install

tests:
	$(MAKE) -C tests/
examples:
	$(MAKE) -C examples/

clean:
	$(MAKE) -C lib/ clean
	$(MAKE) -C tests/ clean
	$(MAKE) -C examples/ clean
cleaner:
	$(MAKE) -C lib/ cleaner
	$(MAKE) -C tests/ cleaner
	$(MAKE) -C examples/ cleaner
veryclean:
	$(MAKE) -C lib/ veryclean
	$(MAKE) -C tests/ veryclean
	$(MAKE) -C examples/ veryclean

distclean: cleaner
	$(MAKE) -C docs/
	-rm -f port.mak
	-rm -f make.bat
	-rm -f batfiles/make*.bat


rebuild: veryclean all
reinstall:
	$(MAKE) -C lib/ reinstall


# What follows is a bit crap.  It requires write permission in the parent
# directory and requires that this directory is called `libnet'.

ifneq ($(DJGPP),)

dist: distclean
	@echo "Creating ZIP distribution..."
	cd .. ; pkzip -ex -rP libnet.zip libnet/*.*

else

.PHONY: bats utod dtou dosdist unixdist

bats:
	cd batfiles ; . gen.sh

utod:
	@echo "Converting files to DOS format..."
	@find . -name CVS -prune -o -type f -! -name makefile\* -! -name \*.mak -exec utod {} \;

dtou:
	@echo "Converting files to Unix format..."
	@find . -name CVS -prune -o -type f -exec dtou {} \;

dosdist: distclean bats utod
	@echo -e "Creating ZIP distribution..."
	@(cd .. && find libnet -name CVS -prune -o -print | zip libnet.zip -@ )

unixdist: distclean dtou
	@echo -e "Creating tarball distribution..."
	@(cd .. && tar -cf libnet.tar libnet --exclude CVS && gzip -f libnet.tar )

dist: dosdist unixdist

endif
