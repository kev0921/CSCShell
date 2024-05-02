/*****************************************************************************/
/*                           CSC209-24s A3 CSCSHELL                          */
/*       Copyright 2024 -- Demetres Kostas PhD (aka Darlene Heliokinde)      */
/*****************************************************************************/

#include "cscshell.h"


void print_help(){
    printf("CSC209 Shell\n");
    printf("Usage: cscshell [OPTION]... [SCRIPT-FILE]\n");
    printf("Options:\n");
    printf("  -h, --help\t\t\tDisplay this help message\n");
    printf("  -i, --init-file=FILE\t\tUse a specific init file. Default is ~/.cscshell_init\n");
    printf("If no script file is given, cscshell will run in interactive mode\n");
}


char *prompt(char *line, size_t line_length){
    char cwd_buff[MAX_PATH_STR];
    if (getcwd(cwd_buff, MAX_PATH_STR) == NULL){
        perror("prompt:");
        return (char *) -1;
    }

    char user_buff[MAX_USER_BUF];
    if (getlogin_r(user_buff, MAX_USER_BUF)){
        perror("prompt:");
        return (char *) -1;
    }

    printf("%s@<%s> %s", user_buff, cwd_buff, PROMPT_STR);
    return fgets(line, line_length, stdin);
}


int run_interactive(Variable **root){
    long error;
    char line[MAX_SINGLE_LINE];

    #ifdef DEBUG
    printf("Interactive CSCSHELL starting...\n");
    #endif

    while ((error = (long) prompt(line, MAX_SINGLE_LINE)) > 0) {
        // kill the newline
        line[strlen(line) - 1] = '\0';

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

        // int *last_ret_code_pt = execute_line(commands);
        // if (last_ret_code_pt == (int *)-1) {
        //     ERR_PRINT(ERR_EXECUTE_LINE);
        //     continue;
        // }
        // free(last_ret_code_pt);
    }
    printf("\n");

    #ifdef DEBUG
    printf("\nInteractive CSCSHELL exiting...\n");
    #endif

    // 0 on EOF, -1 on other errors
    return (int) error;
}


int main(int argc, char *argv[]){

    int num_args_parsed = 0;
    char *init_file = DEFAULT_INIT;

    for (int i=1; i < argc; i++){
        if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], LONG_HELP_ARG) == 0){
            print_help();
            return 0;
        }

        if (strcmp(argv[i], "-i") == 0){
            if (i + 1 < argc){
                init_file = argv[i + 1];
                i++;
                num_args_parsed += 2;
            }
            else{
                fprintf(stderr, ERR_ARGS_MISSING);
                return -1;
            }
        }

        else if (strncmp(argv[1], LONG_INIT_ARG,
                         strlen(LONG_INIT_ARG)) == 0){
            num_args_parsed++;
            init_file = strchr(argv[1], '=') + 1;
        }
    }

    #ifdef DEBUG
    printf("Using init file at: %s\n", init_file);
    #endif

    Variable *start_of_vars = NULL;
    if (run_script(init_file, &start_of_vars) < 0){
        ERR_PRINT(ERR_INIT_SCRIPT, init_file);
        return -1;
    }

    if ((start_of_vars == NULL) ||
        strcmp(start_of_vars->name, PATH_VAR_NAME) > 0) {
        ERR_PRINT(ERR_PATH_INIT, init_file);
    }

    int ret_code;
    if (num_args_parsed < argc-1){
        ret_code = run_script(argv[argc-1], &start_of_vars);
    }
    else{
        ret_code = run_interactive(&start_of_vars);
    }

    free_variable(start_of_vars, NON_ZERO_BYTE);
    return ret_code;
}
