#include "akerror.h"

int x;

ErrorContext *func2(void)
{
    PREPARE_ERROR(errctx);
    ATTEMPT {
	FAIL(errctx, ERR_NULLPOINTER, "This is a failure in func2");
    } CLEANUP {
    } PROCESS(errctx) {
    } FINISH(errctx, true);
    SUCCEED_RETURN(errctx);
}

ErrorContext *func1(void)
{
    PREPARE_ERROR(errctx);
    ATTEMPT {
	CATCH(errctx, func2());
    } CLEANUP {
	x = 0;
    } PROCESS(errctx) {
    } FINISH(errctx, true);
    SUCCEED_RETURN(errctx);
}

int main(void)
{
    x = 12345;
    PREPARE_ERROR(errctx);
    ATTEMPT {
	CATCH(errctx, func1());
    } CLEANUP {
    } PROCESS(errctx) {
    } HANDLE(errctx, ERR_NULLPOINTER) {
	if ( x == 0 ) {
	    fprintf(stderr, "Cleanup works\n");
	    return 0;
	}
	return 1;
    } FINISH_NORETURN(errctx);
}
