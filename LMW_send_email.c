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


#ifndef LMW_SKIP_HEADERS
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
#include <signal.h>
#include <stdarg.h>
#endif  //LMW_SKIP_HEADERS

#include "LMW_send_email.h"

// Default logging function
static void __LMW__default_log_error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

// warning: this assumes that there is a variable called "cfg"
// of type  "LMW_config *cfg"
#define LMW_log_error( msg, ...) \
  { if (cfg && cfg->log_error ) cfg->log_error(msg, ##__VA_ARGS__);}


void LMW_config_init(LMW_config *cfg)
{
  *cfg = (LMW_config) {
    .mailer = LMW_MAILER,
    .max_wait = LMW_MAX_WAIT,
    .failures = 0,
    .log_error = __LMW__default_log_error,
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
    int childstatus = WEXITSTATUS(status);
    if (childstatus) {
      LMW_log_error("Failure in child that should send email exit code : %d %s\n",
		    childstatus, strerror(childstatus));
      if (cfg) cfg->failures++;
    }
    return childstatus;
  } else if (WIFSIGNALED(status)) {
    // subprocess was terminated by signal
    int sig = WTERMSIG(status);
    LMW_log_error("Failure in child that should send email, terminated by signal %d\n", sig);
    if (cfg)  cfg->failures++;
    return LMW_ERROR_SIGNAL;
  } else {
    // other termination
    LMW_log_error("Failure in child that should send email, terminated abnormally\n");
    if (cfg)  cfg->failures++;
    return LMW_ERROR_SIGNAL;
  }
}


static void __LMW_clean_up_tmp(int stdout_fd, int stderr_fd,
			       char *stdout_path, char *stderr_path,
			       LMW_config *cfg)
{
      // Now handle the temporary files
    close(stdout_fd);
    close(stderr_fd);

    // Check stdout file
    struct stat st;
    if (stat(stdout_path, &st) == 0) {
        if (st.st_size == 0) {
            unlink(stdout_path);
        } else {
            LMW_log_error("Mail command stdout captured in: %s (size: %ld bytes)\n",
                         stdout_path, (long)st.st_size);
        }
    } else {
        LMW_log_error("Warning: could not stat stdout temp file %s: %d %s\n",
                     stdout_path, errno, strerror(errno));
        unlink(stdout_path); // Try to clean up anyway
    }

    // Check stderr file
    if (stat(stderr_path, &st) == 0) {
        if (st.st_size == 0) {
            unlink(stderr_path);
        } else {
            LMW_log_error("Mail command stderr captured in: %s (size: %ld bytes)\n",
                         stderr_path, (long)st.st_size);
        }
    } else {
        LMW_log_error("Warning: could not stat stderr temp file %s: %d %s\n",
                     stderr_path, errno, strerror(errno));
        unlink(stderr_path); // Try to clean up anyway
    }

}

static void __LMW__kill_gracefully__(pid_t pid, int count, int max_wait, LMW_config *cfg)
{
  pid_t wp=0;
  int status=0;
  // Kill the child process since we had a write problem
  LMW_log_error("Terminating child emailer, pid %d\n", pid);
  kill(pid, SIGTERM);
  // Wait a bit for graceful termination
  max_wait += 100;
  do {
    if ( usleep(1000) != 0) {
      LMW_log_error("Error in usleep: %d %s\n", errno, strerror(errno));
      break;
    }
    count++;
    wp = waitpid(pid, &status, WNOHANG);
  }   while (wp == 0 && count < max_wait);
  if (wp == 0) {
    // Still running, force kill
    LMW_log_error("Killing child emailer, pid %d\n", pid);
    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0); // This should not block after SIGKILL
  }
}

/***
   This code will send an email to recipient, with subject, and body
   it will wait for at most max_wait milliseconds
   (to avoid hanging the caller, if /bin/mail hangs) 
   and return an exit value: 
   LMW_OK (0)                = all ok 
   LMW_ERROR_CANNOT_CALL (-1) = could not invoke /bin/mail
   LMW_ERROR_PIPE (-2)        = PIPE ERROR when sending body
   LMW_ERROR_TIMEOUT (-3)     = waiting timeout, child did not finish in less than max_wait milliseconds
   LMW_ERROR_SIGNAL (-4)      = child process was terminated by signal
   >0                         = error code from /bin/mail
*/


int LMW_send_email(LMW_config *cfg, char *recipient, char *subject, char *body) {
  char *argv[1] = { NULL };
  int ret = LMW_send_email_argv(cfg, recipient, subject, body, 0, argv);
  return ret;
}


int LMW_send_email_argc(LMW_config *cfg, char *recipient, char *subject, char *body, int argc, ...) {
    char **argv = calloc(sizeof(char *), argc+1);
    va_list ap;
    va_start(ap, argc);
    for (int j = 0; j < argc; j++) {
      argv[j] = strndup(va_arg(ap, char *), LMW_SEND_EMAIL_MAX_LEN_ARGS);
    }
    va_end(ap);
    argv[argc] = NULL;
    int ret = LMW_send_email_argv(cfg, recipient, subject, body, argc, argv);
    for(int j=0; j< argc; j++)
      free(argv[j]);
    free(argv);
    return ret;
}

int LMW_send_email_argv(LMW_config *cfg, char *recipient, char *subject, char *body, int argc, char *argv[]) {
    int pipefd[2];
    pid_t pid;
    char *mailer = cfg ? cfg->mailer : LMW_MAILER;
    int max_wait = cfg ? cfg->max_wait : LMW_MAX_WAIT;

    // Temporary file descriptors and paths
    int stdout_fd = -1, stderr_fd = -1;
    char stdout_path[] = "/tmp/lmw_stdout_XXXXXX";
    char stderr_path[] = "/tmp/lmw_stderr_XXXXXX";

    // Handle null parameters
    if (!recipient || !subject || !body) {
        LMW_log_error("Null parameter passed to LMW_send_email\n");
        if (cfg) cfg->failures++;
        return LMW_ERROR_CANNOT_CALL;
    }

    // Create temporary files for stdout and stderr
    stdout_fd = mkstemp(stdout_path);
    if (stdout_fd == -1) {
        LMW_log_error("Failed to create temporary file for stdout: %d %s\n", errno, strerror(errno));
        if (cfg) cfg->failures++;
        return LMW_ERROR_CANNOT_CALL;
    }
   
    stderr_fd = mkstemp(stderr_path);
    if (stderr_fd == -1) {
        LMW_log_error("Failed to create temporary file for stderr: %d %s\n", errno, strerror(errno));
        close(stdout_fd);
        unlink(stdout_path);
        if (cfg) cfg->failures++;
        return LMW_ERROR_CANNOT_CALL;
    }

    // create the pipe for the body
    if (pipe(pipefd) == -1) {
      LMW_log_error("Failure in creating pipe to send email: %d %s\n", errno, strerror(errno));
      close(stdout_fd);
      close(stderr_fd);
      unlink(stdout_path);
      unlink(stderr_path);
      if (cfg) cfg->failures ++;
      return LMW_ERROR_CANNOT_CALL;
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
      close(stdout_fd);
      close(stderr_fd);
      unlink(stdout_path);
      unlink(stderr_path);
      if (cfg) cfg->failures++;
      return LMW_ERROR_CANNOT_CALL;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[1]);    // Close write end
        dup2(pipefd[0], STDIN_FILENO); // Redirect pipe read end to stdin
        close(pipefd[0]);

        // Save original stdout,stderr before redirecting (for error reporting if exec fails)
        int orig_stdout = dup(STDOUT_FILENO);
        int orig_stderr = dup(STDERR_FILENO);

        // Redirect stdout and stderr to temporary files
        dup2(stdout_fd, STDOUT_FILENO);
        dup2(stderr_fd, STDERR_FILENO);
        close(stdout_fd);
        close(stderr_fd);

	char *args[5+argc];
	args[0] = mailer;
	args[1] = "-s";
	args[2] = subject;
	for(int j=0; j<argc; j++)
	  args[3+j] = argv[j];
	args[3 + argc] =  recipient;
	args[4 + argc] =  NULL;

        execvp(args[0], args);
        // If we get here, exec failed
        int saved_errno = errno; // Save errno before any system calls
	dup2(orig_stdout, STDOUT_FILENO); // Restore original stdout
        dup2(orig_stderr, STDERR_FILENO); // Restore original stderr
        close(orig_stdout);
	close(orig_stderr);
        LMW_log_error("Failure in exec child that should send email: %d %s\n", saved_errno, strerror(saved_errno));
	// exit
        _exit(LMW_CHILD_EXEC_FAILED);
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
	} else if (errno == EPIPE) {
	  LMW_log_error("Broken pipe when sending email body (child may have exited early)\n");
	  write_error = errno;
	  break;
	} else {
	  LMW_log_error("Failure in piping body to send email: %d %s\n", errno, strerror(errno));
	  write_error = errno;
	  break;
	}
      }
      l += -r;
      b +=  r;
    }


    
    // Add a final newline if the body doesn't end with one and we haven't had errors
    if (!write_error && OL > 0 && body[OL - 1] != '\n') {
        if (write(pipefd[1], "\n", 1) == -1) {
            if (errno != EPIPE) {
                LMW_log_error("Failed to write final newline: %d %s\n", errno, strerror(errno));
            }
            write_error = errno;
        }
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
	__LMW_clean_up_tmp(stdout_fd, stderr_fd, stdout_path, stderr_path, cfg);
	return __LMW__process_exit_status__(status, cfg);
      }
      __LMW__kill_gracefully__(pid, count, max_wait, cfg);
      if (cfg) cfg->failures++;
      __LMW_clean_up_tmp(stdout_fd, stderr_fd, stdout_path, stderr_path, cfg);
      return write_error ? LMW_ERROR_PIPE : LMW_ERROR_TIMEOUT;
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
      __LMW__kill_gracefully__(pid, count, max_wait, cfg);
      if (cfg) cfg->failures ++;
      __LMW_clean_up_tmp(stdout_fd, stderr_fd, stdout_path, stderr_path, cfg);
      return LMW_ERROR_TIMEOUT;
    }
    
#ifdef LMW_DEBUG
    LMW_log_error("For child that should send email, waited %d ms\n", count);
#endif
	
    if ( wp == -1) {
      LMW_log_error("Failure in waiting for child that should send email\n");
      if (cfg) cfg->failures++;
      __LMW_clean_up_tmp(stdout_fd, stderr_fd, stdout_path, stderr_path, cfg);
      return LMW_ERROR_CANNOT_CALL;
    }

    __LMW_clean_up_tmp(stdout_fd, stderr_fd, stdout_path, stderr_path, cfg);
    return __LMW__process_exit_status__(status, cfg);
}

