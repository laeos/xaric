#ifndef ircterm_h__
#define ircterm_h__
/*
 * ircterm.h - terminal handling stuffs
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
 * %W%
 *
 */

/* The termcap variables.  */
extern const char *tc_CM, *tc_CE, *tc_CL, *tc_CR, *tc_NL, *tc_AL, *tc_DL, *tc_CS, 
	*tc_DC, *tc_IC, *tc_IM, *tc_EI, *tc_SO, *tc_SE, *tc_US, *tc_UE, 
	*tc_MD, *tc_ME, *tc_SF, *tc_SR, *tc_ND, *tc_LE, *tc_BL, *tc_BS;

/* Current width/height */
extern int term_cols, term_rows;

#define tputs_x(s)			(tputs(s, 0, term_putchar_raw))
#define term_underline_on()		(tputs_x(tc_US))
#define term_underline_off()		(tputs_x(tc_UE))
#define term_standout_on()		(tputs_x(tc_SO))
#define term_standout_off()		(tputs_x(tc_SE))
#define term_clear_screen()		(tputs_x(tc_CL))
#define term_move_cursor(c, r)		(tputs_x(tgoto(tc_CM, (c), (r))))
#define term_cr()			(tputs_x(tc_CR))
#define term_newline()			(tputs_x(tc_NL))
#define	term_bold_on()			(tputs_x(tc_MD))
#define	term_bold_off()			(tputs_x(tc_ME))
#define term_cursor_right()		(tputs_x(tc_ND))
#define term_clear_to_eol()		(tputs_x(tc_CE))

void term_putchar (unsigned char c);
int  term_putchar_raw (int c);
void term_reset (void);
int term_init(void);
int term_resize(void);
void term_reset(void);
void term_continue(void);
void term_pause(void);
int term_echo(int flag);
void term_putchar(unsigned char c);
int term_puts(char *str, int len);
int term_putchar_raw(int c);
void term_flush(void);
void term_beep(void);
void term_set_flow_control(int value);
int term_eight_bit(void);
void term_set_eight_bit(int value);

/* strange function pointers, filled in at runtime */
int (*term_scroll) (int, int, int);     /* best scroll available */
int (*term_insert) (char);              /* best insert available */
int (*term_delete) (void);              /* best delete available */
int (*term_cursor_left) (void);         /* best left available */



#endif /* ircterm_h__ */
