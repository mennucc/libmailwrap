// vim:ts=4:shiftwidth=4:et
/*
   tester program for email sender

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

#include "LMW_send_email.h"

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
  
  int r;
  r =LMW_send_email_argc(&cfg, recipient, subject, body,
			 2,"-A", attach);
  fprintf(stdout,"return code  %d  ,failures %d \n\n", r, cfg.failures);
  
  return r;
}
