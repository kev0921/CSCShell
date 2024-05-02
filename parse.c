/*****************************************************************************/
/*                           CSC209-24s A3 CSCSHELL                          */
/*       Copyright 2024 -- Demetres Kostas PhD (aka Darlene Heliokinde)      */
/*                                                                           */
/*       Functions implemented by Kevin Hu: is_variable_assignment, get_args,*/
/*       is_alphabetical, non_whitespace, is_whitespace, parse_line,         */
/*       replace_variables_mk_line, free_variable                            */
/*****************************************************************************/

#include "cscshell.h"
#include <stdbool.h> // I added this
#include <ctype.h> // I added this

#define CONTINUE_SEARCH NULL


// COMPLETE
char *resolve_executable(const char *command_name, Variable *path){

    if (command_name == NULL || path == NULL){
        return NULL;
    }

    if (strcmp(command_name, CD) == 0){
        return strdup(CD);
    }

    if (strcmp(path->name, PATH_VAR_NAME) != 0){
        ERR_PRINT(ERR_NOT_PATH);
        return NULL;
    }

    char *exec_path = NULL;

    if (strchr(command_name, '/')){
        exec_path = strdup(command_name);
        if (exec_path == NULL){
            perror("resolve_executable");
            return NULL;
        }
        return exec_path;
    }

    // we create a duplicate so that we can mess it up with strtok
    char *path_to_toke = strdup(path->value);
    if (path_to_toke == NULL){
        perror("resolve_executable");
        return NULL;
    }
    char *current_path = strtok(path_to_toke, ":");

    do {
        DIR *dir = opendir(current_path);
        if (dir == NULL){
            ERR_PRINT(ERR_BAD_PATH, current_path);
            // closedir(dir);
            continue;
        }

        struct dirent *possible_file;

        while (exec_path == NULL) {
            // rare case where we should do this -- see: man readdir
            errno = 0;
            possible_file = readdir(dir);
            if (possible_file == NULL) {
                if (errno > 0){
                    perror("resolve_executable");
                    closedir(dir);
                    goto res_ex_cleanup;
                }
                // end of files, break
                break;
            }

            if (strcmp(possible_file->d_name, command_name) == 0){
                // +1 null term, +1 possible missing '/'
                size_t buflen = strlen(current_path) +
                    strlen(command_name) + 1 + 1;
                exec_path = (char *) malloc(buflen);
                // also sets remaining buf to 0
                strncpy(exec_path, current_path, buflen);
                if (current_path[strlen(current_path)-1] != '/'){
                    strncat(exec_path, "/", 2);
                }
                strncat(exec_path, command_name, strlen(command_name)+1);
            }
        }
        closedir(dir);

        // if this isn't null, stop checking paths
        if (possible_file) break;

    } while ((current_path = strtok(CONTINUE_SEARCH, ":")));

res_ex_cleanup:
    free(path_to_toke);
    return exec_path;
}

// returns true if line is a variable assignment and false otherwise
bool is_variable_assignment(const char *line) {
    if (strchr(line, '=') == NULL || strchr(line, '=') == line) {
        return false;
    }
    
    const char *equals_location = strchr(line, '=');
    int var_length = equals_location - line;
    if (var_length == 0) {
        return false;
    }
    
    if (strlen(equals_location + 1) == 0) {
        return false;
    }

    return true;
}

// Given the line, num_args, path, and new_command_ptr, return the necessary args of new_command_ptr and given proper redirection and append values
char **get_args(char *line, int num_args, Variable *path, Command *new_command_ptr) {
    char **args = malloc(((num_args + 1) * sizeof(char*)));  // + 1 to make space for the NULL arg
    if (args == NULL) {
        perror("get_args");
        return NULL;
    }

    char *s_ptr;
    char *token = strtok_r(line, " ", &s_ptr); // token will hold the command name since command name is always first
    char *command_name = malloc(strlen(token) + 1);
    if (command_name == NULL) {
        perror("get_args");
        return NULL;
    }
    strncpy(command_name, token, strlen(token) + 1);
    command_name[strlen(token)] = '\0';
    char *exec_path = resolve_executable(token, path);
    if (exec_path == NULL) {
        args[0] = (char*) -1;
        args[1] = command_name;
        return args;
    }

    token = strtok_r(NULL, " ", &s_ptr);

    num_args = 1;
    args[0] = exec_path;
    if (args[0] == NULL) {
        free(args);
        perror("get_args");
        return NULL;
    }

    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " ", &s_ptr);
            new_command_ptr->redir_out_path = strdup(token);
            if (new_command_ptr->redir_out_path == NULL) {
                free(new_command_ptr);
                perror("get_args");
                return NULL;
            }
        }
        else if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " ", &s_ptr);
            new_command_ptr->redir_in_path = strdup(token);
            if (new_command_ptr->redir_in_path == NULL) {
                free(new_command_ptr->redir_out_path);
                free(new_command_ptr);
                perror("get_args");
                return NULL;
            }
        }
        else if (strcmp(token, ">>") == 0) {
            token = strtok_r(NULL, " ", &s_ptr);
            new_command_ptr->redir_out_path = strdup(token);
            if (new_command_ptr->redir_out_path == NULL) {
                free(new_command_ptr->redir_in_path);
                free(new_command_ptr);
                perror("get_args");
                return NULL;
            }
            new_command_ptr->redir_append = 1;
        }
        else {
            args[num_args] = strdup(token);
            if (args[num_args] == NULL) {
                int i = 0;
                for (i = 0; i < num_args - 1; i++) {
                    free(args[i]);
                }
                free(args);
                perror("get_args");
                return NULL;
            }
            num_args++;
        }

        token = strtok_r(NULL, " ", &s_ptr);
    }
    
    args[num_args] = NULL;  // create a NULL arg at the end of args
    free(command_name);
    return args;
}

// Checks if line contains only alphabetical characters or '_'
bool is_alphabetical(const char *line) {
    while (*line != '\0') {
        if (!isalpha(*line) && *line != '_') {
            return false;
        }
        line++;
    }
    return true;
}

// Returns the first non white space char in line
char non_whitespace(const char *line) {
    while (*line != '\0') {
        if (!isspace(*line)) {
            return *line;
        }
        line++;
    }
    return '\0';
}

// Checks if line contains only white space
bool is_whitespace(const char *line) {
    while (*line) {
        if (!isspace(*line)) {
            return false;
        }
        line++;
    }
    return true;
}

Command *parse_line(char *line, Variable **variables){
    Command *commands;

    // find variable assignments
    if (is_variable_assignment(line)) {
        char *s_ptr;
        char *token = strtok_r(line, "=", &s_ptr);
        char *name = strdup(token);
        if (name == NULL) {
            perror("parse_line");
            return (Command *) -1;
        }
        token = strtok_r(NULL, "=", &s_ptr);
        char *value = strdup(token);
        if (value == NULL) {
            perror("parse_line");
            return (Command *) -1;
        }

        if (!is_alphabetical(line)) {
            ERR_PRINT(ERR_VAR_NAME, name);
            return (Command *) -1;
        }

        if (line[0] == '=') {
            ERR_PRINT(ERR_VAR_NAME, name);
            return (Command *) -1;
        }

        // traverse the variable linked list
        Variable *curr_var = *variables;
        bool var_in_list = false; // keep track of whether new_var is already in the variable linked list or not
        if (curr_var != NULL) {
            Variable *new_var = malloc(sizeof(Variable));
            if (new_var == NULL) {
                perror("parse_line");
                return (Command *) -1;
            }

            new_var->next = NULL;
            char *line_cpy = strdup(line);
            if (line_cpy == NULL) {
                perror("parse_line");
                return (Command *) -1;
            }
            if (strstr(line_cpy, "PATH=") != NULL) { 
                if (strcmp((*variables)->name, "PATH") == 0) {
                    curr_var->value = value;
                }
                else {
                    new_var->name = name; 
                    new_var->value = value;
                    new_var->next = *variables;
                    *variables = new_var;
                }
            }
            else {
                while (curr_var->next != NULL) {
                    if (strcmp(curr_var->name, name) == 0) {
                        curr_var->value = value;
                        var_in_list = true;
                        break;
                    }
                    curr_var = curr_var->next;
                }
                if (!var_in_list) {  // append new_var to the end of variables
                    if (strcmp(curr_var->name, name) == 0) {
                        curr_var->value = value;
                        var_in_list = true;
                        return NULL;
                    }   
                    new_var->name = name; 
                    new_var->value = value;
                    curr_var->next = new_var;
                }
            }

            free(line_cpy);
        }
        else { // for empty variables linked list
            Variable *new_var = malloc(sizeof(Variable));
            if (new_var == NULL) {
                perror("parse_line");
                return (Command *) -1;
            }
            new_var->next = NULL;
            new_var->name = name; 
            new_var->value = value;
            *variables = new_var;
        }

        return NULL;
    }

    // find comments and blank lines 
    else if (non_whitespace(line) == '#' || strlen(line) == 0 || is_whitespace(line)) {
        return NULL;
    }

    // find the cd command 
    else if (strstr(line, "cd ") != NULL) {
        if (strchr(line, '$') != NULL) {
            line = replace_variables_mk_line(line, *variables);
            if (line == (char *) -1 || line == NULL) {
                return (Command *) -1;
            }
        }

        int i = 0;
        Variable *path = *variables;  // returns the PATH variable
        char *exec_path = resolve_executable("cd", path);
        Command *new_command_ptr = malloc(sizeof(Command));
        if (new_command_ptr == NULL) {
            perror("parse_line");;
            return (Command *) -1;
        }

        new_command_ptr->exec_path = exec_path;
        i = 3;
        if (*(strstr(line, "cd ") + 3) == ' ') { // if there are no args in the cd command
            new_command_ptr->args = malloc(sizeof(char *));
            if (new_command_ptr->args == NULL) {
                free(new_command_ptr->exec_path);
                free(new_command_ptr);
                perror("parse_line");
                return (Command *) -1;
            }

            new_command_ptr->args[0] = exec_path;
            new_command_ptr->args[1] = NULL;
        }
        else {  // if there is one arg in the cd command
            new_command_ptr->args = malloc(sizeof(char *) * 2);
            if (new_command_ptr->args == NULL) {
                free(new_command_ptr);
                perror("parse_line");
                return (Command *) -1;
            }
            new_command_ptr->args[1] = malloc(sizeof(char) * (strlen(line) + 1));
            if (new_command_ptr->args[1] == NULL) {
                free(new_command_ptr->args);
                free(new_command_ptr);
                perror("parse_line");
                return (Command *) -1;
            }

            new_command_ptr->args[0] = exec_path;

            i = 3;
            char *arg = malloc((strlen(line) * sizeof(char)) + 1);
            if (arg == NULL) {
                perror("parse_line");
                return (Command *) -1;
            }

            int command_arg_len = 0;
            while (line[i] != ' ' && line[i] != '\0') {
                arg[command_arg_len] = line[i];
                command_arg_len++;
                i++;
            }

            strncpy(new_command_ptr->args[1], arg, command_arg_len);
            new_command_ptr->args[1][command_arg_len] = '\0';
            free(arg);
        }

        new_command_ptr->next = NULL;
        new_command_ptr->stdin_fd = STDIN_FILENO;
        new_command_ptr->stdout_fd = STDOUT_FILENO;
        new_command_ptr->redir_in_path = NULL;
        new_command_ptr->redir_out_path = NULL;
        new_command_ptr->redir_append = 0; 

        commands = new_command_ptr;
    }

    // find commands other than cd
    else {
        if (strchr(line, '$') != NULL) {
            line = replace_variables_mk_line(line, *variables);
            if (line == (char *) -1 || line == NULL) {
                return (Command *) -1;
            }
        }

        // split up line if there are any | operators
        int i = 0;
        int count = 0;
        if (strchr(line, '|') != NULL) {
            int i = 0;
            while (line[i] != '\0') {
                if (line[i] == '|') {
                    count += 1;
                }
                i++;
            }
        }
        char **line_split = malloc((count + 1) * sizeof(char*)); // this list of strings will store the lines separated by the | operator
        if (line_split == NULL) {
            perror("parse_line");
            return (Command *) -1;
        }
        int k = 0;  // the char index for line
        for (i = 0; i < count + 1; i++) {
            char *section = malloc(strlen(line) + 1);
            if (section == NULL) {
                perror("parse_line");
                return (Command *) -1;
            }

            int section_len = 0;

            while (line[k] == ' ') { // skip leading white space
                k++;
            }

            while(line[k] != '|' && line[k] != '\0') { 
                section[section_len] = line[k];
                k++;
                section_len++;
            }
            k++; // skip the | operator
            
            if (section[section_len - 1] != ' ') {
                char *new_section = malloc(section_len + 1); // create the new_section without extra null terminators
                strncpy(new_section, section, section_len + 1);
                new_section[section_len] = '\0';
                line_split[i] = strdup(new_section);
                if (line_split[i] == NULL) {
                    int j = 0;
                    for (j = 0; j < i; j++) {
                        free(line_split[j]);
                    }
                    free(line_split);
                    perror("parse_line");
                    return (Command *) -1;
                }

                free(new_section);
            }
            else { // eliminate the space before the pipe
                char *new_section = malloc(section_len); // create the new_section without extra null terminators
                strncpy(new_section, section, section_len);
                new_section[section_len - 1] = '\0';
                line_split[i] = strdup(new_section);
                if (line_split[i] == NULL) {
                    int j = 0;
                    for (j = 0; j < i; j++) {
                        free(line_split[j]);
                    }
                    free(line_split);
                    perror("parse_line");
                    return (Command *) -1;
                }

                free(new_section);
            }

            free(section);
        }

        commands = NULL;
        Command *og_head;
        for (i = 0; i < count + 1; i++) {
            Command *new_command_ptr = malloc(sizeof(Command));
            if (new_command_ptr == NULL) {
                return NULL;
            }

            char *str_section = line_split[i];
            while (*str_section == ' ') { // skip leading white space
                str_section++;
            }

            int num_args = 0; // the exec path will always be the first argument
            while (*str_section != '\0') { // find the number of args
                if (*str_section == ' ') {
                    num_args++;
                }
                str_section++;
            }
            num_args++; // add 1 to get the actual number of arguments since the # of whitespace is one less than the # of args

            // Create the Command object
            Variable *path = *variables;  // the PATH variable is always at the head of variables

            new_command_ptr->redir_in_path = NULL;
            new_command_ptr->redir_out_path = NULL;
            new_command_ptr->redir_append = 0;

            new_command_ptr->args = get_args(line_split[i], num_args, path, new_command_ptr);
            if (new_command_ptr->args[0] == (char *) -1) {
                ERR_PRINT(ERR_NO_EXECU, new_command_ptr->args[1]);  // name of command is stored in args[1]
                free(new_command_ptr->args[1]);
                return (Command *) -1;
            }

            new_command_ptr->exec_path = strdup(new_command_ptr->args[0]);
            if (new_command_ptr->exec_path == NULL) {
                free(new_command_ptr);
                perror("parse_line");;
                return (Command *) -1;
            }

            new_command_ptr->stdin_fd = STDIN_FILENO;
            new_command_ptr->stdout_fd = STDOUT_FILENO;
            new_command_ptr->next = NULL;

            if (commands == NULL) { // commands is the first node in the linked list
                commands = new_command_ptr;
                og_head = commands;
            } else { 
                while (commands->next != NULL) { // traverse to tail of linked list to add the new command
                    commands = commands->next;
                }
                commands->next = new_command_ptr;
            }

            free(line_split[i]);
        }

        commands = og_head;
        free(line_split);
    }

    return commands;
}


/*
** This function is partially implemented for you, but you may
** scrap the implementation as long as it produces the same result.
**
** Creates a new line on the heap with all named variable *usages*
** replaced with their associated values.
**
** Returns NULL if replacement parsing had an error, or (char *) -1 if
** system calls fail and the shell needs to exit.
*/
char *replace_variables_mk_line(const char *line,
                                Variable *variables){
    Variable *replacements = NULL;
    Variable *curr_replacement = replacements;
    Variable *prev_replacement = NULL;

    // 1. Figure out new line length and build the replacements linked list
    int actual_line_len = strlen(line);
    bool in_variable = false;
    int var_length = 0;
    bool has_brackets = false;
    char *curr_var_name = malloc(strlen(line) + 1);
    if (curr_var_name == NULL) {
        perror("replace_variables_mk_line");
        return NULL;
    }

    int i = 0;
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == '$') {
            in_variable = true;
        } 
        if (line[i] == '$' && line[i + 1] == '{') {
            has_brackets = true;
        }

        if (in_variable) {
            if (line[i + 1] != '}' && line[i + 1] != '{') {
                curr_var_name[var_length] = line[i + 1];
                var_length++;
                curr_var_name[var_length] = '\0';
            }
        }
        bool add_to_name = (line[i + 1] != '_' && line[i + 1] != '}' && line[i + 1] != '{' && in_variable
            && (line[i] == '}' || line[i + 1] == '\0' ||  !isalpha(line[i + 1])));
        if (add_to_name) {
            char *new_var_name = malloc(var_length);
            bool variable_exists = false;
            in_variable = false;
            memcpy(new_var_name, curr_var_name, var_length);
            new_var_name[var_length - 1] = '\0';
            var_length = 0;

            if (line[i] != '}' && has_brackets) {
                ERR_PRINT(ERR_VAR_NAME, new_var_name);
                return NULL;
            }

            if (has_brackets) {
                actual_line_len -= 3;  // account for brackets and $
                actual_line_len -= strlen(new_var_name);
            }
            else {
                actual_line_len -= 1;  // acount for only the $
                actual_line_len -= strlen(new_var_name);
            }
            has_brackets = false;

            // Look for new_var_name in variables
            Variable *curr_var = variables;
            while (curr_var != NULL && !variable_exists) {
                if (strcmp(curr_var->name, new_var_name) == 0) { // loop until we find new_var_name in variables
                    // Add a new variable into replacements
                    variable_exists = true;
                    Variable *var = malloc(sizeof(Variable));
                    if (var == NULL) {
                        perror("replace_variables_mk_line");
                        return NULL;
                    }
                    var->next = NULL;
                    var->name = strdup(new_var_name);
                    if (var->name == NULL) {
                        free(var);
                        perror("replace_variables_mk_line");
                        return NULL;
                    }
                    var->value = curr_var->value;

                    curr_replacement = var;

                    if (replacements == NULL) {
                        replacements = var;
                        prev_replacement = var;
                    }
                    else {
                        prev_replacement->next = var;
                    }
                    prev_replacement = curr_replacement;
                    curr_replacement = curr_replacement->next;
                }
                
                curr_var = curr_var->next;
            }

            if (!variable_exists) {
                ERR_PRINT(ERR_VAR_NOT_FOUND, new_var_name);
                return (char *) -1;
            }

            free(curr_var_name);
            curr_var_name = malloc(strlen(line) + 1);
            if (curr_var_name == NULL) {
                perror("replace_variables_mk_line");
                return NULL;
            }
            curr_var_name[0] = '\0';

        }

    }

    Variable *current_var = replacements;
    while (current_var != NULL) {
        actual_line_len += strlen(current_var->value);
        current_var = current_var->next;
    }

    // 2. Replace all the variables with their actual values
    char *new_line = (char *)malloc(actual_line_len + 1);  // +1 to make space for the null terminator
    if (new_line == NULL) {
        perror("replace_variables_mk_line");
        return NULL;
    }

    int line_i = 0;
    int new_line_i = 0;
    int j = 0;
    current_var = replacements; 
    while (line[line_i] != '\0') {
        if (line[line_i] == '$' && line[line_i + 1] == '{') {
            for (j = 0; j < strlen(current_var->value); j++) {
                new_line[new_line_i] = current_var->value[j];
                new_line_i++;
            }
            line_i += strlen(current_var->name) + 1;  // + 1 to skip $ 
            line_i += 2; // skip the curly brackets
            if (current_var->next != NULL) {
                current_var = current_var->next;  
            }
        }
        else if (line[line_i] == '$') {
            for (j = 0; j < strlen(current_var->value); j++) {
                new_line[new_line_i] = current_var->value[j];
                new_line_i++;
            }
            line_i += strlen(current_var->name) + 1;  // + 1 to skip $ 
            if (current_var->next != NULL) {
                current_var = current_var->next;   
            }
        }
        else {
            new_line[new_line_i] = line[line_i];
            line_i++;
            new_line_i++;
        }
    }

    new_line[new_line_i] = '\0';
    return new_line;
}

void free_variable(Variable *var, uint8_t recursive){
    Variable *prev = NULL;
    while (var != NULL) {
        free(var->name);
        free(var->value);
        prev = var;
        var = var->next;
        free(prev);
    }
}
