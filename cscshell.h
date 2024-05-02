/*****************************************************************************/
/*                     CSCSHELL -- CSC209 A3 Winter 2024                     */
/*                   Copyright 2024 -- Demetres Kostas PhD                   */
/*                  ----------------------------------------                 */
/*                    See also: cscshell.c, parse.c, run.c                   */
/*****************************************************************************/


#ifndef CSCSHELL_H
#define CSCSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <dirent.h>
#include <pwd.h>
#include <errno.h>

// Arg help
#define LONG_HELP_ARG "--help"
#define LONG_INIT_ARG "--init-file="
#define DEFAULT_INIT "~/.cscshell_init"

// Buffer sizes
#define MAX_USER_BUF 128
#define MAX_PATH_STR 4096
#define MAX_SINGLE_LINE 4096

// Prompt config
#define PROMPT_STR "<:"

// other strings and values
#define PATH_VAR_NAME "PATH"
#define CD "cd"
#define VARIABLE_PARSE_MARKER '$'
#define PARSING_START_MARKER '<'
#define PARSING_END_MARKER '>'
#define NON_ZERO_BYTE 0x42

// Error Strings
#define ERR_ARGS_MISSING "Missing init file path after argument: '-i'\n"
#define ERR_PATH_INIT "PATH not defined in init file %s, or not at the head \
of the variable list."
#define ERR_PARSING_LINE "Could not parse line into commands.\n"
#define ERR_EXECUTE_LINE "Could not execute line.\n"
#define ERR_INIT_SCRIPT "Failed to run init script: %s\n"
#define ERR_VAR_START "Assignment cannot start with '=' character.\n"
#define ERR_VAR_NAME "Variable names must only contain alphabetic characters and\
 '_' chars.\n Got: %s\n"
#define ERR_NOT_PATH "Variable used for PATH is not correctly named.\n"
#define ERR_BAD_PATH "PATH directory %s invalid.\n"
#define ERR_NO_EXECU "Could not resolve executable [%s]\n"
#define ERR_VAR_USAGE "Variable could not be parsed from %s\n"
#define ERR_VAR_NOT_FOUND "Could not find variable: <%s>\n"

#define ERR_PRINT(...) fprintf(stderr, "ERROR: ");\
    fprintf(stderr, __VA_ARGS__);

/*
** Two structures for maintaining a singly-linked list of:
**
** 1. Shell Variables; including PATH, which is the
**    first item of main variable list. Consider using
**    this list structure for replacing variables used
**    in a shell command as well.
** 2. Commands to execute; A single line may have only a
**    single command, or may consist of multiple commands
**    connected by pipes.
*/
typedef struct Variable{
    char *name;
    char *value;
    struct Variable *next;
} Variable;

typedef struct Command {
    char *exec_path;
    char **args;
    struct Command *next;
    uint32_t stdin_fd;
    uint32_t stdout_fd;
    char *redir_in_path;
    char *redir_out_path;
    uint8_t redir_append;
} Command;


/*
** The following functions are provided for you in _shell.c
** You should modify them as needed, but do *not* change their signatures
**
** If they are marked as "COMPLETE", do not change them!
*/

/*
** Parses a single line of text and returns a linked list of commands.
** The last command in the list has a next pointer that points to NULL.
**
** Return possibilities:
** 1. The first in a list of commands that should execute roughly
**    simultaneously (see instructions for details).
**
** 2. NULL if successfully parsed line, with *no commands*, this happens:
**    -- Case 1: Empty line
**    -- Case 2: Line is /exclusively/ a comment (i.e. first non-whitespace
**               char is '#'). Comments may also trail commands or assignments.
**               You must handle text before '#' characters.
**    -- Case 3: Shell variable assignment (e.g. VAR=VALUE)
**       -- The variable should added to the variables list
**       -- or updated if the variable already exists
**
** 3. If there is an error, returns -1 cast as a (Command *)
*/
Command *parse_line(char *line, Variable **variables);

/*
** WARNING: this is a challenging string parsing task.
**
** Creates a new line on the heap with all named variable *usages*
** replaced with their associated values.
**
** Returns NULL if replacement parsing had an error, or (char *) -1 if
** system calls fail and the shell needs to exit.
*/
char *replace_variables_mk_line(const char *line,
                                Variable *variables);

/*
** This function is provided for you and should not be modified.
**
** Implements the `cd` operation for the CSCSHELL.
**
** Returns 0 on success, -1 on any error encountered.
 */
int cd_cscshell(const char *target_dir);

/*
** This function is provided for you and should not be modified.
**
** Determines the correct path of the executable for a particular command.
**
** If PATH contains non-existent directories, it prints an error to stderr
** and ignores this directory.
**
** Returns:
** -- A heap string with the first working path to the command_name
**    *if* it is not already a sort of path.
** -- Otherwise the command_name is duplicated on the heap
** -- NULL if no command could be found on the path,
**    or an error occurred.
*/
char *resolve_executable(const char *command_name, Variable *path);

/*
** Executes a single "line" of commands (through pipes)
** If a command fails, the rest of the line should not be executed.
**
** The error code from the last command is returned through a pointer
** to a heap integer on success. If the line is a `cd` command, the
** return value of `cd_cscshell` is stored by the heap int.
** -- If there are no commands to execute, returns NULL
** -- If there were any errors starting any commands,
**    returns (pointer value) -1
*/
int *execute_line(Command *head);

/*
** Forks a new process and execs the command
** making sure all file descriptors are set up correctly.
**
** Parent process returns -1 on error.
** Any child processes should not return.
*/
int run_command(Command *command);

/*
** Executes an entire script line-by-line.
** Stops and indicates an error as soon as any line fails.
**
** Returns 0 on success, -1 on error
*/
int run_script(char *file_path, Variable **root);

/*
** Implement the following function that frees all the
** heap memory associated with a particular command.
 */
void free_command(Command *command);

/*
** Implement the following function that frees variable(s).
**
** If recursive is non-zero, recursively free an entire
** list starting at var, else just var.
 */
void free_variable(Variable *var, uint8_t recursive);
#endif
