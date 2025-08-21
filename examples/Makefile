ALLBIN = LMW_send_email_test LMW_send_email_stresstest LMW_send_email_direct LMW_send_email_attach

all: $(ALLBIN)

CFLAGS += -I..  -L..

### test various different ways to compile code that uses the library

## compile and linking at the same time
LMW_send_email_test: LMW_send_email_test.c ../LMW_send_email.c ../LMW_send_email.h
	$(CC) $(CFLAGS) LMW_send_email_test.c ../LMW_send_email.c -o LMW_send_email_test

LMW_send_email_stresstest: LMW_send_email_stresstest.c ../LMW_send_email.c ../LMW_send_email.h
	$(CC) $(CFLAGS) LMW_send_email_stresstest.c ../LMW_send_email.c -o LMW_send_email_stresstest

## including the LMW code inside our code
LMW_send_email_direct: LMW_send_email_direct.c ../LMW_send_email.c ../LMW_send_email.h
	$(CC) $(CFLAGS) LMW_send_email_direct.c -o LMW_send_email_direct

## linking to the .so
LMW_send_email_attach: LMW_send_email_attach.c ../LMW_send_email.h $(SONAME)
	$(CC) $(CFLAGS) LMW_send_email_attach.c  -l mailwrap -o LMW_send_email_attach

clean:
	rm -f *.o  $(ALLBIN)


