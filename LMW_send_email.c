// vim:ts=4:shiftwidth=4:et
/*

   Code to send email.

   This code will call /bin/mail  (by forking)
   and it will send it the body, then wait for a while,
   and report the exit status.

   It is intended to be robust, to protect the calling program
   from crashes or hangouts.
  
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

   You should have received a copy of the GNU GPL
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/



#ifndef LWM_SKIP_HEADERS
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
#include <fcntl.h>
#endif  //LWM_SKIP_HEADERS

#include "LMW_send_email.h"

#ifndef LMW_log_error
#define LMW_log_error( msg, ...) \
    fprintf(stderr, msg, ##__VA_ARGS__)
#endif

void LWM_config_init(LMW_config *cfg)
{
  *cfg = (LMW_config) {
    .mailer = LMW_MAILER,
    .max_wait = LMW_MAX_WAIT,
    .failures = 0,
  };
};

/* Function to make pipe non-blocking */
static int __LMW__make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int __LMW__process_exit_status__(int status, LMW_config *cfg)
{
  if (WIFEXITED(status)) {
    // exited normally
    int childstatus =    WEXITSTATUS(status);
    if (childstatus) {
      LMW_log_error("Failure in child that should send email exit code : %d %s\n",
		    childstatus, strerror(childstatus));
      if (cfg) cfg->failures++;
    }
    return childstatus;
  } else { // subprocess was interrupted
    LMW_log_error("Failure in child that should send email, terminated ?\n");
    if (cfg)  cfg->failures++;
    return -4;
  }
}

/***
   This code will send an email to recipient, with subject, and body
   it will wait for at most max_wait milliseconds
   (to avoid hanging the caller, if /bin/mail hangs) 
   and return an exit value: 
   0 all ok 
   -1 could not invoke /bin/mail
   -2 PIPE ERROR when sending body
   -3 waiting timeout, child did not finish in less than LMW_MAX_WAIT milliseconds
   -4 child process was terminated by signal
   >0 error code from /bin/mail
*/

int LMW_send_email(char *recipient, char *subject, char *body, LMW_config *cfg) {
    int pipefd[2];
    pid_t pid;
    char *mailer = cfg ? cfg->mailer : LMW_MAILER;
    int max_wait = cfg ? cfg->max_wait : LMW_MAX_WAIT;
    char *args[] = {mailer, "-s", subject, recipient, NULL};
    
    // Handle null parameters
    if (!recipient || !subject || !body) {
        LMW_log_error("Null parameter passed to LMW_send_email\n");
        if (cfg) cfg->failures++;
        return -1;
    }

    if (pipe(pipefd) == -1) {
      LMW_log_error("Failure in creating pipe to send email: %d %s\n", errno, strerror(errno));
      if (cfg) cfg->failures ++;
      return(-1);
    }

    // Make write end non-blocking to help avoid SIGPIPE issues
    if (__LMW__make_nonblocking(pipefd[1]) == -1) {
        LMW_log_error("Warning: could not make pipe non-blocking: %d %s\n", errno, strerror(errno));
        // Continue anyway - this is not fatal
    }
    
    pid = fork();
    if (pid == -1) {
      LMW_log_error("Failure in forking child that should send email: %d %s\n",
		    errno, strerror(errno));
      close(pipefd[0]);
      close(pipefd[1]);
      if (cfg) cfg->failures++;
      return(-1);
    }

    if (pid == 0) {
        // Child process
        close(pipefd[1]);    // Close write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect pipe read end to stdin
        close(pipefd[0]);

        execvp(args[0], args);
        // If we get here, exec failed
        LMW_log_error("Failure in exec child that should send email: %d %s\n", errno, strerror(errno));
        _exit(errno);
    }
    // Parent process
    close(pipefd[0]); // Close read end

    // Save current SIGPIPE handler and ignore SIGPIPE temporarily
    // We'll detect broken pipe via write() return value
    void (*old_sigpipe_handler)(int) = signal(SIGPIPE, SIG_IGN);


    // count how many time we sleep
    int count = 0;
    int write_error = 0;
    size_t l = strlen(body);
    const size_t OL = l;
    ssize_t r;
    char *b=body;
    while(l>0 && count < max_wait) {
      r = write(pipefd[1], b, l);
      if( r == -1) {
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
	  // Non-blocking write would block, wait a bit and try again
	  usleep(1000);
	  count ++;
	  continue;
	} else {
	  LMW_log_error("Failure in piping body to send email: %d %s\n", errno, strerror(errno));
	  write_error = errno;
	  break;
	}
      }
      l += -r;
      b +=  r;
    }
    close(pipefd[1]); // EOF for child process input

    // Restore previous SIGPIPE handler
    signal(SIGPIPE, old_sigpipe_handler);

    if (count == max_wait) {
      LMW_log_error("Timeout in piping to child that should send email, only %lu of %lu sent, waited %d ms\n",
		    OL-l, OL, count);
    }

    pid_t wp;
    int status;
    if (count == max_wait || write_error) {
      // try to obtain the reason why
      usleep(1000);
      wp = waitpid(pid, &status, WNOHANG);
      if (wp == pid ) {
	return __LMW__process_exit_status__(status, cfg);
      }
      // Kill the child process since we had a write problem
      LMW_log_error("Killing child emailer, pid %d\n", pid);
      kill(pid, SIGTERM);
      waitpid(pid, NULL, 0); // Clean up zombie
      if (cfg) cfg->failures++;
      return -3;
    }
    
    // Wait specifically for the child process

    wp = waitpid(pid, &status, WNOHANG);
    while ( wp == 0 && count <  max_wait) {
      if ( usleep(1000) != 0) {
	LMW_log_error("Error in usleep: %d %s\n", errno, strerror(errno));
	break;
      }
      wp = waitpid(pid, &status, WNOHANG);
      count++;
    }
    
    if ( wp == 0) {
      LMW_log_error("Timeout in waiting for child that should send email, waited %d ms\n", count);
      if (cfg) cfg->failures ++;
      //... we are not waiting further..
      return -2;
    }
    
#ifdef LMW_DEBUG
    LMW_log_error("For child that should send email, waited %d ms\n", count);
#endif
	
    if ( wp == -1) {
      LMW_log_error("Failure in waiting for child that should send email\n");
      if (cfg) cfg->failures++;
      return -2;
    }

    return __LMW__process_exit_status__(status, cfg);
}

