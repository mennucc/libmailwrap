
typedef struct {
    LMW_config *cfg;
    char *recipient;
    char *subject;
    char *body;
    int argc;
    char **argv;
    pthread_t thread;
    int result;
    int completed;
    pthread_mutex_t mutex;
} LMW_thread_context;

/**
 * Start sending email asynchronously using pthread with extra arguments
 * Returns: context pointer on success, NULL on failure
 */
LMW_thread_context* LMW_send_email_argv_thread_start(LMW_config *cfg, char *recipient, char *subject, char *body, int argc, char *argv[]);

/**
 * Convenience wrapper for the simple case (no extra args)
 * Returns: context pointer on success, NULL on failure
 */
LMW_thread_context* LMW_send_email_thread_start(LMW_config *cfg, char *recipient, char *subject, char *body);


/**
 * Wait for async email sending to complete and get result
 * Returns: email sending result, or LMW_ERROR_CANNOT_CALL on failure
 */
int LMW_send_email_thread_wait(LMW_thread_context *ctx);

/**
 * Check if async email sending is complete (non-blocking)
 * Returns: 1 if complete, 0 if still running, -1 on error
 */
int LMW_send_email_thread_check(LMW_thread_context *ctx);





