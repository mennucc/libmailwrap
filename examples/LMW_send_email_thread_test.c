// vim:ts=4:shiftwidth=4:et
/*
   tester program for email sender, in a separate thread

   will send email with attachment

   warning: this program requires /bin/mail.mailutils from GNU mailutils.\n"


  Copyright (c) by Andrea C G Mennucci
    
   LICENSE

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "LMW_send_email.h"
#include "LMW_send_email_in_thread.h"


int main(int argc , char *argv[])
{
  if(argc<5) {
    fprintf(stderr,"Usage:  %s RECIPIENT SUBJECT BODY ATTACH_FILE\n"
	    " Warning: this program requires /bin/mail.mailutils from GNU mailutils.\n"
	    ,argv[0]);
    return(0);
  }

  char *recipient= argv[1],
    *subject =  argv[2],
    *body    =  argv[3],
    *attach  =  argv[4];

  LMW_config cfg;
  LMW_config_init(&cfg);
  // sending attachments can take longer
  cfg.max_wait = 3000;
  // force using GNU mailutils
  cfg.mailer =   "/bin/mail.mailutils";
  
  
  // Start async email sending with extra arguments
  char *mail_argv[] = { "-A", attach, NULL};
  LMW_thread_context *ctx =
    LMW_send_email_argv_thread_start(&cfg, recipient, subject, body,
				      2, mail_argv);
  if (!ctx) {
    fprintf(stderr, "Failed to start async email sending\n");
    return 1;
  }
  
  // Do other work while email is being sent
  printf("Email being sent in background ...\n");
  // ... do other work ...

  usleep(20000);
  while ( 0  == LMW_send_email_thread_check(ctx) ) {
    printf("...email still being sent in background ...\n");
    usleep(100000);
  }

  int result = LMW_send_email_thread_wait(ctx);
  printf("Email send result: %d\n", result);

  return result;
}
