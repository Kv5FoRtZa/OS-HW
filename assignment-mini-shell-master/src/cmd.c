// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cmd.h"
#include "utils.h"

#define READ		0
#define WRITE		1

static bool shell_cd(word_t *dir)
{
	int lungime = strlen(dir->string) * 8;
	char *director = malloc(lungime * sizeof(char *));

	memcpy(director, dir->string, strlen(dir->string) + 1);
	while (dir->next_part != NULL) {
		dir = dir->next_part;
		strcat(director, dir->string);
	}
	if (chdir(director) == 0) {
		free(director);
		return 0;
	}
	free(director);
	return 1;
}


static int shell_exit(void)
{
	_exit(0);
	return 0;
}

static int parse_simple(simple_command_t *s,  int level,  command_t *father)
{
	if (s == NULL || s->verb == NULL || s->verb->string == NULL)
		return 0;
	if (strcmp(s->verb->string, "false") == 0)
		return 1;
	if (strcmp(s->verb->string, "true") == 0)
		return 0;
	if (strcmp(s->verb->string, "quit") == 0 || strcmp(s->verb->string, "exit") == 0)
		shell_exit();
	if (strcmp(s->verb->string, "cd") == 0) {
		if (s->out != NULL) {
			int saved_stdout = dup(STDOUT_FILENO);
			word_t *out = s->out;
			int lung = strlen(out->string) * 8;
			char *fisier = malloc(lung * sizeof(char));

			memcpy(fisier, out->string, strlen(out->string) + 1);
			while (out->next_part != NULL) {
				out = out->next_part;
				strcat(fisier, out->string);
			}
			int flags = O_WRONLY | O_CREAT;

			if (s->io_flags & IO_OUT_APPEND)
				flags |= O_APPEND;
			else
				flags |= O_TRUNC;
			int file = open(fisier, flags, 0644);

			dup2(file, STDOUT_FILENO);
			close(file);
			dup2(saved_stdout,  STDOUT_FILENO);
			close(saved_stdout);
			free(fisier);
		}
		if (s->params == NULL) {
			char *home = getenv("HOME");

			chdir(home);
			return 0;
		} else {
			return shell_cd(s->params);
		}
	} else if ((s->verb->next_part != NULL) && (s->verb->next_part->string != NULL) &&
			strcmp(s->verb->next_part->string, "=") == 0) {
		//env set
		int lungime_dalloc1 = strlen(s->verb->string) * 8;
		int lungime_dalloc2 = strlen(s->verb->next_part->next_part->string) * 8;
		char *part1 = malloc(lungime_dalloc1 * sizeof(char *));
		char *part2 = malloc(lungime_dalloc2 * sizeof(char *));
		word_t *de_bagat = s->verb->next_part->next_part;

		memcpy(part1, s->verb->string, strlen(s->verb->string) + 1);
		if (de_bagat->expand == true) {
			char *value = getenv(de_bagat->string);

			if (value != NULL)
				memcpy(part2, value, strlen(value) + 1);
		} else {
			memcpy(part2, de_bagat->string, strlen(de_bagat->string) + 1);
		}
		while (de_bagat->next_part != NULL) {
			de_bagat = de_bagat->next_part;
			if (de_bagat->expand == true) {
				char *value = getenv(de_bagat->string);

				if (value != NULL)
					strcat(part2, value);
		} else {
			strcat(part2, de_bagat->string);
		}
		}
		setenv(part1, part2, 1);
		//free(part1);
		//free(part2);
	} else {
		pid_t pid = fork();

		if (pid == 0) {
			int lungime_args = strlen(s->verb->string) * 8;
			char **args = malloc((lungime_args) * sizeof(char *));
			int cnt = 1;
			word_t *secundar = s->params;

			args[0] = (char *)s->verb->string;
			while (secundar != NULL) {
				int lungime_alocare = strlen(secundar->string) * 8;
				char *parametrii = malloc(lungime_alocare * sizeof(char *));

				if (secundar->expand == true) {
					char *value = getenv(secundar->string);

					if (value != NULL)
						memcpy(parametrii, value, strlen(value) + 1);
				} else {
					memcpy(parametrii, secundar->string, strlen(secundar->string) + 1);
				}
				word_t *tertiar = secundar;

				while (tertiar->next_part != NULL) {
					tertiar = tertiar->next_part;
					if (tertiar->expand == true && getenv(tertiar->string) != NULL)
						strcat(parametrii, getenv(tertiar->string));
					else
						strcat(parametrii, tertiar->string);
				}
				secundar = secundar->next_word;
				args[cnt] = parametrii;
				cnt++;
				////free(parametrii);
			}
			args[cnt] = NULL;
			if (s->out != NULL && s->err != NULL) {
				if (s->out == s->err) {
					word_t *out = s->out;
					int lung = strlen(out->string) * 8;
					char *fisier = malloc(lung * sizeof(char));

					if (out->expand == true && getenv(out->string) != NULL)
						memcpy(fisier, getenv(out->string), strlen(getenv(out->string)) + 1);
					else
						memcpy(fisier, out->string, strlen(out->string) + 1);
					while (out->next_part != NULL) {
						out = out->next_part;
						if (out->expand == true && getenv(out->string) != NULL)
							strcat(fisier, getenv(out->string));
						else
							strcat(fisier, out->string);
					}
					int flags = O_WRONLY | O_CREAT;

					if ((s->io_flags & IO_OUT_APPEND) || (s->io_flags & IO_ERR_APPEND))
						flags |= O_APPEND;
					else
						flags |= O_TRUNC;
					int file = open(fisier, flags, 0644);

					dup2(file, STDOUT_FILENO);
					dup2(file, STDERR_FILENO);
					close(file);
					//free(fisier);
				} else {
					word_t *out = s->out;
					int lung = strlen(out->string) * 8;
					char *fisier = malloc(lung * sizeof(char));
					char *value3 = NULL;

					if (out->expand == true)
						value3 = getenv(out->string);
					if (value3 != NULL)
						memcpy(fisier, value3, strlen(value3) + 1);
					else
						memcpy(fisier, out->string, strlen(out->string) + 1);
					while (out->next_part != NULL) {
						out = out->next_part;
						if (!out->expand) {
							strcat(fisier, out->string);
							continue;
						}
						char *value = getenv(out->string);

						if (value != NULL)
							strcat(fisier, value);
					}
					int flags = O_WRONLY | O_CREAT;

					if (s->io_flags & IO_OUT_APPEND)
						flags |= O_APPEND;
					else
						flags |= O_TRUNC;
					int file = open(fisier, flags, 0644);

					dup2(file, STDOUT_FILENO);
					close(file);
					//
					word_t *err = s->err;
					int lung2 = strlen(err->string) * 8;
					char *fisier2 = malloc(lung2 * sizeof(char));
					char *value = NULL;

					if (err->expand == true)
						value = getenv(err->string);
					if (value != NULL)
						memcpy(fisier2, value, strlen(value) + 1);
					else
						memcpy(fisier2, err->string, strlen(err->string) + 1);
					while (err->next_part != NULL) {
						err = err->next_part;
						char *value = NULL;

					if (err->expand == true)
						value = getenv(err->string);
					if (value != NULL)
						strcat(fisier, value);
					else
						strcat(fisier, err->string);
					}
					int flags2 = O_WRONLY | O_CREAT;

					if (s->io_flags & IO_ERR_APPEND)
						flags2 |= O_APPEND;
					else
						flags2 |= O_TRUNC;
					int file2 = open(fisier2, flags2, 0644);

					dup2(file2, STDERR_FILENO);
					close(file2);
					//free(fisier);
					//free(fisier2);
				}
			} else if (s->out != NULL) {
				word_t *out = s->out;
				int lung = strlen(out->string) * 8;
				char *fisier = malloc(lung * sizeof(char));

				if (out->expand == true)	{
					char *value = getenv(out->string);

					if (value != NULL)
						memcpy(fisier, value, strlen(value) + 1);
				} else {
					memcpy(fisier, out->string, strlen(out->string) + 1);
				}
				while (out->next_part != NULL) {
					out = out->next_part;
					if (!out->expand) {
						strcat(fisier, out->string);
						continue;
					}
					char *value = getenv(out->string);

					if (value != NULL)
						strcat(fisier, value);
				}
				int flags = O_WRONLY | O_CREAT;

				if (s->io_flags & IO_OUT_APPEND)
					flags |= O_APPEND;
				else
					flags |= O_TRUNC;
				int file = open(fisier, flags, 0644);

				dup2(file, STDOUT_FILENO);
				close(file);
				//free(fisier);
			} else if (s->err != NULL) {
				word_t *err = s->err;
				int lung = strlen(err->string) * 8;
				char *fisier = malloc(lung * sizeof(char));

				if (err->expand == true) {
					char *value = getenv(err->string);

					if (value != NULL)
						memcpy(fisier, value, strlen(value) + 1);
				} else {
					memcpy(fisier, err->string, strlen(err->string) + 1);
				}
				while (err->next_part != NULL) {
					err = err->next_part;
					char *value = NULL;

					if (err->expand == true)
						value = getenv(err->string);
					if (value != NULL)
						strcat(fisier, value);
					else
						strcat(fisier, err->string);
				}
				int flags = O_WRONLY | O_CREAT;

				if (s->io_flags & IO_ERR_APPEND)
					flags |= O_APPEND;
				else
					flags |= O_TRUNC;
				int file = open(fisier, flags, 0644);

				dup2(file, STDERR_FILENO);
				close(file);
				//free(fisier);
			}
			if (s->in != NULL) {
				word_t *in = s->in;
				int lung = strlen(in->string) * 8;
				char *fisier = malloc(lung * sizeof(char));

				if (in->expand == true)	{
					char *value = getenv(in->string);

					if (value != NULL)
						memcpy(fisier, value, strlen(value) + 1);
				} else {
					memcpy(fisier, in->string, strlen(in->string) + 1);
				}
				while (in->next_part != NULL) {
					in = in->next_part;
					char *value = NULL;

					if (in->expand == true)
						value = getenv(in->string);
					if (value != NULL)
						strcat(fisier, value);
					else
						strcat(fisier, in->string);
				}
				int file = open(fisier, O_RDONLY);

				dup2(file, STDIN_FILENO);
				close(file);
				//free(fisier);
			}
			int test = execvp(args[0], args);

			if (test == -1) {
				printf("Execution failed for '%s'\n", args[0]);
				//free(args);
				exit(1);
			}
			//free(args);
		} else {
			int status;

			waitpid(pid, &status, 0);
			if (WIFEXITED(status))
				return WEXITSTATUS(status);
		}
	}
	return 0;
}
static bool run_in_parallel(command_t *cmd1,  command_t *cmd2,  int level,
		command_t *father)
{
	int pid1 = fork();

	if (pid1 == 0) {
		int status = parse_command(cmd1, 0, father);

		exit(status);
	}
	int pid2 = fork();

	if (pid2 == 0) {
		int status = parse_command(cmd2, 0, father);

		exit(status);
	}
	int status1, status2;

	waitpid(pid1, &status1, 0);
	waitpid(pid2, &status2, 0);
	return true;
}

static bool run_on_pipe(command_t *cmd1,  command_t *cmd2,  int level,
		command_t *father)
{
	int fd[2];

	pipe(fd);
	int left = fork();

	if (left == 0) {
		dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		int status = parse_command(cmd1, 0, father);

		exit(status);
	}

	int right = fork();

	if (right == 0) {
		dup2(fd[0], STDIN_FILENO);
		close(fd[1]);
		close(fd[0]);
		int status = parse_command(cmd2, 0, father);

		exit(status);
	}

	close(fd[0]);
	close(fd[1]);
	int status;

	waitpid(right, &status, 0);
	waitpid(left, NULL, 0);
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return true;
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c,  int level,  command_t *father)
{
	//while (waitpid(-1, NULL, WNOHANG) > 0)
	//;
	int prima;

	if (c->op == OP_NONE)
		return parse_simple(c->scmd, 0, father);

	switch (c->op) {
	case OP_SEQUENTIAL:
		parse_command(c->cmd1, 0, father);
		return parse_command(c->cmd2, 0, father);

	case OP_PARALLEL:
		return run_in_parallel(c->cmd1, c->cmd2, 0, father);

	case OP_CONDITIONAL_NZERO:
		prima = parse_command(c->cmd1, 0, father);
		if (prima != 0)
			return parse_command(c->cmd2, 0, father);
		return prima;

	case OP_CONDITIONAL_ZERO:
		prima = parse_command(c->cmd1, 0, father);
		if (prima == 0)
			return parse_command(c->cmd2, 0, father);
		return prima;

	case OP_PIPE:
		return run_on_pipe(c->cmd1, c->cmd2, 0, father);

	default:
		return SHELL_EXIT;
	}

	return 0;/* TODO: Replace with actual exit code of command. */
}
