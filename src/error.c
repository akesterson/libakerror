#include "akerror.h"
#if defined(AKERR_USE_STDLIB) && AKERR_USE_STDLIB == 1
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#endif // AKERR_USE_STDLIB

akerr_ErrorContext __akerr_last_ditch;
akerr_ErrorContext *__akerr_last_ignored;
akerr_ErrorUnhandledErrorHandler akerr_handler_unhandled_error;
akerr_ErrorLogFunction akerr_log_method = NULL;

char __AKERR_ERROR_NAMES[AKERR_MAX_ERR_VALUE+1][AKERR_MAX_ERROR_NAME_LENGTH];

akerr_ErrorContext AKERR_ARRAY_ERROR[AKERR_MAX_ARRAY_ERROR];

void akerr_default_logger(const char *fmt, ...)
{
#if defined(AKERR_USE_STDLIB) && AKERR_USE_STDLIB == 1
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#else
    return;
#endif
}

void akerr_init()
{
    static int inited = 0;
    if ( inited == 0 ) {
	for (int i = 0; i < AKERR_MAX_ARRAY_ERROR; i++ ) {
	    memset((void *)&AKERR_ARRAY_ERROR[i], 0x00, sizeof(akerr_ErrorContext));
	    AKERR_ARRAY_ERROR[i].arrayid = i;
	    AKERR_ARRAY_ERROR[i].stacktracebufptr = (char *)&AKERR_ARRAY_ERROR[i].stacktracebuf;
	}
	__akerr_last_ignored = NULL;
	memset((void *)&__akerr_last_ditch, 0x00, sizeof(akerr_ErrorContext));
	__akerr_last_ditch.stacktracebufptr = (char *)&__akerr_last_ditch.stacktracebuf;
	if ( akerr_log_method == NULL ) {
	    akerr_log_method = &akerr_default_logger;
	}
	akerr_handler_unhandled_error = &akerr_default_handler_unhandled_error;
	memset((void *)&__AKERR_ERROR_NAMES[0], 0x00, ((AKERR_MAX_ERR_VALUE+1) * AKERR_MAX_ERROR_NAME_LENGTH));

	akerr_name_for_status(AKERR_NULLPOINTER, "Null Pointer Error");
	akerr_name_for_status(AKERR_OUTOFBOUNDS, "Out Of Bounds Error");
	akerr_name_for_status(AKERR_API, "API Error");
	akerr_name_for_status(AKERR_ATTRIBUTE, "Attribute Error");
	akerr_name_for_status(AKERR_TYPE, "Type Error");
	akerr_name_for_status(AKERR_KEY, "Key Error");
	akerr_name_for_status(AKERR_HEAP, "Heap Error");
	akerr_name_for_status(AKERR_INDEX, "Index Error");
	akerr_name_for_status(AKERR_FORMAT, "Format Error");
	akerr_name_for_status(AKERR_IO, "Input Output Error");
	akerr_name_for_status(AKERR_REGISTRY, "Registry Error");
	akerr_name_for_status(AKERR_VALUE, "Value Error");
	akerr_name_for_status(AKERR_BEHAVIOR, "Behavior Error");
	akerr_name_for_status(AKERR_RELATIONSHIP, "Relationship Error");
	
	inited = 1;
    }
}

void akerr_default_handler_unhandled_error(akerr_ErrorContext *errctx)
{
  if ( errctx == NULL ) {
    exit(1);
  }
  exit(errctx->status);
}

akerr_ErrorContext *akerr_next_error()
{
    for (int i = 0; i < AKERR_MAX_ARRAY_ERROR; i++ ) {
	if ( AKERR_ARRAY_ERROR[i].refcount == 0 ) {
	    return &AKERR_ARRAY_ERROR[i];
	}
    }
    return (akerr_ErrorContext *)NULL;
}

akerr_ErrorContext *akerr_release_error(akerr_ErrorContext *err)
{
    int oldid = 0;
    if ( err == NULL ) {
	akerr_ErrorContext *errctx = &__akerr_last_ditch;
	FAIL_RETURN(errctx, AKERR_NULLPOINTER, "akerr_release_error got NULL context pointer");
    }
    if ( err->refcount > 0 ) {
      err->refcount -= 1;
    }
    if ( err->refcount == 0 ) {
	oldid = err->arrayid;
	memset(err, 0x00, sizeof(akerr_ErrorContext));
	err->stacktracebufptr = (char *)&err->stacktracebuf;
	err->arrayid = oldid;
	return NULL;
    }
    return err;
}


// returns or sets the name for the given status.
// Call with name = NULL to retrieve a status.
char *akerr_name_for_status(int status, char *name)
{
    if ( status > AKERR_MAX_ERR_VALUE ) {
	return "Unknown Error";
    }
    if ( name != NULL ) {
	strncpy((char *)&__AKERR_ERROR_NAMES[status], name, AKERR_MAX_ERROR_NAME_LENGTH);	
    }
    return (char *)&__AKERR_ERROR_NAMES[status];
}
