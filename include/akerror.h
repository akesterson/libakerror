#ifndef _AKERR_H_
#define _AKERR_H_

#if defined(AKERR_USE_STDLIB) && AKERR_USE_STDLIB == 1
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#endif

#define AKERR_MAX_ERROR_CONTEXT_STRING_LENGTH              1024
#define AKERR_MAX_ERROR_NAME_LENGTH                        64
#define AKERR_MAX_ERROR_FNAME_LENGTH                       256
#define AKERR_MAX_ERROR_FUNCTION_LENGTH                    128
#define AKERR_MAX_ERROR_STACKTRACE_BUF_LENGTH              2048

#define AKERR_NULLPOINTER           1
#define AKERR_OUTOFBOUNDS           2
#define AKERR_API                   3
#define AKERR_ATTRIBUTE             4
#define AKERR_TYPE                  5
#define AKERR_KEY                   6                    
#define AKERR_HEAP                  7
#define AKERR_INDEX                 8
#define AKERR_FORMAT                9
#define AKERR_IO                    10
#define AKERR_REGISTRY              11
#define AKERR_VALUE                 12
#define AKERR_BEHAVIOR              13
#define AKERR_RELATIONSHIP          14

#ifndef AKERR_MAX_ERR_VALUE
#define AKERR_MAX_ERR_VALUE 14
#endif

extern char __AKERR_ERROR_NAMES[AKERR_MAX_ERR_VALUE+1][AKERR_MAX_ERROR_NAME_LENGTH];

#define AKERR_MAX_ARRAY_ERROR                    128


typedef struct
{
    char message[AKERR_MAX_ERROR_CONTEXT_STRING_LENGTH];
    int arrayid;
    int status;
    bool handled;
    int refcount;
    char fname[AKERR_MAX_ERROR_FNAME_LENGTH];
    char function[AKERR_MAX_ERROR_FNAME_LENGTH];
    int lineno;
    bool reported;
    char stacktracebuf[AKERR_MAX_ERROR_STACKTRACE_BUF_LENGTH];
    char *stacktracebufptr;
} akerr_ErrorContext;

#define AKERR_NOIGNORE __attribute__((warn_unused_result))

typedef void (*akerr_ErrorUnhandledErrorHandler)(akerr_ErrorContext *errctx);
typedef void (*akerr_ErrorLogFunction)(const char *f, ...);

extern akerr_ErrorContext AKERR_ARRAY_ERROR[AKERR_MAX_ARRAY_ERROR];
extern akerr_ErrorUnhandledErrorHandler akerr_handler_unhandled_error;
extern akerr_ErrorLogFunction akerr_log_method;
extern akerr_ErrorContext *__akerr_last_ignored;

akerr_ErrorContext AKERR_NOIGNORE *akerr_release_error(akerr_ErrorContext *ptr);
akerr_ErrorContext AKERR_NOIGNORE *akerr_next_error();
char *akerr_name_for_status(int status, char *name);
void akerr_init();
void akerr_default_handler_unhandled_error(akerr_ErrorContext *ptr);
void akerr_default_logger(const char *f, ...);

#define LOG_ERROR_WITH_MESSAGE(__err_context, __err_message)		\
    akerr_log_method("%s%s:%s:%d: %s %d (%s): %s", (char *)&__err_context->stacktracebuf, (char *)__FILE__, (char *)__func__, __LINE__, __err_message, __err_context->status, akerr_name_for_status(__err_context->status, NULL), __err_context->message); \

#define LOG_ERROR(__err_context)		\
    LOG_ERROR_WITH_MESSAGE(__err_context, "");

#define RELEASE_ERROR(__err_context)				\
    if ( __err_context != NULL ) {				\
	__err_context = akerr_release_error(__err_context);	\
    }

#define PREPARE_ERROR(__err_context)					\
    akerr_init();							\
    akerr_ErrorContext __attribute__ ((unused)) *__err_context = NULL;

#define ENSURE_ERROR_READY(__err_context)				\
    if ( __err_context == NULL ) {					\
	__err_context = akerr_next_error();				\
	if ( __err_context == NULL ) {					\
	    akerr_log_method("%s:%s:%d: Unable to pull an error context from the array!", __FILE__, (char *)__func__, __LINE__); \
	    exit(1);							\
	}								\
    }									\
    __err_context->refcount += 1;

/* 
 * Failure and success methods for functions that return akerr_ErrorContext *
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
    snprintf((char *)__err_context->fname, AKERR_MAX_ERROR_FNAME_LENGTH, __FILE__); \
    snprintf((char *)__err_context->function, AKERR_MAX_ERROR_FUNCTION_LENGTH, __func__); \
    __err_context->lineno = __LINE__;					\
    snprintf((char *)__err_context->message, AKERR_MAX_ERROR_CONTEXT_STRING_LENGTH, __message, ## __VA_ARGS__); \
    __err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, AKERR_MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d: %d (%s) : %s\n", (char *)__err_context->fname, (char *)__err_context->function, __err_context->lineno, __err_context->status, akerr_name_for_status(__err_context->status, NULL), __err_context->message);


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
	__err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, AKERR_MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d: Detected error %d from array (refcount %d)\n", (char *)__FILE__, (char *)__func__, __LINE__, __err_context->arrayid, __err_context->refcount); \
	if ( __err_context->status != 0 ) {				\
	    __err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, AKERR_MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d\n", (char *)__FILE__, (char *)__func__, __LINE__); \
	    break;							\
	}								\
    }

#define CATCH(__err_context, __stmt)			\
    DETECT(__err_context, __err_context = __stmt);

#define IGNORE(__stmt)							\
    __akerr_last_ignored = __stmt;					\
    if ( __akerr_last_ignored != NULL ) {				\
	LOG_ERROR_WITH_MESSAGE(__akerr_last_ignored, "** IGNORED ERROR **"); \
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
	    __err_context->stacktracebufptr += snprintf(__err_context->stacktracebufptr, AKERR_MAX_ERROR_STACKTRACE_BUF_LENGTH, "%s:%s:%d\n", (char *)__FILE__, (char *)__func__, __LINE__); \
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
	    akerr_handler_unhandled_error(__err_context);		\
	}								\
    }									\
    RELEASE_ERROR(__err_context);
  
#define CATCH_AND_RETURN(__err_context, __stmt) \
    ATTEMPT {					\
	CATCH(__err_context, __stmt);		\
    } CLEANUP {					\
      } PROCESS(__err_context) {		\
	} FINISH(__err_context, true);		
  

#endif // _AKERR_H_
