/*
 * term.h: header file for term.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef _TERM_H_
# define _TERM_H_

#include "irc_std.h"

extern	int	need_redraw;

extern	char	*CM, *DO, *CE, *CL, *CR, *NL, *SO, *SE, *US, *UE, *MD, *ME, *BL;
extern	int	CO, LI, SG; 
extern int putchar_x(int);

#define tputs_x(s)			(tputs(s, 0, putchar_x))
#define term_underline_on()		(tputs_x(US))
#define term_underline_off()		(tputs_x(UE))
#define term_standout_on()		(tputs_x(SO))
#define term_standout_off()		(tputs_x(SE))
#define term_clear_screen()		(tputs_x(CL))
#define term_move_cursor(c, r)		(tputs_x(tgoto(CM, (c), (r))))
#define term_cr()			(tputs_x(CR))
#define term_newline()			(tputs_x(NL))
#define	term_bold_on()			(tputs_x(MD))
#define	term_bold_off()			(tputs_x(ME))

extern	void	term_continue 	(void);
extern 	void 	term_beep 	(void);
extern	int	term_echo 	(int);
extern	int	term_init 	(void);
extern	int	term_resize 	(void);
extern	void	term_pause 	(char, char *);
extern	void	term_putchar 	(unsigned char);
extern	int	term_puts 	(char *, int);
extern	void	term_flush 	(void);
extern	int	(*term_scroll) 	(int, int, int);
extern	int	(*term_insert) 	(char);
extern	int	(*term_delete) 	(void);
extern	int	(*term_cursor_right) (void);
extern	int	(*term_cursor_left) (void);
extern	int	(*term_clear_to_eol) (void);
extern	void	term_space_erase (int);
extern	void	term_reset 	(void);

extern  void    copy_window_size (int *, int *);
extern	int	term_eight_bit 	(void);
extern	void	set_term_eight_bit (int);
extern	void	set_flow_control (int);

#endif /* _TERM_H_ */

