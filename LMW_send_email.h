/*
 * Copyright (C) 2025  Andrea C G Mennucci 
 *
 * This file is part of libmailwrap.
 *
 * libmailwrap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * libmailwrap is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with <Project Name>; if not, see
 * <https://www.gnu.org/licenses/>.
 */


#ifndef __LMW_SEND_EMAIL_H__
#define  __LMW_SEND_EMAIL_H__

#include <errno.h>          // <-- This provides ENOEXEC

// Error code definitions, as returned by LMW_send_email()
#define LMW_OK                    0   // All ok
#define LMW_ERROR_CANNOT_CALL    -1   // Could not invoke /bin/mail (fork/exec failure)
#define LMW_ERROR_PIPE           -2   // PIPE ERROR when sending body
#define LMW_ERROR_TIMEOUT        -3   // Waiting timeout, child did not finish
#define LMW_ERROR_SIGNAL         -4   // Child process was terminated by signal
// Positive values (>0) are error codes from /bin/mail
#define LMW_CHILD_EXEC_FAILED    ENOEXEC   // Standard exit code for "cannot exec"

// defaults
#define LMW_MAILER "/bin/mail"
#define LMW_MAX_WAIT 900 // in milliseconds

// maximum length of extra string arguments for LMW_send_email_argc()
#define LMW_SEND_EMAIL_MAX_LEN_ARGS 512

typedef struct {
  char *mailer;
  int max_wait;  // in milliseconds
  int failures; // keeps count of successive failures
  void (*log_error)(const char *msg, ...); // function pointer for logging errors
} LMW_config;

/* initialize pre-allocated config */
void LMW_config_init(LMW_config *cfg);

/***
   LMW_send_email()
    
   This code will send an email to recipient, with subject, and body;
   it will use "/bin/mail" or other compatible program specified in cfg->mailer
   that must follow the calling convention
   "mailer -s subject recipient"
   and that accepts the body as stdin input.

   (Both the implementations in GNU mailutils and BSD work fine).

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

int LMW_send_email(LMW_config *cfg, char *recipient, char *subject, char *body);


/**
   LMW_send_email_argc() is as LMW_send_email() ,

   but will accept `argc` extra arguments that will be passed to `/bin/mail`;
   each must be a `char *` , null terminated, of length at most LMW_SEND_EMAIL_MAX_LEN_ARGS

*/

int LMW_send_email_argc(LMW_config *cfg, char *recipient, char *subject, char *body, int argc, ...);


/**
   LMW_send_email_argv() is  as LMW_send_email() ,

   but will accept `argc` extra arguments, that are the strings in `argv`
   that will be passed to `/bin/mail`

*/
int LMW_send_email_argv(LMW_config *cfg, char *recipient, char *subject, char *body, int argc, char *argv[]);

#endif // __LMW_SEND_EMAIL_H__
