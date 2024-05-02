/*****************************************************************************/
/*                           CSC209-24s A3 CSCSHELL                          */
/*       Copyright 2024 -- Demetres Kostas PhD (aka Darlene Heliokinde)      */
/*                                                                           */
/*       Functions implemented by Kevin Hu: execute_line, run_command,       */
/*       run_script, free_command                                            */
/*****************************************************************************/

#include "cscshell.h"


// COMPLETE
int cd_cscshell(const char *target_dir){
    if (target_dir == NULL) {
        char user_buff[MAX_USER_BUF];
        if (getlogin_r(user_buff, MAX_USER_BUF) != 0) {
           perror("run_command");
           return -1;
        }
        struct passwd *pw_data = getpwnam((char *)user_buff);
        if (pw_data == NULL) {
           perror("run_command");
           return -1;
        }
        target_dir = pw_data->pw_dir;
    }

    if(chdir(target_dir) < 0){
        perror("cd_cscshell");
        return -1;
    }
    return 0;
}


int *execute_line(Command *head){
    #ifdef DEBUG
    printf("\n***********************\n");
    printf("BEGIN: Executing line...\n");
    #endif
    Command *og_head = head;  // store the head for later since we are mutating it from the traversal

    if (head == NULL) {
        return NULL;
    }

    if (strcmp(head->exec_path, CD) == 0) {
        int *ret = malloc(sizeof(int));
        *ret = cd_cscshell(head->args[1]);
        return ret;
    }

    while (head != NULL) {  // loop over each command and call run_command on each of them
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            exit(1);
        }

        if (head->next != NULL) {
            Command *command_1 = head;
            Command *command_2 = head->next;
            command_2->stdin_fd = fd[0];
            command_1->stdout_fd = fd[1];
        }

        int *ret = malloc(sizeof(int));
        *ret = run_command(head);
        if (*ret < 0) {
            int *return_value = malloc(sizeof(int));
            *return_value = -1;
            return return_value;
        }

        head = head->next;
        free(ret);
    }

    #ifdef DEBUG
    printf("All children created\n");
    #endif

    // Wait for every children to finish
    int *exit_code_ptr = malloc(sizeof(int));
    while (og_head != NULL) {
        int status;
        int ret = wait(&status);
        if (ret < 0) {
            perror("wait");
            int *return_value = malloc(sizeof(int));
            *return_value = -1;
            return return_value;
        }

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            *exit_code_ptr = exit_code;
        }

        og_head = og_head->next;
    }

    #ifdef DEBUG
    printf("All children finished\n");
    #endif

    #ifdef DEBUG
    printf("END: Executing line...\n");
    printf("***********************\n\n");
    #endif

    return exit_code_ptr;
}


/*
** Forks a new process and execs the command
** making sure all file descriptors are set up correctly.
**
** Parent process returns -1 on error.
** Any child processes should not return.
*/
int run_command(Command *command){
    #ifdef DEBUG
    printf("Running command: %s\n", command->exec_path);
    printf("Argvs: ");
    if (command->args == NULL){
        printf("NULL\n");
    }
    else if (command->args[0] == NULL){
        printf("Empty\n");
    }
    else {
        for (int i=0; command->args[i] != NULL; i++){
            printf("%d: [%s] ", i+1, command->args[i]);
        }
    }
    printf("\n");
    printf("Redir out: %s\n Redir in: %s\n",
           command->redir_out_path, command->redir_in_path);
    printf("Stdin fd: %d | Stdout fd: %d\n",
           command->stdin_fd, command->stdout_fd);
    #endif

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } 
    else if (pid == 0) {  // setup file descriptors in the child process
        if (command->stdout_fd != STDOUT_FILENO) {
            if (dup2(command->stdout_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }

            close(command->stdout_fd);  // Close original stdout_fd
        }

        if (command->stdin_fd != STDIN_FILENO) {
            if (dup2(command->stdin_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }

            close(command->stdin_fd);  // Close original stdin_fd
        }

        if (command->redir_in_path != NULL) {
            FILE *file = fopen(command->redir_in_path, "r");
            if (file == NULL) {
                perror("fopen");
                exit(1);
            }
            
            int fd = fileno(file);
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }

            fclose(file);
        }

        if (command->redir_out_path != NULL) {
            int flags;
            if (command->redir_append) {
                flags = O_WRONLY | O_CREAT | O_APPEND;
            } 
            else {
                flags = O_WRONLY | O_CREAT | O_TRUNC;
            }

            int fd = open(command->redir_out_path, flags, 0644);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd);
        }

        if (execvp(command->exec_path, command->args) == -1) {
            perror("execvp");
            exit(1);
        }

        exit(0);
    }

    if (command->stdin_fd != STDIN_FILENO) {
        close(command->stdin_fd);
    }

    if (command->stdout_fd != STDOUT_FILENO) {
        close(command->stdout_fd);
    }   

    return pid;

    #ifdef DEBUG
    printf("Parent process created child PID [%d] for %s\n", pid, command->exec_path);
    #endif
    return -1;
}


int run_script(char *file_path, Variable **root){
    char line[MAX_SINGLE_LINE];

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    while (fgets(line, MAX_SINGLE_LINE, file) != NULL) {
        Command *commands = parse_line(line, root);
        if (commands == (Command *) -1){
            ERR_PRINT(ERR_PARSING_LINE);
            continue;
        }
        if (commands == NULL) continue;

        int *last_ret_code_pt = execute_line(commands);
        if (last_ret_code_pt == (int *) -1){
            ERR_PRINT(ERR_EXECUTE_LINE);
            free(last_ret_code_pt);
            return -1;
        }
        free(last_ret_code_pt);

        line[0] = '\0';
    }

    fclose(file);
    return 0;
}

void free_command(Command *command){
    Command *prev = NULL;
    while (command != NULL) {
        free(command->exec_path);
        if (command->redir_in_path != NULL) {
            free(command->redir_in_path);
        }
        if (command->redir_out_path != NULL) {
            free(command->redir_out_path);
        }
        int i = 0;
        while ((command->args)[i] != NULL) {
            free((command->args)[i]);
            i++;
        }
        prev = command;
        command = command->next;
        free(prev);
    } 
}
