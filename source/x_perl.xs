/*
 * x_perl.xs -- Xaric help system
 * (c) 199 Rex Feany (laeos@ptw.com) 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 *
 */

#include <stdio.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "output.h"

void perl_eval(char *pcode);


#define dHANDLE(name) GV *handle = gv_fetchpv(name, TRUE, SVt_PVIO)

#define TIEHANDLE(name,obj) \
{ \
      dHANDLE(name); \
      sv_unmagic((SV*)handle, 'q'); \
      sv_magic((SV*)handle, obj, 'q', Nullch, 0); \
}

static PerlInterpreter *xaric_perl;  /***    The Perl interpreter    ***/

extern void xs_init(void);

SV *new_blessed_obj(void)
{
	SV *sv = sv_newmortal();
	return newSVrv(sv, "Xaric");
}

static const char boot_strap_code[] = "$SIG{__WARN__} = sub { Xaric::warn $_[0]; }";

void
perl_init(void)
{
        char *empty_args[] = { "", "-e", "0" };
	char *bootargs[] = { "Xaric", NULL };
	SV *b, *c;

        xaric_perl = perl_alloc();
        perl_construct(xaric_perl);
        perl_parse(xaric_perl, xs_init, 3, empty_args, NULL);
	perl_call_argv("Xaric::bootstrap", (long)G_DISCARD, bootargs);
        perl_run(xaric_perl);

	/* to differenciate between stderr, stdout */
	b = newSViv(1);
	c = newSViv(2);

	newSVrv(b, "Xaric");
	newSVrv(c, "Xaric");

	TIEHANDLE("STDIN", b)
	TIEHANDLE("STDOUT", b)
	TIEHANDLE("STDERR", c)
	perl_eval(boot_strap_code); 
	yell("perl interpreter initilized...");
}

void
perl_end(void)
{
	if (xaric_perl)
	{
		perl_run(xaric_perl);
		perl_destruct(xaric_perl);
		perl_free(xaric_perl);
	}
}

void
perl_eval(char *pcode)
{
    char	*err;
    STRLEN	length;
    SV		*sv;

    dSP;
    ENTER;
    SAVETMPS;

    sv = newSVpv(pcode, 0);
    perl_eval_sv(sv, G_DISCARD | G_NOARGS);
    SvREFCNT_dec(sv);

    err = SvPV(GvSV(errgv), length);

    FREETMPS;
    LEAVE;

    if (!length)
	return;

    err[length-1]='\0';
    bitchsay(err);
}

MODULE = Xaric		PACKAGE = Xaric		

void
say(msg)
	char * msg
	CODE:
	say(msg);

void
yell(msg)
	char * msg
	CODE:
	yell(msg);

void
warn(msg)
	char * msg
	CODE:
	bitchsay("Perl Warning: %s", msg);

void
PRINT(self, msg)
	SV *self
	char * msg
	CODE:
	say("%s", msg);

