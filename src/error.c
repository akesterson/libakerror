#include "akerror.h"
#if defined(AKERROR_USE_STDLIB) && AKERROR_USE_STDLIB == 1
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#endif // AKERROR_USE_STDLIB

ErrorContext __error_last_ditch;
ErrorContext *__error_last_ignored;
ErrorUnhandledErrorHandler error_handler_unhandled_error;
ErrorLogFunction error_log_method = NULL;

char __ERROR_NAMES[MAX_ERR_VALUE+1][MAX_ERROR_NAME_LENGTH];

ErrorContext ARRAY_ERROR[MAX_ARRAY_ERROR];

void error_default_logger(const char *fmt, ...)
{
#if defined(AKERROR_USE_STDLIB) && AKERROR_USE_STDLIB == 1
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#else
    return;
#endif
}

void error_init()
{
    static int inited = 0;
    if ( inited == 0 ) {
	for (int i = 0; i < MAX_ARRAY_ERROR; i++ ) {
	    memset((void *)&ARRAY_ERROR[i], 0x00, sizeof(ErrorContext));
	    ARRAY_ERROR[i].arrayid = i;
	    ARRAY_ERROR[i].stacktracebufptr = (char *)&ARRAY_ERROR[i].stacktracebuf;
	}
	__error_last_ignored = NULL;
	memset((void *)&__error_last_ditch, 0x00, sizeof(ErrorContext));
	__error_last_ditch.stacktracebufptr = (char *)&__error_last_ditch.stacktracebuf;
	if ( error_log_method == NULL ) {
	    error_log_method = &error_default_logger;
	}
	error_handler_unhandled_error = &error_default_handler_unhandled_error;
	memset((void *)&__ERROR_NAMES[0], 0x00, ((MAX_ERR_VALUE+1) * MAX_ERROR_NAME_LENGTH));

	error_name_for_status(ERR_NULLPOINTER, "Null Pointer Error");
	error_name_for_status(ERR_OUTOFBOUNDS, "Out Of Bounds Error");
	error_name_for_status(ERR_API, "API Error");
	error_name_for_status(ERR_ATTRIBUTE, "Attribute Error");
	error_name_for_status(ERR_TYPE, "Type Error");
	error_name_for_status(ERR_KEY, "Key Error");
	error_name_for_status(ERR_HEAP, "Heap Error");
	error_name_for_status(ERR_INDEX, "Index Error");
	error_name_for_status(ERR_FORMAT, "Format Error");
	error_name_for_status(ERR_IO, "Input Output Error");
	error_name_for_status(ERR_REGISTRY, "Registry Error");
	error_name_for_status(ERR_VALUE, "Value Error");
	error_name_for_status(ERR_BEHAVIOR, "Behavior Error");
	error_name_for_status(ERR_RELATIONSHIP, "Relationship Error");
	
	inited = 1;
    }
}

void error_default_handler_unhandled_error(ErrorContext *errctx)
{
  if ( errctx == NULL ) {
    exit(1);
  }
  exit(errctx->status);
}

ErrorContext *array_next_error()
{
    for (int i = 0; i < MAX_ARRAY_ERROR; i++ ) {
	if ( ARRAY_ERROR[i].refcount == 0 ) {
	    return &ARRAY_ERROR[i];
	}
    }
    return (ErrorContext *)NULL;
}

ErrorContext *array_release_error(ErrorContext *err)
{
    int oldid = 0;
    if ( err == NULL ) {
	ErrorContext *errctx = &__error_last_ditch;
	FAIL_RETURN(errctx, ERR_NULLPOINTER, "array_release_error got NULL context pointer");
    }
    if ( err->refcount > 0 ) {
      err->refcount -= 1;
    }
    if ( err->refcount == 0 ) {
	oldid = err->arrayid;
	memset(err, 0x00, sizeof(ErrorContext));
	err->stacktracebufptr = (char *)&err->stacktracebuf;
	err->arrayid = oldid;
	return NULL;
    }
    return err;
}


// returns or sets the name for the given status.
// Call with name = NULL to retrieve a status.
char *error_name_for_status(int status, char *name)
{
    if ( status > MAX_ERR_VALUE ) {
	return "Unknown Error";
    }
    if ( name != NULL ) {
	strncpy((char *)&__ERROR_NAMES[status], name, MAX_ERROR_NAME_LENGTH);	
    }
    return (char *)&__ERROR_NAMES[status];
}
