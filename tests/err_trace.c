#include "akerror.h"

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
    } FINISH_NORETURN(errctx);
}
