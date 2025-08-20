// vim:ts=4:shiftwidth=4:et
/*
   tester program for email sender
   
   will test different errounous code paths, and check
   the exit status
   
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
#include <string.h>

#include "LMW_send_email.h"

int main(int argc , char *argv[])
{
  if(argc>1 && (0==strcmp(argv[1],"-h"))) {
    fprintf(stderr,"Usage:  %s [BODY]\n"
	    "  if [BODY] is not specified, a long one will be used\n"
	    ,argv[0]);
    return(0);
  }

  char *recipient = "TEST";
  char *subject =  "the subject";
  
  LMW_config *cfg = malloc(sizeof(LMW_config));
  LWM_config_init(cfg);
  char *b=NULL;
  if(argc<=1) {
    const int L=2000000;
    b = calloc(1,L+1);
    for (unsigned int i=0 ; i<L-1 ; i++ ) {
      if ( 0 == ( i & 15))
	b[i]='\n';
      else
	b[i] = 32 + (i % 94);
    }
  } else {
    b = argv[1];
  }

  int r, ret=0;

#define CHECK(r,e) \
  { fprintf(stdout,"for %s, return code  %d , %s  \n\n", \
	    cfg->mailer, \
	    r, \
	    (r == e) ? "as expected": "AND THIS IS NOT correct"); \
    ret = (r==e) ? ret : 1 ;  }
  
  fprintf(stdout,"========== test  /bin/false\n");
  cfg->mailer = "/bin/false";
  r =LMW_send_email(cfg, recipient, subject, b);
  CHECK(r, 1);
  
  fprintf(stdout,"========== test  nonexistent\n");
  cfg->mailer = "/nonexistent";
  r =LMW_send_email(cfg, recipient, subject, b);
  CHECK(r,LMW_CHILD_EXEC_FAILED);
  
  
  fprintf(stdout,"========== test  ./sleep.sh (sleeps 10 seconds)\n");
  cfg->mailer = "./sleep.sh";
  r =LMW_send_email(cfg, recipient, subject, b);
  CHECK(r,LMW_ERROR_PIPE);
  
  fprintf(stdout,"======= test  ./cat_dev_null.sh  (cat body to /dev/null, then sleeps 10 seconds)\n");
  cfg->mailer = "./cat_dev_null.sh";
  r =LMW_send_email(cfg, recipient, subject, b);
  CHECK(r,LMW_ERROR_TIMEOUT);

  if(argc<=1)
    free(b);
  
  free(cfg);
  
  return ret;
}
