# Makefile for generating docs in a variety of formats

# based upon DJGPP FAQ's makefile (by Eli Zaretskii)

# Although I've shuffled things around quite a lot, the
# basic effect is the same.  I split it into lots of small 
# rules, because then the intermediate files are treated
# as such.  I also made it easier to adapt for your own
# Texinfo files.

# Set STEM to the stem of your filename, and set TLA to a
# three letter acronym.  Don't use something silly like
# `txt' or `sed'!
include makefile.cfg

# If your main Texinfo file includes others, tag them on
# the end of SOURCES.  Note: I haven't tested this.
SOURCES = $(STEM).txi

# Supported formats: .info, .txt, .html, .dvi, .ps
FORMATS = .info .txt .html     # .dvi .ps

# Compilation setup for helper programs
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -s


# No need to touch anything below this point, unless either
# something doesn't work or you just want to fiddle around.

all: $(addprefix $(STEM), $(FORMATS))

ifdef COMSPEC
 CP = copy /y
 RM = del
else
 CP = cp -f
 RM = rm -f
endif

.PRECIOUS: numbered.% contents.% chapters.% textprep.% textpost.% \
           textint1.% textint2.%     text.% htmltemp.%     html.%


# We want to use the TLA where possible, so copy the 
# main source file
txi.$(TLA): $(STEM).txi
	$(CP) $< $@


######## # # # #  #  #  #   #   #    #     #     #      generic
# Generic things

numbered.$(TLA): txi.$(TLA) enum
	./enum $< $@


######## # # # #  #  #  #   #   #    #     #     #      .info
# For the Info target

# How to make the STEM files from the target TLA files
$(STEM).inf: $(TLA).inf
	$(CP) $< $@
$(STEM).info: $(TLA).info
	$(CP) $< $@

# How to make the target TLA files
$(TLA).inf: $(TLA).info
$(TLA).info: numbered.$(TLA) $(SOURCES)
	makeinfo --no-split --fill-column 64 -o $@ $<


######## # # # #  #  #  #   #   #    #     #     #      .html
# For the HTML target

# How to make the STEM files from the target TLA files
$(STEM).html: $(TLA).html
	$(CP) $< $@

# How to make the target TLA files
$(TLA).html: numbered.$(TLA) $(SOURCES)
	makeinfo --no-split --fill-column 64 --html -o $@ $<


######## # # # #  #  #  #   #   #    #     #     #      .txt
# For the plain text target

# How to make the STEM file from the target TLA file
$(STEM).txt: text.$(TLA)
	$(CP) $< $@

# Generic rules for making text output
contents.$(TLA): numbered.$(TLA) contents.sed
	sed -n -f contents.sed $< > $@
chapters.$(TLA): numbered.$(TLA) chapters.sed
	sed -n -f chapters.sed $< > $@
textprep.$(TLA): chapters.$(TLA) chapxref.sed
	sed -f chapxref.sed $< > $@
textpost.$(TLA): chapters.$(TLA) idxref.sed
	sed -f idxref.sed $< > $@
textint1.$(TLA): numbered.$(TLA) textprep.$(TLA)
	@echo 'The following command may take a long time.  Be patient.'
	sed -f textprep.$(TLA) $< > $@
textint2.$(TLA): textint1.$(TLA) contents.$(TLA) $(SOURCES)
	$(CP) contents.$(TLA) contents.idx
	makeinfo --no-split --no-headers --no-validate --fill-column 64 -Dtext -o $@ $<
	$(RM) contents.idx
text.$(TLA): textint2.$(TLA) textpost.$(TLA)
	sed -f textpost.$(TLA) $< > $@


######## # # # #  #  #  #   #   #    #     #     #      .dvi
# For DVI output

$(STEM).dvi: $(STEM).txi
	texi2dvi $<


######## # # # #  #  #  #   #   #    #     #     #      .ps
# For Postscript output

$(STEM).ps: $(STEM).dvi
	dvips $< -o $@


######## # # # #  #  #  #   #   #    #     #     #      minibook
# For output as a little booklet
#
# Print `...41.ps' first, then print `...23.ps' on the backs.

minibook: $(STEM).ps
	mpage -O -o -M -m -A $< > $(STEM)41.ps
	mpage -E -o -M -m -A $< > $(STEM)23.ps


clean:
ifdef COMSPEC
	command > nul /c for %f in (contents.idx *.$(TLA) enum enum.exe) do if exist %f del %f
	command > nul /c for %f in ($(addprefix $(STEM), .aux .log .toc .cp .fn .ky) do if exist %f del %f
	command > nul /c for %f in ($(addprefix $(STEM), .pg .tp .vr .cps .fns .vrs) do if exist %f del %f
else
	rm -f contents.idx *.$(TLA) enum enum.exe $(addprefix $(STEM), .aux .log .toc .cp .fn .ky .pg .tp .vr .cps .fns .vrs)
endif

distclean: clean

veryclean: distclean
ifdef COMSPEC
	command > nul /c for %f in (*.htm *.html *.inf *.ps *.dvi *.txt) do if exist %f del %f
else
	rm -f *.htm* *.inf* *.ps *.dvi *.txt
endif

dist: all distclean

love:
	@echo 'Not war!'

coffee:
	@echo 'Not yet.  Maybe in v3.x...'

