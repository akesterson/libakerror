# libsdlerror

**Explicit, disciplined error handling for C — built on SDL3.**

`libsdlerror` is a small library that helps C programmers write correct, readable, and maintainable error-handling code without pretending that C is something it is not.

It does not add exceptions to C.  
It does not hide control flow.  
It does not allocate memory at runtime.

If you want magic, look elsewhere.

---

## Why This Library Exists

Error handling in C is not hard — but it is tedious, repetitive, and easy to get wrong.

The usual pattern:

```c
if (foo() < 0) {
    fprintf(stderr, "%s\n", SDL_GetError());
    cleanup();
    return -1;
}
```

works, until it doesn’t. As programs grow:

- Error checks get duplicated
- Cleanup paths multiply
- Context disappears
- Control flow becomes fragmented

SDL compounds this by storing errors in a single thread-local string, forcing programmers to either handle errors immediately or lose information.

`libsdlerror` exists to impose structure and discipline on this process — without compromising the explicitness that makes C reliable.

---

## What This Library Is — and Is Not

This library is deliberately conservative.

### 🚫 No `setjmp`, No `longjmp`

Many C error libraries attempt to simulate exceptions using `setjmp` / `longjmp`.

That approach is rejected here.

Non-local jumps:

- Bypass normal control flow
- Skip cleanup invisibly
- Break assumptions about stack lifetime
- Make reasoning about correctness harder, not easier

If control flow moves in `libsdlerror`, it does so because the source code says so.

Nothing jumps. Nothing unwinds itself. Nothing “just happens.”

---

### 🚫 No Runtime Memory Allocation — Ever

`libsdlerror` never performs dynamic memory allocation at runtime.

It does not call:

- `malloc`
- `calloc`
- `realloc`
- `free`

There is no allocator dependency and no runtime allocation failure mode.

---

### 📌 Explicit Storage Model

All memory used by `libsdlerror` falls into one of two categories:

- **Automatic storage (stack)**  
  Caller-owned error contexts and local state

- **Static storage duration**  
  Fixed-size internal tables allocated at program load time

There is no heap allocation and no runtime resizing.

---

### ✅ C Means C

- No compiler extensions
- No undefined behavior
- No optimizer tricks

If it compiles as C, it works as C.

---

### ✅ Designed for SDL, Not Against It

SDL already has an error system. It is simple, global, and fragile.

`libsdlerror` does not replace it. It captures SDL error state at the right moment, preserves context, and makes it usable without forcing error checks everywhere.

---

## The Six Guiding Principles

This library is opinionated, and intentionally so.

1. **Control flow must be explicit**  
   If execution moves, the source code must show why.

2. **No `setjmp` / `longjmp`**  
   Errors must not bypass the stack.

3. **No runtime memory allocation**  
   The library relies only on stack and static storage.

4. **The caller owns all state**  
   No hidden globals exposed to the user.

5. **Cleanup must be deterministic**  
   Cleanup always runs, exactly once.

6. **Errors are values, not side effects**  
   They are captured, inspected, propagated, or handled deliberately.

---

## Quick Start

### Build and Install

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

### Compile Your Program

```sh
gcc `pkg-config --cflags sdlerror` main.c \
    `pkg-config --libs sdlerror` \
    -o app
```

### Minimal Example

```c
#include <sdlerror.h>

int main(void) {
    PREPARE_ERROR(err);

    ATTEMPT {
        CATCH(err, SDL_Init(SDL_INIT_VIDEO));
        CATCH(err, do_something_that_can_fail());
    }
    CLEANUP {
        SDL_Quit();
    }
    PROCESS(err) {
        fprintf(stderr, "Error: %s\n", ERROR_MESSAGE(err));
        return 1;
    }
    FINISH(err, true);

    return 0;
}
```

---

## User-Configurable Error Codes

While most aspects of `libsdlerror` are intentionally fixed, error codes themselves are user-defined.

### Defining `ERR_*` Codes

```c
#define ERR_OK        0
#define ERR_SDL       1
#define ERR_IO        2
#define ERR_CONFIG    3
```

These are simple integer status values.

---

### `MAX_ERR_VALUE`

`MAX_ERR_VALUE` must be greater than or equal to the highest `ERR_*` value you define.

It determines the size of internal static tables used to map error codes to names.

If you add new error codes, you must update `MAX_ERR_VALUE` and recompile.

---

### Naming Errors: `error_name_for_status`

```c
error_name_for_status(ERR_IO, "I/O error");
error_name_for_status(ERR_CONFIG, "Configuration error");
```

Names are copied into fixed-size static buffers and persist for the lifetime of the program.

No allocation occurs.

---

## Appendix: Memory Model

### Storage Duration Overview

`libsdlerror` uses only automatic (stack) storage and static storage duration.

No heap allocation occurs at runtime.

---

### Compile-Time Limits (`MAX_ERR*`)

The `MAX_ERR*` macros in `sdlerror.h` define fixed upper bounds for:

- Error message length
- Error name length
- Stacktrace buffer size
- Number of error contexts

These values are not user-tunable at runtime.

If you need different limits, you are expected to edit the header and recompile.

---

### Static Memory Usage (x86_64)

On a typical x86_64 system:

- Error name table: ~1 KB
- Static error pool: ~450 KB
- Miscellaneous globals: a few KB

**Total static footprint: ~470 KB**

All of this memory is allocated at program load time.

---

## FAQ

### Why Use Static Globals Instead of the Heap?

Because error handling infrastructure must not itself fail.

Heap allocation introduces failure modes, ordering problems, and unpredictability.

Static storage avoids all of this.

The globals used by `libsdlerror` are fixed-size, initialized at load time, and free of allocator dependencies.

---

### Why Not Allocate Per-Error Structures Dynamically?

Because error handling must work when memory is tight, during early startup, and during shutdown.

Dynamic allocation undermines all three.

---

## Summary

`libsdlerror` is deliberately conservative.

No jumps.  
No heap allocation.  
No hidden control flow.

Just explicit, disciplined error handling — the way C has always worked when used correctly.

