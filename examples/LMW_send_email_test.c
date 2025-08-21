// vim:ts=4:shiftwidth=4:et
/*
   tester program for email sender

   will send email
   
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
  if(argc<2) {
    fprintf(stderr,"Usage:  %s RECIPIENT [SUBJECT] [BODY]\n"
	    "  if [BODY] is not specified, a long one will be sent\n"
	    ,argv[0]);
    return(0);
  }

  char *recipient= argv[1],
    *subject = (argc>=3) ? argv[2] :  "the subject";
  
  LMW_config *cfg = malloc(sizeof(LMW_config));
  LMW_config_init(cfg);
  char *b=NULL;
  if(argc<=3) {
    const int L=2000000;
    b = calloc(1,L+1);
    for (unsigned int i=0 ; i<L-1 ; i++ ) {
      if ( 27 == ( i % 28))
	b[i]='\n';
      else {
	if ( 26 == ( i % 28))
	  b[i]='a' + ( ( (i-26) / 28  ) % 26);
	else
	  b[i]='A' + (i % 28);
      }
    }
  } else {
    b = argv[3];
  }

  int r;
  r =LMW_send_email(cfg, recipient, subject, b);
  fprintf(stdout,"return code  %d  ,failures %d \n\n", r, cfg->failures);

  if(argc<=3)
    free(b);
  free(cfg);
  
  return r;
}
