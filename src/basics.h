/**
 *          ╔════════╗
 *          ║ BASICS ║
 *          ╚════════╝
 *
 * \author  August-Alm@github.com         
 *
 * \notes   General definitions and macros.
 */

/* ***** ***** */

#ifndef BASICS_H
#define BASICS_H

/* ***** ***** */

#include <stdio.h>

/* ***** ***** */

#define MALCHECK(s) if(!s) {                               \
    fprintf(stderr, "Malloc failed at line %d in `%s`.\n"  \
                  , __LINE__, __FUNCTION__);               \
    return NULL;                                           \
}                                                          \

/* ***** ***** */

#endif // BASICS_H
