// vim:ts=4:shiftwidth=4:et
/*

   Code to send email.

   This code will call /bin/mail  (by forking)
   and it will send it the body, then wait up to 50ms
   and report the exit status.
  
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

   You should have received a copy of the GNU Gene
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#define LWM_MAILER "/bin/mail"
#define LWM_MAX_WAIT 900 // in milliseconds

/* this code will send an email to recipient, with subject, and body 
   it will wait for at most 50milliseconds 
   (to avoid hanging the caller, if /bin/mail hangs) 
   and return an exit value: 
   0 all ok 
   -1 could not invoke /bin/mail
   -2 PIPE ERROR when sending body
   -3 waiting timeout, child did not finish in less than LWM_MAX_WAIT milliseconds
   >0 error in /bin/mail
*/





int LMW_send_email(char *recipient, char *subject, char *body) {
    int pipefd[2];
    pid_t pid;
    char *args[] = {LWM_MAILER, "-s", subject, recipient,  NULL};

    
    if (pipe(pipefd) == -1) {
      LMW_log_error("Failure in creating pipe to send email: %d %s", errno, strerror(errno));
      return(-1);
    }
    // FIXME: set O_NONBLOCK on the pipe
    
    pid = fork();
    if (pid == -1) {
      LMW_log_error("Failure in forking child that should send email: %d %s", errno, strerror(errno));
      return(-1);
    }

    if (pid == 0) {
        // Child process
        close(pipefd[1]);    // Close write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect pipe read end to stdin
        close(pipefd[0]);

        execvp(args[0], args);
	LMW_log_error("Failure in exec child that should send email: %d %s", errno, strerror(errno));
	return errno;
    } else {
      // Parent process
      close(pipefd[0]); // Close read end

      size_t l = strlen(body);
      ssize_t r;
      while(l>0) {
	r = write(pipefd[1], body, l);
	if( r == -1) {
	  LMW_log_error("Failure in piping body to send email: %d %s", errno, strerror(errno));
	  break;
	}
	l = l-r;
      }
      close(pipefd[1]); // EOF for child process input
      
      // Wait specifically for the child process
      int count = 0;
      int status;
      pid_t wp = waitpid(pid, &status, WNOHANG);
      while ( wp == 0 && count <  LWM_MAX_WAIT) {
	if ( usleep(1000) != 0) {
	  LMW_log_error("Error in usleep: %d %s", errno, strerror(errno));
	  break;
	}
	wp = waitpid(pid, &status, WNOHANG);
	count++;
      }
      
      if ( wp == 0) {
	LMW_log_error("Timeout in waiting for child that should send email, waited %d ms", count);
	//... we are not waiting further..
	return -2;
      }
#ifdef LMW_DEBUG
      else {
	LMW_log_error("For child that should send email, waited %d ms", count);
      }
#endif
	
      if ( wp == -1) {
	LMW_log_error("Failure in waiting for child that should send email");
	return -2;
      }

      if (WIFEXITED(status)) {
	// exited normally
	int childstatus =    WEXITSTATUS(status);
	if (childstatus) {
	  LMW_log_error("Failure in child that should send email exit code : %d",
		       (childstatus));
	  return childstatus;
	}
      } else { // subprocess was interrupted
	  LMW_log_error("Failure in child that should send email, terminated ?");
	return -1;
      }
    }

    return 0;
}

