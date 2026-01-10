#ifndef _ERROR_H_
#define _ERROR_H_

#if defined(AKERROR_USE_STDLIB) && AKERROR_USE_STDLIB == 1
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#endif

#define MAX_ERROR_CONTEXT_STRING_LENGTH              1024
#define MAX_ERROR_NAME_LENGTH                        64
#define MAX_ERROR_FNAME_LENGTH                       256
#define MAX_ERROR_FUNCTION_LENGTH                    128
#define MAX_ERROR_STACKTRACE_BUF_LENGTH              2048

#define ERR_NULLPOINTER           1
#define ERR_OUTOFBOUNDS           2
#define ERR_API                   3
#define ERR_ATTRIBUTE             4
#define ERR_TYPE                  5
#define ERR_KEY                   6                    
#define ERR_HEAP                  7
#define ERR_INDEX                 8
#define ERR_FORMAT                9
#define ERR_IO                    10
#define ERR_REGISTRY              11
#define ERR_VALUE                 12
#define ERR_BEHAVIOR              13
#define ERR_RELATIONSHIP          14

#ifndef MAX_ERR_VALUE
#define MAX_ERR_VALUE 14
#endif

extern char __ERROR_NAMES[MAX_ERR_VALUE+1][MAX_ERROR_NAME_LENGTH];

#define MAX_HEAP_ERROR                    128


typedef struct
{
    char message[MAX_ERROR_CONTEXT_STRING_LENGTH];
    int heapid;
    int status;
    bool handled;
    int refcount;
    char fname[MAX_ERROR_FNAME_LENGTH];
    char function[MAX_ERROR_FNAME_LENGTH];
    int lineno;
    bool reported;
    char stacktracebuf[MAX_ERROR_STACKTRACE_BUF_LENGTH];
    char *stacktracebufptr;
} ErrorContext;

#define ERROR_NOIGNORE __attribute__((warn_unused_result))

typedef void (*ErrorUnhandledErrorHandler)(ErrorContext *errctx);
typedef void (*ErrorLogFunction)(const char *f, ...);

extern ErrorContext HEAP_ERROR[MAX_HEAP_ERROR];
extern ErrorUnhandledErrorHandler error_handler_unhandled_error;
extern ErrorLogFunction error_log_method;
extern ErrorContext *__error_last_ignored;

ErrorContext ERROR_NOIGNORE *heap_release_error(ErrorContext *ptr);
ErrorContext ERROR_NOIGNORE *heap_next_error();
char *error_name_for_status(int status, char *name);
void error_init();
void error_default_handler_unhandled_error(ErrorContext *ptr);
void error_default_logger(const char *f, ...);

#define LOG_ERROR_WITH_MESSAGE(__err_context, __err_message)		\
    error_log_method("%s%s:%s:%d: %s %d (%s): %s", (char *)&__err_context->stacktracebuf, (char *)__FILE__, (char *)__func__, __LINE__, __err_message, __err_context->status, error_name_for_status(__err_context->status, NULL), __err_context->message); \

#define LOG_ERROR(__err_context)		\
    LOG_ERROR_WITH_MESSAGE(__err_context, "");

#define RELEASE_ERROR(__err_context)				\
    if ( __err_context != NULL ) {				\
	__err_context = heap_release_error(__err_context);	\
    }

#define PREPARE_ERROR(__err_context)					\
    error_init();							\
    ErrorContext __attribute__ ((unused)) *__err_context = NULL;

#define ENSURE_ERROR_READY(__err_context)				\
    if ( __err_context == NULL ) {					\
	__err_context = heap_next_error();				\
	if ( __err_context == NULL ) {					\
	    error_log_method("%s:%s:%d: Unable to pull an ErrorContext from the heap!", __FILE__, (char *)__func__, __LINE__); \
	    exit(1);							\
	}								\
    }									\
    __err_context->refcount += 1;

/* 
 * Failure and success methods for functions that return ErrorContext *
 */

#define FAIL_ZERO_RETURN(__err_context, __x, __err, __message, ...)	\
    if ( __x == 0 ) {							\
	FAIL(__err_context, __err, __message, ##__VA_ARGS__);		\
	return __err_context;						\
    }

#define FAIL_NONZERO_RETURN(__err_context, __x, __err, __message, ...)	\
    if ( __x != 0 ) {							\
	FAIL(__err_context, __err, __message, ##__VA_ARGS__);		\
	return __err_context;						\
    }

#define FAIL_RETURN(__err_context, __err, __message, ...)	\
    FAIL(__err_context, __err, __message, ##__VA_ARGS__);	\
    return __err_context;

#define SUCCEED_RETURN(__err_context)		\
    RELEASE_ERROR(__err_context);		\
    return NULL;

/*
 * Failure and success methods for use inside of ATTEMPT() blocks
 */

#define FAIL_ZERO_BREAK(__err_context, __x, __err, __message, ...)	\
    if ( __x == 0 ) {							\
	FAIL(__err_context, __err, __message, ##__VA_ARGS__);		\
	break;								\
    }

#define FAIL_NONZERO_BREAK(__err_context, __x, __err, __message, ...)	\
    if ( __x != 0 ) {							\
	FAIL(__err_context, __err, __message, ##__VA_ARGS__);		\
	break;								\
    }

#define FAIL_BREAK(__err_context, __err_, __message, ...)	\
    FAIL(__err_context, __err_, __message, ##__VA_ARGS__);	\
    break;

#define SUCCEED_BREAK(__err_context)		\
    SUCCEED(__err_context);			\
    break;

/*
 * General failure and success methods
 */

#define FAIL(__err_context, __err, __message, ...)			\
    ENSURE_ERROR_READY(__err_context);					\
    __err_context->status = __err;					\
    snprintf((char *)__err_context->fname, MAX_ERROR_FNAME_LENGTH, __FILE__); \
    snprintf((char *)__err_context->function, MAX_ERROR_FUNCTION_LENGTH, __func__); \
    __err_context->lineno = __LINE__;					\
    snprintf((char *)__err_context->message, MAX_ERROR_CONTEXT_STRING_LENGTH, __message, ## __VA_ARGS__); \
    __err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d: %d (%s) : %s\n", (char *)__err_context->fname, (char *)__err_context->function, __err_context->lineno, __err_context->status, error_name_for_status(__err_context->status, NULL), __err_context->message);


#define SUCCEED(__err_context)			\
    ENSURE_ERROR_READY(__err_context);		\
    __err_context->status = 0;

/*
 * Defines for the ATTEMPT/CATCH/CLEANUP/PROCESS/HANDLE/FINISH process
 */

#define ATTEMPT					\
    switch ( 0 ) {				\
    case 0:					\

#define DETECT(__err_context, __stmt)					\
    __stmt;								\
    if ( __err_context != NULL ) {					\
	__err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d: Detected error %d from heap (refcount %d)\n", (char *)__FILE__, (char *)__func__, __LINE__, __err_context->heapid, __err_context->refcount); \
	if ( __err_context->status != 0 ) {				\
	    __err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d\n", (char *)__FILE__, (char *)__func__, __LINE__); \
	    break;							\
	}								\
    }

#define CATCH(__err_context, __stmt)			\
    DETECT(__err_context, __err_context = __stmt);

#define IGNORE(__stmt)							\
    __error_last_ignored = __stmt;					\
    if ( __error_last_ignored != NULL ) {				\
	LOG_ERROR_WITH_MESSAGE(__error_last_ignored, "** IGNORED ERROR **"); \
    }

#define CLEANUP					\
    }; 

#define PROCESS(__err_context)			\
    if ( __err_context != NULL ) {		\
    switch ( __err_context->status ) {		\
    case 0:					\
    __err_context->handled = true;

#define HANDLE(__err_context, __err_status)				\
    break;								\
    case __err_status:							\
    __err_context->stacktracebufptr = (char *)&__err_context->stacktracebuf; \
    __err_context->handled = true;

#define HANDLE_GROUP(__err_context, __err_status)			\
    case __err_status:							\
    __err_context->stacktracebufptr = (char *)&__err_context->stacktracebuf; \
    __err_context->handled = true;

#define HANDLE_DEFAULT(__err_context)					\
    break;								\
    default:								\
    __err_context->stacktracebufptr = (char *)&__err_context->stacktracebuf; \
    __err_context->handled = true;

#define FINISH(__err_context, __pass_up)				\
    };									\
    };									\
    if ( __err_context != NULL ) {					\
	if ( __err_context->handled == false && __pass_up == true ) {	\
	    __err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d\n", (char *)__FILE__, (char *)__func__, __LINE__); \
	    return __err_context;					\
	}								\
    }									\
    RELEASE_ERROR(__err_context);

#define FINISH_NORETURN(__err_context)					\
    };									\
    };									\
    if ( __err_context != NULL ) {					\
	if ( __err_context->handled == false ) {			\
	    LOG_ERROR_WITH_MESSAGE(__err_context, "Unhandled Error");	\
	    error_handler_unhandled_error(__err_context);		\
	}								\
    }									\
    RELEASE_ERROR(__err_context);
  
#define CATCH_AND_RETURN(__err_context, __stmt) \
    ATTEMPT {					\
	CATCH(__err_context, __stmt);		\
    } CLEANUP {					\
      } PROCESS(__err_context) {		\
	} FINISH(__err_context, true);		
  

#endif // _ERROR_H_
