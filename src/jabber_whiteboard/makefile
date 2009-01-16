# Convenience stub makefile to call the real Makefile.



OBJEXT = o

# Explicit so that it's the default rule.
all:
	cd .. && $(MAKE) jabber_whiteboard/all

clean %.a %.$(OBJEXT):
	cd .. && $(MAKE) jabber_whiteboard/$@

.PHONY: all clean

.SUFFIXES:
.SUFFIXES: .a .$(OBJEXT)
