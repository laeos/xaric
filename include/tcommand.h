#ifndef tcommand_h__
#define tcommand_h__

#define C_AMBIG (void *)(-1)

struct command
{
	char *name;
	char *rname;

	void *data;
	void (*fcn)(struct command *cmd, char *args);

	char *qhelp;
};

struct tnode *t_insert(struct tnode *p, char *string, struct command *data);
struct tnode *t_remove(struct tnode *root, char *s, struct command **data);
struct command *t_search(struct tnode *root, char *s);
struct tnode *t_build(struct tnode *root, struct command c[], int n);
void t_traverse(struct tnode * p, char *s, void (*fcn)(struct command *, char *), char *data);
void init_commands (void);



void t_parse_command(char *command, char *line);
void t_bind_exec(struct command *cmd, char *line);
int t_bind(char *string, char *command, char *args);
int t_unbind(char *command);


#endif


