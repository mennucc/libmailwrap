
#ifndef __LWM_SEND_EMAIL_H__
#define  __LWM_SEND_EMAIL_H__


// defaults
#define LMW_MAILER "/bin/mail"
#define LMW_MAX_WAIT 900 // in milliseconds

typedef struct {
  char *mailer;
  int max_wait;  // in milliseconds
  int failures; // keeps count of successive failures
} LMW_config;

/* initialize pre-allocated config */
void LWM_config_init(LMW_config *cfg);

/***
   This code will send an email to recipient, with subject, and body;
   it will use "/bin/mail" or other compatible program specified in cfg->mailer
   that must follow the calling convention
   "mailer -s subject recipient"
   and that accepts the body as stdin input.

   It can be called with cfg=NULL (defaults will be used);
   
   It will wait for at most cfg->max_wait milliseconds
   (to avoid hanging the caller, if /bin/mail hangs).
   
   It will return an exit value: 
   LMW_OK (0)                = all ok 
   LMW_ERROR_CANNOT_CALL (-1) = could not invoke /bin/mail
   LMW_ERROR_PIPE (-2)        = PIPE ERROR when sending body
   LMW_ERROR_TIMEOUT (-3)     = waiting timeout, child did not finish in less than max_wait milliseconds
   LMW_ERROR_SIGNAL (-4)      = child process was terminated by signal
   >0                         = error code from /bin/mail
*/

int LMW_send_email(char *recipient, char *subject, char *body, LMW_config *cfg);


#endif // __LWM_SEND_EMAIL_H__
