/*
 * Parallel wrappers for LMW_send_email_argv()
 */

#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "LMW_send_email.h"
#include "LMW_send_email_in_thread.h"


/* ========== PTHREAD-BASED APPROACH ========== */


/**
 * Initialize a thread context with given parameters
 * Returns: 0 on success, -1 on failure
 */
static int __LMW_ctx_init(LMW_thread_context *ctx, LMW_config *cfg, char *recipient, char *subject, char *body, int argc, char *argv[]) {
    if (!recipient || !subject || !body) {
        return -1;
    }

    // Initialize basic fields
    ctx->cfg = cfg;
    ctx->argc = argc;
    ctx->result = LMW_ERROR_CANNOT_CALL;
    ctx->completed = 0;

    // Duplicate argv array and its strings
    if (argc > 0) {
        ctx->argv = malloc(sizeof(char*) * argc);
        if (!ctx->argv) {
            return -1;
        }
        
        for (int i = 0; i < argc; i++) {
            ctx->argv[i] = strdup(argv[i]);
            if (!ctx->argv[i]) {
                return -1;
            }
        }
    } else {
        ctx->argv = NULL;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        return -1;
    }
    // Duplicate strings
    ctx->recipient = strdup(recipient);
    ctx->subject = strdup(subject);
    ctx->body = strdup(body);

    
    return 0;
}

/**
 * Free all resources in a thread context
 */
static void __LMW_ctx_free(LMW_thread_context *ctx) {
    if (!ctx) return;
    
    // Free strings
    free(ctx->recipient);
    free(ctx->subject);  
    free(ctx->body);
    
    // Free argv array
    for (int i = 0; i < ctx->argc; i++) {
        free(ctx->argv[i]);
    }
    free(ctx->argv);
    
    // Destroy mutex
    pthread_mutex_destroy(&ctx->mutex);
    
    // Free the context itself
    free(ctx);
}

static void* __LMW_thread_worker(void *arg) {
    LMW_thread_context *ctx = (LMW_thread_context*)arg;
    
    int result = LMW_send_email_argv(ctx->cfg, ctx->recipient, ctx->subject, ctx->body, ctx->argc, ctx->argv);
    
    pthread_mutex_lock(&ctx->mutex);
    ctx->result = result;
    ctx->completed = 1;
    pthread_mutex_unlock(&ctx->mutex);
    
    return NULL;
}

/**
 * Start sending email asynchronously using pthread with extra arguments
 * Returns: context pointer on success, NULL on failure
 */
LMW_thread_context* LMW_send_email_argv_thread_start(LMW_config *cfg, char *recipient, char *subject, char *body, int argc, char *argv[]) {
    LMW_thread_context *ctx = malloc(sizeof(LMW_thread_context));
    if (!ctx) return NULL;
    
    // Initialize context
    if (__LMW_ctx_init(ctx, cfg, recipient, subject, body, argc, argv) != 0) {
        __LMW_ctx_free(ctx);
        return NULL;
    }
    
    // Start worker thread
    if (pthread_create(&ctx->thread, NULL, __LMW_thread_worker, ctx) != 0) {
        __LMW_ctx_free(ctx);
        return NULL;
    }
    
    return ctx;
}

/**
 * Convenience wrapper for the simple case (no extra args)
 * Returns: context pointer on success, NULL on failure
 */
LMW_thread_context* LMW_send_email_thread_start(LMW_config *cfg, char *recipient, char *subject, char *body) {
    return LMW_send_email_argv_thread_start(cfg, recipient, subject, body, 0, NULL);
}

/**
 * Wait for async email sending to complete and get result
 * Returns: email sending result, or LMW_ERROR_CANNOT_CALL on failure
 */
int LMW_send_email_thread_wait(LMW_thread_context *ctx) {
    if (!ctx) return LMW_ERROR_CANNOT_CALL;
    
    // Wait for thread to complete
    pthread_join(ctx->thread, NULL);
    
    pthread_mutex_lock(&ctx->mutex);
    int result = ctx->result;
    pthread_mutex_unlock(&ctx->mutex);
    
    // Free context and return result
    __LMW_ctx_free(ctx);
    
    return result;
}

/**
 * Check if async email sending is complete (non-blocking)
 * Returns: 1 if complete, 0 if still running, -1 on error
 */
int LMW_send_email_thread_check(LMW_thread_context *ctx) {
    if (!ctx) return -1;
    
    pthread_mutex_lock(&ctx->mutex);
    int completed = ctx->completed;
    pthread_mutex_unlock(&ctx->mutex);
    
    return completed;
}

