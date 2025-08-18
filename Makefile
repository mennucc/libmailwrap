CFLAGS += -Wall

LMW_send_email_test: LMW_send_email_test.c LMW_send_email.c  LMW_send_email.h
	$(CC) $(CFLAGS)  LMW_send_email_test.c LMW_send_email.c -o LMW_send_email_test
