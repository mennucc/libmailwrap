CFLAGS += -Wall

all: LMW_send_email_test LMW_send_email_strestest LMW_send_email_direct

clean:
	rm LMW_send_email_test LMW_send_email_strestest LMW_send_email_direct

LMW_send_email_test: LMW_send_email_test.c LMW_send_email.c  LMW_send_email.h
	$(CC) $(CFLAGS)  LMW_send_email_test.c LMW_send_email.c -o LMW_send_email_test

LMW_send_email_strestest: LMW_send_email_stresstest.c LMW_send_email.c  LMW_send_email.h
	$(CC) $(CFLAGS)  LMW_send_email_stresstest.c LMW_send_email.c -o LMW_send_email_stresstest

LMW_send_email_direct: LMW_send_email_test.c LMW_send_email.c  LMW_send_email.h
	$(CC) $(CFLAGS)  LMW_send_email_direct.c -o LMW_send_email_direct
