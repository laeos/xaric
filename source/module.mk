# module.mk : makefile snippit
# Copyright (c) 2000 Rex Feany (laeos@xaric.org)
#
# This file is a part of Xaric, a silly IRC client.
# You can find Xaric at http://www.xaric.org/.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# @(#)module.mk 1.4
#


# All of the objects in this directory
lobjs = 		\
	alloca.o	\
	bsearch.o	\
	cmd_hostname.o	\
	cmd_modes.o	\
	cmd_orignick.o	\
	cmd_save.o	\
	cmd_scan.o	\
	cmd_who.o	\
	commands.o	\
	ctcp.o		\
	dcc.o		\
	debug.o		\
	env.o		\
	exec.o		\
	expr.o		\
	files.o		\
	flood.o		\
	fset.o		\
	functions.o	\
	funny.o		\
	hash.o		\
	help.o		\
	history.o	\
	hook.o		\
	ignore.o	\
	input.o		\
	irc.o		\
	ircaux.o	\
	ircsig.o	\
	keys.o		\
	lastlog.o	\
	list.o		\
	log.o		\
	misc.o		\
	names.o		\
	ncommand.o	\
	network.o	\
	newio.o		\
	notice.o	\
	notify.o	\
	numbers.o	\
	output.o	\
	parse.o		\
	readlog.o	\
	reg.o		\
	screen.o	\
	server.o	\
	status.o	\
	tcommand.o	\
	term.o		\
	ternary.o	\
	timer.o		\
	vars.o		\
	whois.o		\
	whowas.o	\
	window.o	\
	words.o		\
	util.o		\
	xaric_vers.o

O_OBJS		+= $(addprefix source/, $(sort $(lobjs)))

