/*
 * stdlib.h
 *
 * Standard library functions
 *
 * PSXSDK
 */

#ifndef _STDLIB_H
#define _STDLIB_H

typedef unsigned int size_t;
typedef signed int ssize_t;

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#ifndef NULL
#define NULL    ((void*)0)
#endif /* NULL */

/* Conversion functions */

EXTERNC int atoi(const char *s);
EXTERNC long atol(const char *s);
EXTERNC char *itoa(int value, char *str, int base);
EXTERNC char *ltoa(long value, char *str, int base);
EXTERNC char *lltoa(long long value, char *str, int base);
EXTERNC char *utoa(unsigned int value, char *str, int base);
EXTERNC char *ultoa(unsigned long value, char *str, int base);
EXTERNC char *ulltoa(unsigned long long value, char *str, int base);
//extern char atob(char *s); // Is this right?


// Random number functions

#define RAND_MAX        0x7fffffff

EXTERNC int rand(void);
EXTERNC void srand(unsigned int seed);

// Quick sort

EXTERNC void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

// Memory allocation functions

//#warning "malloc() family of functions NEEDS MORE TESTING"

EXTERNC void *malloc(size_t size);
EXTERNC void free(void *buf);
EXTERNC void *calloc(size_t number, size_t size);
EXTERNC void *realloc(void *buf , size_t n);

EXTERNC int abs(int x);
EXTERNC long long strtoll(const char *nptr, char **endptr, int base);
EXTERNC long strtol(const char *nptr, char **endptr, int base);
EXTERNC double strtod(const char *nptr, char **endptr);
EXTERNC long double strtold(const char *nptr, char **endptr);
EXTERNC float strtof(const char *nptr, char **endptr);

// Misc
EXTERNC void abort(void);
EXTERNC void exit(int status);
EXTERNC void call_atexit_callbacks(void);

// Program return codes

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

#endif
