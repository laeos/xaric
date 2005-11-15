#ifndef tcommand_h__
#define tcommand_h__

#define C_AMBIG (void *)(-1)

struct command {
    char *name;
    char *rname;

    void *data;
    void (*fcn) (struct command * cmd, char *args);

    char *qhelp;
};

struct tnode *t_insert(struct tnode *p, char *string, struct command *data);
struct tnode *t_remove(struct tnode *root, char *s, struct command **data);
struct command *t_search(struct tnode *root, char *s);
struct tnode *t_build(struct tnode *root, struct command c[], int n);
void t_traverse(struct tnode *p, char *s, void (*fcn) (struct command *, char *), char *data);
void init_commands(void);

void t_parse_command(char *command, char *line);
void t_bind_exec(struct command *cmd, char *line);
int t_bind(char *string, char *command, char *args);
int t_unbind(char *command);

/* Commands not in ncommand.c */

void cmd_hostname(struct command *cmd, char *args);	/* cmd_hostname.c */
void cmd_orig_nick(struct command *cmd, char *args);	/* cmd_orignick.c */
void cmd_scan(struct command *cmd, char *args);	/* cmd_scan.c */
void cmd_who(struct command *cmd, char *args);	/* cmd_who.c */
void cmd_chat(struct command *cmd, char *args);	/* dcc.c */
void cmd_exec(struct command *cmd, char *args);	/* exee.c */
void cmd_no_flood(struct command *cmd, char *args);	/* flood.c */
void cmd_show_hash(struct command *cmd, char *args);	/* hash.c */
void cmd_help(struct command *cmd, char *args);	/* help.c */
void cmd_history(struct command *cmd, char *args);	/* history.c */
void cmd_ignore(struct command *cmd, char *args);	/* ignore.c */
void cmd_tignore(struct command *cmd, char *args);	/* ignore.c */
void cmd_window(struct command *, char *);	/* in window.c */
void cmd_awaylog(struct command *, char *);	/* in lastlog.c */
void cmd_lastlog(struct command *, char *);	/* in lastlog.c */
void cmd_set(struct command *cmd, char *args);	/* in vars.c */
void cmd_remove_log(struct command *cmd, char *args);	/* in readlog.c */
void cmd_readlog(struct command *cmd, char *args);	/* in readlog.c */
void cmd_notify(struct command *cmd, char *args);	/* in notify.c */
void cmd_timer(struct command *cmd, char *args);	/* in timer.c */

/* in keys.c */
void cmd_parsekey(struct command *cmd, char *args);
void cmd_bind(struct command *cmd, char *args);
void cmd_rbind(struct command *cmd, char *args);
void cmd_type(struct command *cmd, char *args);

/* in cmd_save.c */
void save_all(char *fname);	/* used in cmd_abort to save settings before we die */
void cmd_save(struct command *cmd, char *args);

/* cmd_modes.c */
void cmd_unban(struct command *cmd, char *args);
void cmd_kick(struct command *cmd, char *args);
void cmd_kickban(struct command *cmd, char *args);
void cmd_ban(struct command *cmd, char *args);
void cmd_banstat(struct command *cmd, char *args);
void cmd_tban(struct command *cmd, char *args);
void cmd_bantype(struct command *cmd, char *args);
void cmd_deop(struct command *cmd, char *args);
void cmd_deoper(struct command *cmd, char *args);
void cmd_op(struct command *cmd, char *args);
void cmd_oper(struct command *cmd, char *args);
void cmd_umode(struct command *cmd, char *args);
void cmd_unkey(struct command *cmd, char *args);
void cmd_kill(struct command *cmd, char *args);

#endif
