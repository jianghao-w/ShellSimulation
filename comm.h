#include <sys/types.h>
#ifndef	DEFS_H
#define DEFS_H

/*
 * Input Modes
 */
#define I_NULL  0
#define I_FILE  1
#define I_APPND 2
#define I_PIPE  3

/*
 * Output Modes
 */

#define O_NULL         0
#define O_WRITE        1
#define O_APPND        2
#define O_PIPE         3 

/*
 * Run Next Command Modes
 */
#define NEXT_NULL          0
#define NEXT_ON_ANY        1
#define NEXT_ON_SUCCESS    2
#define NEXT_ON_FAIL       3 
#define NEXT_ON_PIPE        4


/*
 * Parse State 
 */
#define NEED_ANY_TOKEN 0
#define NEED_NEW_COMMAND 1
#define NEED_IN_PATH 2
#define NEED_OUT_PATH 3

extern int cerror;


/* A C sytle linked list to parse and build the argv structure */
typedef struct Arg{
        char *arg;
        struct Arg *next;
} Arg;


typedef enum SymbolType {
    ArgumentsOrOthers = 0,
    RedirectIn,             // <
    RedirectInAppend,       // <<
    RedirectOut = 3,            // >
    RedirectOutAppend,      // >>
    ExecuteOnSuccess,         // && - exec on success
    ExecuteOnFailure,          // || - exec on failure
    Pipe,                   // |
    Null,                   // end of string (null character encountered)
    NewLine,                // end of command due to new line
    Semicolon,              // ;
} SymbolType;

/* A C sytle linked list to parse the input line */
typedef struct Command{
    
        char *commandName;
    
        int  execute_state;

        Arg  *arg_list;
        Arg  *last_arg;

        char *input_file;
        int  input_mode;
        int  input_fd;
//        FILE *input_file_handler;

        char *output_file;
        int  output_mode;
        int  output_fd;
//        FILE *output_file_handler;

        int  next_command_exec_on;
        pid_t pid;
        struct Command *nextCommand,*prevCommand;
} Command;

#endif

