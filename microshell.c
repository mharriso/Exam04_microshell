#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define FD_READ 0
#define FD_WRITE 1

typedef	struct		s_list
{
	char			**argv;
	int				argc;
	int				pipe[2];
	int				write_pipe;
	int				read_pipe;
	struct s_list	*prev;
	struct s_list	*next;
}					t_list;

int					ft_strlen(const char *str)
{
	int i = 0;

	while (str[i])
		i++;
	return (i);
}

void				put_error(const char *str)
{
	write(STDERR_FILENO, str, ft_strlen(str));
}

void				exit_fatal(void)
{
	put_error("error: fatal\n");
	exit(EXIT_FAILURE);
}

t_list				*lst_new(void)
{
	t_list		*elem;

	if (!(elem = malloc(sizeof(t_list))))
		exit_fatal();
	elem->argv = NULL;
	elem->argc = 0;
	elem->write_pipe = 0;
	elem->read_pipe = 0;
	elem->prev = NULL;
	elem->next = NULL;
	return (elem);
}

t_list				*lst_get_last(t_list *lst)
{
	while (lst && lst->next)
		lst = lst->next;
	return (lst);
}

void				lst_push_back(t_list **lst, t_list *new)
{
	t_list		*last;

	if (!*lst)
	{
		*lst = new;
		return ;
	}
	last = lst_get_last(*lst);
	last->next = new;
	new->prev = last;
}

t_list				*parser(int argc, char **argv)
{
	static int		i = 1;
	t_list			*cmd_list;
	t_list			*temp;

	cmd_list = NULL;
	while (i < argc)
	{
		if (!strcmp(argv[i], ";"))
		{
			if (cmd_list)
			{
				argv[i] = NULL;
				++i;
				return (cmd_list);
			}
		}
		else if (!strcmp(argv[i], "|"))
		{
			temp = lst_get_last(cmd_list);
			temp->write_pipe = 1;
			temp = lst_new();
			temp->argv = &argv[i + 1];
			temp->read_pipe = 1;
			lst_push_back(&cmd_list, temp);
			argv[i] = NULL;
		}
		else
		{
			if (!cmd_list)
			{
				cmd_list = lst_new();
				cmd_list->argv = &argv[i];
			}
			cmd_list->argc++;
		}
		++i;
	}
	return (cmd_list);
}

void				builtin_cd(t_list *cmd)
{
	if (cmd->argc != 2)
		put_error("error: cd: bad arguments\n");
	else if (chdir(cmd->argv[1]))
	{
		put_error("error: cd: cannot change directory to ");
		put_error(cmd->argv[1]);
		put_error("\n");
	}
}

void				execute_cmd(t_list *cmd, char **envp)
{
	int			ret;
	pid_t		pid;

	if (cmd->write_pipe && pipe(cmd->pipe) < 0)
		exit_fatal();
	if ((pid = fork()) < 0)
		exit_fatal();
	if (pid == 0)
	{
		if (cmd->write_pipe && dup2(cmd->pipe[FD_WRITE], STDOUT_FILENO) < 0)
			exit_fatal();
		else if (cmd->read_pipe && dup2(cmd->prev->pipe[FD_READ], STDIN_FILENO) < 0)
			exit_fatal();
		if ((ret = execve(cmd->argv[0], cmd->argv, envp)))
		{
			put_error("error: cannot execute ");
			put_error(cmd->argv[0]);
			put_error("\n");
		}
		exit(ret);
	}
	else
	{
		waitpid(pid, NULL, 0);
		if (cmd->write_pipe && close(cmd->pipe[FD_WRITE]) < 0)
			exit_fatal();
		else if (cmd->read_pipe && close(cmd->prev->pipe[FD_READ]) < 0)
			exit_fatal();
	}
}

int					main(int argc, char **argv, char **envp)
{
	t_list		*cmd_list;

	while ((cmd_list = parser(argc, argv)))
		while (cmd_list)
		{
			if (!strcmp(cmd_list->argv[0], "cd"))
				builtin_cd(cmd_list);
			else
				execute_cmd(cmd_list, envp);
			cmd_list = cmd_list->next;
		}
	return (0);
}
