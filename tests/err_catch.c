#include "akerror.h"

akerr_ErrorContext *func2(void)
{
    PREPARE_ERROR(errctx);
    ATTEMPT {
	FAIL(errctx, AKERR_NULLPOINTER, "This is a failure in func2");
    } CLEANUP {
    } PROCESS(errctx) {
    } FINISH(errctx, true);
    SUCCEED_RETURN(errctx);
}

akerr_ErrorContext *func1(void)
{
    PREPARE_ERROR(errctx);
    ATTEMPT {
	CATCH(errctx, func2());
    } CLEANUP {
    } PROCESS(errctx) {
    } FINISH(errctx, true);
    SUCCEED_RETURN(errctx);
}


int main(void)
{
    PREPARE_ERROR(errctx);
    ATTEMPT {
	CATCH(errctx, func1());
    } CLEANUP {
    } PROCESS(errctx) {
    } HANDLE(errctx, AKERR_NULLPOINTER) {
      akerr_log_method("Caught exception");
    } FINISH_NORETURN(errctx);
}
