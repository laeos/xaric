/*
**  ./xversion.c -- Version Information for xaric (syntax: C/C++)
**  [automatically generated and maintained by GNU shtool]
*/

#ifdef ___XVERSION_C_AS_HEADER_

#ifndef ___XVERSION_C_
#define ___XVERSION_C_

#define XARICVERSION 0x00A203

typedef struct {
    const int   v_hex;
    const char *v_short;
    const char *v_long;
    const char *v_tex;
    const char *v_gnu;
    const char *v_web;
    const char *v_sccs;
    const char *v_rcs;
} XARICversion_t;

extern XARICversion_t XARICversion;

#endif /* ___XVERSION_C_ */

#else /* ___XVERSION_C_AS_HEADER_ */

#define ___XVERSION_C_AS_HEADER_
#include "./xversion.c"
#undef  ___XVERSION_C_AS_HEADER_

XARICversion_t XARICversion = {
    0x00A203,
    "0.10.3",
    "0.10.3 (27-Aug-2000)",
    "This is xaric, Version 0.10.3 (27-Aug-2000)",
    "xaric 0.10.3 (27-Aug-2000)",
    "xaric/0.10.3",
    "@(#)xaric 0.10.3 (27-Aug-2000)",
    "$Id$"
};

#endif /* ___XVERSION_C_AS_HEADER_ */

