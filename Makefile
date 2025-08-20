CFLAGS += -Wall -fPIC

LIBNAME = libmailwrap
VERSION = 1.0
SONAME = $(LIBNAME).so.$(VERSION)

PREFIX ?= /usr/local
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

ALLBIN = LMW_send_email_test LMW_send_email_stresstest LMW_send_email_direct LMW_send_email_attach

all: $(SONAME) $(ALLBIN)

$(SONAME): LMW_send_email.o
	$(CC) -shared -o $(SONAME) LMW_send_email.o
	ln -sf $(SONAME) $(LIBNAME).so

LMW_send_email.o: LMW_send_email.c LMW_send_email.h
	$(CC) $(CFLAGS) -c LMW_send_email.c -o LMW_send_email.o

LMW_send_email_test: LMW_send_email_test.c LMW_send_email.c LMW_send_email.h
	$(CC) $(CFLAGS) LMW_send_email_test.c LMW_send_email.c -o LMW_send_email_test

LMW_send_email_stresstest: LMW_send_email_stresstest.c LMW_send_email.c LMW_send_email.h
	$(CC) $(CFLAGS) LMW_send_email_stresstest.c LMW_send_email.c -o LMW_send_email_stresstest

LMW_send_email_direct: LMW_send_email_direct.c LMW_send_email.c LMW_send_email.h
	$(CC) $(CFLAGS) LMW_send_email_direct.c -o LMW_send_email_direct

LMW_send_email_attach: LMW_send_email_attach.c LMW_send_email.h $(SONAME)
	$(CC) $(CFLAGS) LMW_send_email_attach.c  -L . -l mailwrap -o LMW_send_email_attach

install: $(SONAME)
	install -d $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(LIBDIR)
	install -m 644 LMW_send_email.h $(DESTDIR)$(INCLUDEDIR)/
	install -m 755 $(SONAME) $(DESTDIR)$(LIBDIR)/
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/$(LIBNAME).so

clean:
	rm -f *.o *.so* $(ALLBIN)

