CFLAGS += -Wall -fPIC
LIBNAME = libmailwrap
VERSION = 1.0
SONAME = $(LIBNAME).so.$(VERSION)

PREFIX ?= /usr/local
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

ALLBIN = LMW_send_email_test LMW_send_email_stresstest LMW_send_email_direct LMW_send_email_attach

all: $(SONAME)
	make -C examples

$(SONAME): LMW_send_email.o
	$(CC) -shared -o $(SONAME) LMW_send_email.o
	ln -sf $(SONAME) $(LIBNAME).so

LMW_send_email.o: LMW_send_email.c LMW_send_email.h
	$(CC) $(CFLAGS) -c LMW_send_email.c -o LMW_send_email.o


install: $(SONAME)
	install -d $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(LIBDIR)
	install -m 644 LMW_send_email.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 755 $(SONAME) $(DESTDIR)$(LIBDIR)/
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/$(LIBNAME).so

clean:
	rm -f *.o *.so*
	make -C examples clean


