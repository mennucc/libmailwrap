// vim:ts=4:shiftwidth=4:et
/*
   tester program for email sender
   
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


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>  // getpid(2)
#include <limits.h>  // PATH_MAX


#define LMW_log_error( msg, ...) \
    fprintf(stderr, msg, ##__VA_ARGS__)

#define LMW_DEBUG 1

#include "LMW_send_email.c"

int main(int argc , char *argv[])
{

  if(argc<3) {
    fprintf(stderr,"Usage:  %s RECIPIENT SUBJECT [BODY]\n",argv[0]);
    return(0);
  }

  LMW_config *cfg = malloc(sizeof(LMW_config));
  LWM_config_init(cfg);
  char *b;
  if(argc==3) {
    const int L=1000000;
    b = calloc(1,L+1);
    for (unsigned int i=0 ; i<L-1 ; i++ ) {
      if ( 0 == ( i & 15))
	b[i]='\n';
      else
	b[i]='Y';
    }
  } else {
    b = argv[3];
  }
  
  int r =LMW_send_email(argv[1], argv[2], b ,cfg) ;
  fprintf(stdout,"return code [ %d ] \n\n",r);
  return r;
}
