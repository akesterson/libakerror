# Summary

This library provides a TRY/CATCH style exception handling mechanism for C. 

# Why?

There is nothing wrong with C as it is. This library does not claim to fix some problem with C. 

Instead, this library implements a pragmatic and stylistic choice to assist the programmer in better handling errors in their programs. Vanilla C provides everything you need to do this out of the box, but this library makes it easier to avoid pointing certain guns at your foot, and when you do, it provides better context with those errors to help you more quickly recover.

Why? Because some programmers prefer to have the power of C with just a little bit of help in managing their errors.

# Library Architecture

## Philosophy of Use

This library has 6 guiding principles:

* Manually checking every possible return code for every possible meaning of that return code is tedious and prone to miss unpredicted failure cases
* Functions should return rich descriptive error contexts, not values
* Uncaught errors should cause program termination with a stacktrace
* Dynamic memory allocation is the source of many errors and should be avoided if possible
* Manipulating the call stack directly is error prone and dangerous
* Declaring, capturing, and reacting to errors should be intuitive and no more difficult than managing return codes

## Lifecycle of an error in the AKError library

TL;DR - `ErrorContext` objects are filled with error context information and bubbled up through nested control structures until they are handled or reach the top level, where an unhandled error halts program termination with a stack trace

1. At the point where an error occurs, an `ErrorContext` object is created and populated with information regarding the failure
2. The ErrorContext is returned from the scope where the error was detected
3. The ErrorContext enters a control structure provided by the AKError library through a series of macros that examine `ErrorContext` objects as they pass through
4. The control structure checks to see if the `ErrorContext` has an error set, and if so, if there are any handlers in the current control structure that can handle it
5. If the current control structure can handle the `ErrorContext`, it does so
6. If the current control structure can not handle the `ErrorContext`, then the current control structure's cleanup code (if any) is executed, and the `ErrorContext` object is passed out of the current control structure to the parent control structure
7. Steps 2-6 are repeated through as many control structures as are necessary to reach the first level of the control structure
8. When the first level of the control structure is reached, if the `ErrorContext` has an error set in it, then the stack trace information in the `ErrorContext` object is used to print a stack trace using the configured logging function, and program termination is halted

## What is in an Error Context

The Error Context object is a simple object which contains a few things:

* A numeric error code
* The name of the file in which the error occurred
* The name of the function in which the error occurred
* The line number in the file at which the error occurred
* A character buffer containing a message about the error in question

The structure also contains housekeeping information for the library which are of no specific interest to the user. See [include/akerror.h](include/akerror.h) for more details.

## What are the control structures

The library is structured around a series of macros that construct `switch` statements that perform logic against an `ErrorContext` which exists in the current scope and has been initialized. These macros must be assembled in a specific order to produce a syntactically correct `switch` statement which performs correct operations against the `ErrorContext` to attempt operations, detect failures, perform cleanup operations, handle errors, and then exit a given scope in a success or failure state.

## Functions and Return Codes

This library can catch errors from any function or expression that returns an integer value, or from functions that return `ErrorContext *`. 

Any function which uses the `PREPARE_ERROR` macro should have a return type of `ErrorContext *`. The macros within this library, when they detect an unhandled error, will attempt to pass up the unhandled error to the context of the previous function in the call stack. This allows for errors to propagate up through the call stack in the same way as exceptions. (For example, if you use traditional C error handling in a call stack of `a() -> b() -> c()`, and `c()` fails because it runs out of memory, `b()` will likely detect that error and return some error to `a()`, but it may or may not return the context of what failed and why. With this, you get that context all the way up in `a()` without knowing anything about `c()`.

## Error codes

The library uses integer values to specify error codes inside of its context. These integer return codes are defined in `akerror.h` in the form of `ERR_xxxxx` where `xxxxx` is the name of the error code in question. See `akerror.h` for a list of defined errors and their descriptions. 

You can define additional error types by defining additional `ERR_xxxxx` values. Error values up to 127 are reserved by the library, so begin your error values at 128. Define a human-friendly name for the error with the `error_name_for_status` method:

```c
error_name_for_status(129, "Some Error Code Description")
```

When you add additional error codes, you need to define `-DMAX_ERR_VALUE=n` to the compiler, where `n` is the maximum error code you have defined.

# Installation

```bash
cmake -S . -B build
cmake --build build
cmake --install build
```

## Dependencies

This library depends upon `stdlib`. If you don't want to link against stdlib, you must modify the library code to include headers and link against a library that provides the following:

- `memset` function
- `strncpy` function
- `sprintf` function
- `exit` function
- `bool` type
- `NULL` type

... then you can compile it thusly:

```
cmake -S . -B build -DAKERROR_USE_STDLIB=OFF
cmake --build build
cmake --install build
```

# Using the library

## Setting up your project

Include it

```c
#include <akerror.h>
```

Link the library directly, or

```sh
cc -lakerror
```

Using pkg-config, or

```sh
pkg-config akerror --cflags
pkg-config akerror --ldflags
```

Using cmake:

```cmake
find_package(akerror REQUIRED)
pkg_check_modules(akerror REQUIRED akerror)
target_link_libraries(YOUR_TARGET PRIVATE akerror::akerror)
```


## (Optional) Configuring the logging function

The default logging function (used for logging stack traces on failure) defaults to a wrapper that calls `fprintf(stderr, f, ...)`. If you want to override this behavior, then set the error handler to a function with a printf-style signature:

```
void my_logger(const char *fmt, ...)
{
	/* ... do something */
}


/* set your custom error handler */
error_log_method = &my_logger;
	
/* proceed to use the library */
```

## Setting Up the Error Context

Before you can use any of these macros you must set up an error context inside of the current scope.

```c
PREPARE_ERROR(errctx);
```

This will create a ErrorContext structure inside of the current scope named `errctx` and initialize it. This structure is used for all operations of the library within the current scope. Attempting to use the library in a given scope before calling this will result in compile-time errors.

## Attempting an Operation

```c
ATTEMPT {
	// ... code
} CLEANUP {
} PROCESS(errctx) {
} FINISH(errctx, true)
```

`ATTEMPT { ... }` is the block within which you will perform operations which may cause errors that need to be caught. See "Capturing errors", below.

`CLEANUP { ... }` is the block within which you will perform any code which MUST be executed REGARDLESS of whether or not errors were thrown. Closing open file handles, or releasing memory, for example.

`PROCESS(errctx) { ... }` is the block within which you will handle any errors that were caught inside of the `ATTEMPT` block. See "Handling Errors" below.

`FINISH(errctx, true)` terminates the attempt operation. The `FINISH` macro takes two arguments: the name of the ErrorContext, and a boolean regarding whether or not to pass unhandled errors up to the calling function. Unless you have a good reason not to, this should be true.

# Capturing errors

Inside of an `ATTEMPT` block, any operation which could generate or represent an error should be wrapped in one of several macros.

## Capturing errors from functions which return ErrorContext *

For functions that return `ErrorContext *`, you should use the `CATCH` macro.

```c
ATTEMPT {
    CATCH(errctx, errorGeneratingFunction())
} // ...
```

This will assign the return value of the function in question to the ErrorContext previously prepared in the current scope. If the function returns an ErrorContext that indicates any type of error, the `ATTEMPT` block is immediately exited, and the `CLEANUP` block begins.

## Setting errors from functions or expressions returning integer

For functions that return integer, such as logical comparisons or most standard library functions, use the `FAIL_ZERO_BREAK` and `FAIL_NONZERO_BREAK` macros. These macros allow you to capture an integer return code from an expression or function and set an error code in the current context based off that return.

Here is an example of checking for a NULL pointer

```c
ATTEMPT {
    FAIL_ZERO_BREAK(errctx, (somePointer == NULL), ERR_NULLPOINTER, "Someone gave me a NULL pointer")
} // ...
```

Here is an example of checking for two strings that are not equal

```c
ATTEMPT {
    FAIL_NONZERO_BREAK(errctx, strcmp("not", "equal"), ERR_VALUE, "Strings are not equal")
} // ...
```

When either of these two macros are used, the `ATTEMPT` block is immediately exited, and the `CLEANUP` block begins.

# Handling errors

Inside of the `PROCESS { ... }` block, you must handle any errors that occurred during the `ATTEMPT { ... }` block. You do this with `HANDLE`, `HANDLE_GROUP`, and `HANDLE_DEFAULT`.

## Handling a specific error with HANDLE

In order to handle a specific error code, use the `HANDLE` macro.

```c
} PROCESS(errctx) {
} HANDLE(errctx, ERR_NULLPOINTER) {
    // Something is complaining about a null pointer error. Do something about it.
} // ...
```

## Handling a group of errors with HANDLE_GROUP

In order to handle a group of related errors that all require the same failure behavior, use `HANDLE` followed by `HANDLE_GROUP`. For example, to handle a scenario where an IO error, key error, and index error all need to be handled the same way:

```c
} PROCESS(errctx) {
} HANDLE(errctx, ERR_IO) {
} HANDLE_GROUP(errctx, ERR_KEY) {
} HANDLE_GROUP(errctx, ERR_INDEX) {
    // error handling code goes here
}
```

This creates a fallthrough mechanism where all 3 errors get the same error handling code. Note that while the cases fall through, you can still (if desired) put some code specific to each error in that error's `HANDLE` or `HANDLE_GROUP` block; but this is not required, only the final handler needs to get any code.

The fallthrough behavior stops as soon as another `HANDLE` macro is encountered. For example, in this example, `ERR_IO`, `ERR_KEY` and `ERR_INDEX` are all handled as a group, but `ERR_RELATIONSHIP` is not.

```c
} PROCESS(errctx) {
} HANDLE(errctx, ERR_IO) {
} HANDLE_GROUP(errctx, ERR_KEY) {
} HANDLE_GROUP(errctx, ERR_INDEX) {
    // This code handles 3 error cases
} HANDLE(errctx, ERR_RELATIONSHIP) {
    // This code handles 1 error case
}
```

# Returning success or failure from functions returning ErrorContext *

If at all possible, when using this library, your functiions should return `ErrorContext *`. When returning from such functions, you should use the `SUCCEED_RETURN` and `FAIL_RETURN` macros.

## SUCCEED_RETURN

This macro is used when your function has reached the end of its happy code path and is prepared to exit successfully. This sets the ErrorContext to a successful state and exits the function.

```c
PREPARE_ERROR(errctx);
ATTEMPT {
    // ... stuff
} CLEANUP {
} PROCESS(errctx) {
} FINISH(errctx, true);
SUCCEED_RETURN(errctx);
```

## FAIL_RETURN

If the code path in the current function reaches a state wherein an error must be set and the function must return early, you can use `FAIL_RETURN` to accomplish this. Note that this should not be used inside of an `ATTEMPT { ... }` block; this immediately exits the function, preventing a `CLEANUP { ... }` block from executing. This can be safely used from inside of a `CLEANUP` or `PROCESS` block, or from anywhere within the function not inside of an `ATTEMPT { ... }` block.

The function allows you to provide printf-style variable arguments to provide a meaningful failure message.

```c
PREPARE_ERROR(errctx);
FAIL_RETURN(ERR_BEHAVIOR, "Something went horribly wrong!")
```

## Conditionally failing and returning

In addition to `FAIL_RETURN` you can also test for zero or non-zero conditions, set an error, and return from the function immediately. Use the `FAIL_ZERO_RETURN` and `FAIL_NONZERO_RETURN` macros for this. These macros can be used anywhere that `FAIL_RETURN` can be used.

```c
PREPARE_ERROR(errctx);
FAIL_ZERO_RETURN(errctx, (somePointer == NULL), ERR_NULLPOINTER, "Someone gave me a NULL pointer")
```

```c
PREPARE_ERROR(errctx);
FAIL_NONZERO_RETURN(errctx, strcmp("not", "equal"), ERR_VALUE, "Strings are not equal")
```

# Uncaught errors

## Ensuring that all error codes are captured

Any function which returns `ErrorContext *` should also be marked with `ERROR_NOIGNORE`.

```c
ErrorContext ERROR_NOIGNORE *f(...);
```

This will cause a compile-time error if the return value of such a function is not used. "Used" here means assigned to a variable - it does not necessarily mean that the value is checked. However assuming that such functions are called inside of `ATTEMPT { ... }` blocks, it is safe to assume that such returns will be caught with `CATCH(...)`; therefore this error is a generally effective safeguard against careless coding where errors are not checked.

Beware that `ERROR_NOIGNORE` is not a failsafe - it implements the `warn_unused_result` mechanic. By design users may explicitly ignore an error code from a function marked with `warn_unused_result` by explicitly casting the return to `void`.

```c
#define ERROR_NOIGNORE __attribute__((warn_unused_result))
```

## Stack Traces

Whenever an error is captured using the `FAIL_*` or `CATCH` methods, and is unhandled such that it manages to propagate all the way to the top of the caller stack without being managed, the last `FINISH` macro to touch the error will trigger a stack trace and kill the program.

Consider the `tests/err_trace.c` program which intentionally triggers this behavior. It produces output like this:

```
tests/err_trace.c:func2:7: 1 (Null Pointer Error) : This is a failure in func2
tests/err_trace.c:func2:10
tests/err_trace.c:func1:18: Detected error 0 from array (refcount 1)
tests/err_trace.c:func1:18
tests/err_trace.c:func1:21
tests/err_trace.c:main:30: Detected error 0 from array (refcount 1)
tests/err_trace.c:main:30
tests/err_trace.c:main:33: Unhandled Error 1 (Null Pointer Error): This is a failure in func2
```

From bottom to top, we have:

* The last line printed is the `FINISH` macro call that triggered the stacktrace. 
* Above that, the `CATCH()` inside of `main()` which caught the exception from `func1()` but did not handle it
* Above that, a statement that the error was detected in the `CATCH()` statement at the same line
* Above that, the `FINISH()` macro in the `func1` method which detected the presence of an unhandled error and returned it up the calling stack
* Above that, the `CATCH()` macro in the `func1` method which caught the error coming out of `func2()`
* Above that, a statement that the error was detected in the `CATCH()` statement at the same line
* Above that, the `FINISH()` macro in `func2()` which detected an unhandled error and passed it out of the function
* Above that, a reference to the line where the `FAIL()` macro set the error code and provided the message which is printed here

