/*
**  xversion.c -- Version Information for xaric (syntax: C/C++)
**  [automatically generated and maintained by GNU shtool]
*/

#ifdef _XVERSION_C_AS_HEADER_

#ifndef _XVERSION_C_
#define _XVERSION_C_

#define XVERSION 0x00A206

typedef struct {
    const int   v_hex;
    const char *v_short;
    const char *v_long;
    const char *v_tex;
    const char *v_gnu;
    const char *v_web;
    const char *v_sccs;
    const char *v_rcs;
} xversion_t;

extern xversion_t xversion;

#endif /* _XVERSION_C_ */

#else /* _XVERSION_C_AS_HEADER_ */

#define _XVERSION_C_AS_HEADER_
#include "xversion.c"
#undef  _XVERSION_C_AS_HEADER_

xversion_t xversion = {
    0x00A206,
    "0.10.6",
    "0.10.6 (10-Sep-2000)",
    "This is xaric, Version 0.10.6 (10-Sep-2000)",
    "xaric 0.10.6 (10-Sep-2000)",
    "xaric/0.10.6",
    "@(#)xaric 0.10.6 (10-Sep-2000)",
    "$Id$"
};

#endif /* _XVERSION_C_AS_HEADER_ */

