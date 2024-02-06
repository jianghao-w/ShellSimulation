//
//  main.c
//  unixShellPhase1
//
//  Created by hok on 2016/9/24.
//  Copyright © 2016年 jianghao. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "comm.h"
#include <errno.h>


const int status_success = 0;
const int status_failure = 1;


typedef struct CommandTokenChain{
    char *tokenName;
    SymbolType symbol;
    struct CommandTokenChain *next;
    struct CommandTokenChain *prev;
} CommandTokenChain;

typedef struct PipeChain{
//    int *pipefd;
    int pipefd[2];
    struct PipeChain *next;
    struct PipeChain *prev;
}PipeChain;

//Utility Print Char **
void printCharArray(char **array, size_t *length)
{
    int i;
    for (i = 0; i < *length; i++) {
        printf("\narray item %s\n",array[i]);
    }
}
int isCommandEnd(char ch)
{
    
    
    //ch == '<' || ch == '>' ||
    //    if(ch == '|' || ch == ';' || ch == '\n' ||
    //       ch == '&' || ch == '\0')
    //        return 1;
    if ( ch == '\n' || ch == '\0') {
        return 1;
    }
    
    return 0;
}

int findNeedNextCommandBreaker(char ch)
{
    //
    if (ch == ';') {
        return 3;
    }
    if(ch == '<' || ch == '>')
        return 2;
    if(ch == '|' || ch == '\n' ||
       ch == '&' || ch == '\0')
        return 1;
    
    return 0;
}
/*
 return: isArgs , 0 is arguments, 1 is divider or end
 */
int checkIsArgsOrDividerType(char **token, SymbolType *symbol)
{
    int isCharArgs = 1;
    char *temp = *token;
    
    SymbolType symtype;
    switch(*temp) {
        case '\0' : symtype = Null;
            break;
            
        case '>' :
            if(*(temp+1) == '>') {
                symtype = RedirectOutAppend;
                
            } else {
                symtype = RedirectOut;
                
            }
            
            break;
            
        case '<' :  if(*(temp+1) == '<') {
            symtype = RedirectInAppend;
            
        } else {
            
            symtype = RedirectIn;
        }
            
            break;
            
        case '&' :
        {
            if(*(temp+1) == '&') {
                symtype = ExecuteOnSuccess;
            } else {
                printf("The symbol is not accepted : (%c)", *(temp+1));
            }
            break;
        }
            
        case '|' :
        {
            if(*(temp +1) == '|') {
                symtype = ExecuteOnFailure;
            } else {
                symtype = Pipe;
            }
            break;
        }
            
        case '\n' :
        {
            symtype = NewLine;
            break;
        }
            
        case ';' :
        {
            symtype = Semicolon;
            //            isCharArgs = 1;
            break;
        }
            
        default :
        {
            //            printf("Unknown symbol");
            symtype = ArgumentsOrOthers;
            isCharArgs = 0;
            break;
        }
            
    }
    *symbol = symtype;
    return isCharArgs;
}
/*
 Split the input line into tokens using seperator space;
 
 return char array, and array length;
 
 */
int splitLineToToken(char *line, char **charStringAry,size_t *tokenArylength){
    
    char *token;
    size_t i = 0;
    
    //    char * tempCharStringAry[1024] = {};
    
    
    while((token = strsep (&line," "))!=NULL)
    {
        char temp = *token;
        if (isCommandEnd(*token) || temp == ' ') {
            continue;
        }
        charStringAry[i] = token;
        //        strcpy(, token);
        //    printf(" ***** %s\n",charStringAry[i]);
        i++;
    }
    
    //    char ** tempTokenStringAry = malloc( sizeof(char *)*i);
    //    for (int j=0; j < i; j++) {
    //        tempTokenStringAry[j] = tempCharStringAry[j];
    ////        printf("------------ %s\n",charStringAry[j]);
    //    }
    
    //    charStringAry = tempTokenStringAry;
    
    *tokenArylength = i;
    
    
//    //For check tokens array
//    printf("------------ \n");
//    printCharArray(charStringAry, &i);
//    //    printCharArray(tempTokenStringAry, &i);
//    printf("------------ \n");
    
    return status_success;
    
}
char ** buildArgv(Arg *arg_list){
    
    char **argv;
    int argc;
    Arg *tmp_list = arg_list;
    
    if (arg_list == NULL) {
        return NULL;
    }
    /*
     * Compute argc
     */
    int length = 0;
    while (arg_list != NULL) {
        arg_list = arg_list->next;
        length++;
    }
    
    /*
     * Malloc/New the space for argv - Select one! - Note the + 1 for the null pointer.
     */
    argv = (char **)(malloc(sizeof(char *) * length + 1 )); /* C  malloc*/
    
    /*
     * Make the argv pointers point to the strings in arg_list
     */
    for (argc = 0; tmp_list != NULL ; tmp_list = tmp_list->next, argc++){
        
        argv[argc] = tmp_list->arg;
        
    }
    
    /*
     * Terminate the argv structure with a Null as shown in
     * the above figure.
     */
    
    argv[length] = NULL;
    return (argv);
}

/* some utility functions */
// trim the line of white spaces
int trim(char *line, size_t *length)
{
    char *read = NULL, *write = NULL;
    int i = 0, j = 0;
    
    // trim from back
    i = *length - 1;
    while(isspace(line[i]))
        i--;
    line[i+1] = '\0';
    
    // chomp the spaces in front
    read = write = line;
    while (isspace(*read))
        read++;
    
    // move the characters
    for(  ; *write = *read; read++, write++) ;
    
    // update new length
    *length = strlen(line);
    
    return 0;
}


int allocateNewCommandToken(CommandTokenChain **token)
{
    CommandTokenChain *temp = NULL;
    int status = status_success;
    
    if(NULL == (temp = (CommandTokenChain*)malloc(sizeof(CommandTokenChain)))) {
        printf("could not allocate memory for command\n");
        status = status_failure;
        *token = NULL;
        goto exit1;
    }
    
    temp-> tokenName = NULL;
    temp->symbol = ArgumentsOrOthers;
    temp->next = NULL;
    temp->prev = NULL;
    *token = temp;
exit1:
    return status;
}
int addCommandToken(CommandTokenChain **head, CommandTokenChain *add)
{
    int status = status_success;
    CommandTokenChain *temp;
    
    if(NULL == *head) {
        *head = add;
        return status;
    }
    
    // walk till end of list
    for (temp = *head; NULL != temp->next; temp = temp->next) {
    }
    
    temp->next = add;
    add->prev = temp;
    
    return status;
}
void printCommandTokens(CommandTokenChain *head)
{
    while (head != NULL) {
        printf("\n  -----  %s  ------- %d  \n",head->tokenName,(int)head->symbol);
        //        DumpCommand(head);
        head = head->next;
    }
}
int deleteCommandTokens(CommandTokenChain *head) {
    int status = status_success;
    
    CommandTokenChain *temp = head;
    
    while (temp != NULL) {
        head = temp->next;
        free(temp);
        temp = head;
    }
    
    return status;
}
int allocateNewArgs(Arg **arg)
{
    Arg *temp = NULL;
    int status = status_success;
    
    if(NULL == (temp = (Arg*)malloc(sizeof(Arg)))) {
        printf("could not allocate memory for command\n");
        status = status_failure;
        *arg = NULL;
        return status_failure;
    }
    
    temp->arg = NULL;
    temp->next = NULL;
    *arg = temp;
    
    return status;
    
}

int addArgs(Arg **head, Arg *add)
{
    int status = status_success;
    Arg *temp;
    
    if(NULL == *head) {
        *head = add;
        return status;
    }
    
    // walk till end of list
    for (temp = *head; NULL != temp->next; temp = temp->next) {
    }
    
    temp->next = add;
    //    add->prev = temp;
    
    return status;
}
/*
 // allocate a new commnad structure and initialize it with default values
 //
 // return value :
 //      statu_succes -      success, and command points to the newly allocated
 //                          memory
 //      status_failure -    failed to allocate memory, command points to NULL
 //
 // notes:
 */
int allocateNewCommand(Command **command)
{
    
    
    
    Command *temp = NULL;
    int status = status_success;
    
    if(NULL == (temp = (Command*)malloc(sizeof(Command)))) {
        printf("could not allocate memory for command\n");
        status = status_failure;
        *command = NULL;
        return status;
    }
    
    temp->commandName = '\0';
    
    //Arg
    temp->arg_list = NULL;
    temp->last_arg = NULL;
    
    //In
    temp->input_file = NULL;
    temp->input_mode = I_NULL;
    
    //Out
    temp->output_file = NULL;
    temp->output_mode = O_NULL;
    
    //Next Mode
    temp->next_command_exec_on = NEXT_NULL;
    
    //Next Command
    temp->nextCommand = NULL;
    temp->prevCommand = NULL;
    
    //
    temp->execute_state = status_success;
    //    temp->inFilePtr = NULL;
    //    temp->outFilePtr = NULL;
    //    temp->inFileHandle = -1;
    //    temp->outFileHandle = -1;
    //    temp->status = status_success;
    
    *command = temp;
    
    return status;
}
int addCommand(Command **head, Command *add)
{
    int status = status_success;
    Command *temp;
    
    if(NULL == *head) {
        *head = add;
        return status;
    }
    
    // walk till end of list
    for (temp = *head; NULL != temp->nextCommand; temp = temp->nextCommand) {
    }
    
    temp->nextCommand = add;
    add->prevCommand = temp;
    

    
    return status;
}
/*
 */
int deleteCommand(Command *command)
{
    int status = status_success;
    Command *temp = command;
    
    
    // adjust previous command's fwd link
    if(temp->prevCommand != NULL) {
        (temp->prevCommand)->nextCommand = temp->nextCommand;
    }
    
    // adjust next command's prev link
    if(temp->nextCommand != NULL) {
        (temp->nextCommand)->prevCommand = temp->prevCommand;
    }
    
    // free args
    Arg *args = temp->arg_list;
    while (NULL != args) {
        
        Arg* temparg = args;
        args = args->next;
        free(temparg);
        
        
    }
    // free argument list
    
    free(command);
    
    return status;
    
}
/*
 // deletes all the members in the doubly linked list
 */
int deleteCommandChain(Command *head) {
    int status = status_success;
    
    Command *temp = head;
    
    while (temp != NULL) {
        head = temp->nextCommand;
        deleteCommand(temp);
        temp = head;
    }
    
    return status;
}
/*
 // prints the contents of command. (debugging purposes)
 */
void printCommand(Command* command)
{
    int i = 0;
    printf("----------------------------------------------------\n");
    printf("Cmd : %s\n", command->commandName);
    printf("arglist :\n");
    
    
    Arg *tempArg = command->arg_list;
    while (NULL != tempArg) {
        
        printf("%s\n", tempArg->arg);
        tempArg = tempArg->next;
        
    }
    
    printf("input file: %s\n",command->input_file);
    printf("input mode:");
    switch (command->input_mode) {
        case I_NULL:
            printf(" - \n");
            break;
        case I_FILE:
            printf(" I_FILE \n");
            break;
        case I_PIPE:
            printf(" I_PIPE \n");
            break;
        default:
            break;
    }
    
    
    printf("output file: %s\n",command->output_file);
    printf("output mode:");
    switch (command->output_mode) {
        case O_NULL:
            printf(" - \n");
            break;
        case O_WRITE:
            printf(" O_WRITE \n");
            break;
        case O_APPND:
            printf(" O_APPND \n");
            break;
        case O_PIPE:
            printf(" O_PIPE \n");
            break;
        default:
            break;
    }
    
    //    out(command->output_mode);
    
    printf("Command combine mode:");
    switch (command->next_command_exec_on) {
        case NEXT_NULL:
            printf(" - \n");
            break;
        case NEXT_ON_ANY:
            printf(" NEXT_ON_ANY \n");
            break;
        case NEXT_ON_SUCCESS:
            printf(" NEXT_ON_SUCCESS \n");
            break;
        case NEXT_ON_FAIL:
            printf(" NEXT_ON_FAIL \n");
            break;
        case NEXT_ON_PIPE:
            printf(" NEXT_ON_PIPE \n");
            break;
        default:
            break;
    }
    
    printf("\n---------------------------------------------------\n");
}
void printCommandChain(Command *head)
{
//    printf("here\n");
    while (head != NULL) {
        printCommand(head);
        head = head->nextCommand;
    }
}
/*
 Find the divider through the seperated string array, and record the position after the devider, and also the length of between last divider and next divider.
 */
int popNextCommandByDivider(int *beginIndex,size_t *popedArylength,char **wholeCommandAry,size_t tokenArylength)
{
    
    int status = status_success;
    int findBreakerResult = 0;
    int i = *beginIndex;
    
    
    size_t tempSize = tokenArylength;
    //    printf("_----------------------%d %zu_______________\n",i , tempSize);
    for ( ; i < tempSize; i++) {
        findBreakerResult = findNeedNextCommandBreaker(*wholeCommandAry[i]) ;
        if (findBreakerResult == 1 || findBreakerResult == 3) {
            //            printf("\nfind it %d %zu \n",i,*tokenArylength);
            break;
        }
        
        
    }
    
    
    
    if (i == *beginIndex || (i == tokenArylength - 1 && findBreakerResult==1 )|| (i == tokenArylength && 0!=findNeedNextCommandBreaker( *wholeCommandAry[i - 1]))) {
        status = status_failure;
        
        if (findBreakerResult == 1) {
            printf("Invalid Null Command");
        }
        if (findBreakerResult == 2) {
            printf("Mising File Name For Redirect");
            
        }
        goto exit1;
    }else{
        
        //        size_t tSize = tokenArylength - *beginIndex;
        
        if (i == tempSize) {
            //Not found Any Divider
//            printf("Not find any divider\n");
            *popedArylength = i - *beginIndex;
            *beginIndex = i;
            //            i = i - 1;
            
            
        }else{
            //Found Divider Among Commands
//            printf("find divider\n");
            *popedArylength = i + 1 - *beginIndex;
            *beginIndex = i + 1;
            
        }
        
        
        
        
        
        
        
        
        
        //        printCharArray(popedAry, *popedArylength);
//        printf("begin index ===%d",*beginIndex);
        //        wholeCommandAry = &wholeCommandAry[i+1];
        
        //        if (*tokenArylength > 0) {
        //            <#statements#>
        //        }
        
        //        for (int k = 0; k< *tokenArylength; k++) {
        //            //            printf("\n$$$ %zu\n",tempSize);
        //            printf("\nleft whole ary %s\n",tempWholeCommandAry[k]);
        //        }
        
        
    }
    
exit1:
    return status;
}


int allocateBuffer(void **buffer, size_t *currentSize, size_t newSize, size_t datasize)
{
    void *buf;
    int status = status_success;
    
    if(newSize > *currentSize) {
        buf = (void *)realloc(*buffer, (newSize * datasize));
        
        if(NULL == buf) {
            printf("Unable to allocate memory for buffer\n");
            status = status_failure;
            goto exit_1;
        }
        
        *buffer = (void*)buf;
        *currentSize = newSize;
    }
    
exit_1:
    return status;
}
// wrapper around strncpy. alloates buffer and copies string
int allocateAndCopyString(char** destination, char *source)
{
    int status = status_success;
    char *temp = NULL;
    size_t buffersize = 0;
    size_t newsize = strlen(source) + 1;
    
    if(status_failure ==
       (status = allocateBuffer((void**)&temp, &buffersize, newsize+1, sizeof(char)))) {
        *destination = NULL;
        goto exit1;
    }
    
    strncpy(temp, source, newsize);
    temp[newsize] = '\0';
    
    *destination = temp;
    
exit1:
    return status;
}

/*
 
 */
int parseCommandFromTokens(Command **command, CommandTokenChain **commandTokens,Command **headCommand)
{
    int status = status_success;
//    printf("\nbegin parse ===================================\n");
    //    printCommandTokens(commandTokens);
    char *commandName = NULL;
    
    Command *commandtemp = NULL;
    CommandTokenChain *currentCommandToken = *commandTokens;
    
    if (currentCommandToken == NULL) {
        return status_failure;
    }
    
    // allocate memory for command
    if(*command == NULL) {
        if(status_failure == (status = allocateNewCommand(command)))
        {
            *command = NULL;
            return status_failure;
        }
    }
    
    
    
    
    
    int i = 0;
    while (currentCommandToken!=NULL) {
        
        //        printf("\n ********** current token %s" ,currentCommandToken->tokenName);
        //        if (currentCommandToken->prev !=NULL) {
        //            printf("\n***********prev token %d ",currentCommandToken->prev->symbol);
        //        }
        
        
        if (i == 0 ) {
            //First Token Command Name
            if (findNeedNextCommandBreaker(*currentCommandToken->tokenName) != 0) {
                printf("\nUnexpected Token\n");
                return status_failure;
            }
            
            if(status_failure == (status = allocateAndCopyString(&commandName, currentCommandToken->tokenName))) {
                return status;
            }
            
            (*command)->commandName = commandName;
            
            Arg *firstArg = NULL;
            if (status_failure ==(status = allocateNewArgs(&firstArg))) {
                return status;
            }
            
            firstArg->arg = commandName;
            addArgs(&((*command)->arg_list), firstArg);
            
        }else{
            
            SymbolType syType = ArgumentsOrOthers;
            
            int checkStatus = checkIsArgsOrDividerType(&currentCommandToken->tokenName, &syType);
            
            if (checkStatus == 0) {
                //isArguments
                
                if (findNeedNextCommandBreaker(*currentCommandToken->prev->tokenName) != 0) {
                    
                    SymbolType lastSyType = currentCommandToken->prev->symbol;
                    switch (lastSyType) {
                        case RedirectIn:
                        {
                            if(status_failure == (status = allocateAndCopyString(&(*command)->input_file , currentCommandToken->tokenName))) {
                                return status;
                            }
                            
                            break;
                        }
                        case RedirectInAppend:
                        {
                            if(status_failure == (status = allocateAndCopyString(&(*command)->input_file , currentCommandToken->tokenName))) {
                                return status;
                            }
                            
                            break;
                        }
                        case RedirectOut:
                        {
                            if(status_failure == (status = allocateAndCopyString(&(*command)->output_file , currentCommandToken->tokenName))) {
                                return status;
                            }
                            break;
                        }
                        case RedirectOutAppend:
                        {
                            if(status_failure == (status = allocateAndCopyString(&(*command)->output_file , currentCommandToken->tokenName))) {
                                return status;
                            }
                            break;
                        }
                            
                        default:
                            break;
                    }
                    
                    
                }else{
                    Arg *tempArg = NULL;
                    if (status_failure ==(status = allocateNewArgs(&tempArg))) {
                        return status;
                    }
                    tempArg->arg = currentCommandToken->tokenName;
                    addArgs(&((*command)->arg_list), tempArg);
                }
                
                
                
            }else{
                //dividerType
                if (findNeedNextCommandBreaker(*currentCommandToken->prev->tokenName) != 0) {
                    printf("\nUnexpected Token\n");
                    return status_failure;
                }
                currentCommandToken->symbol = syType;
                
//                printf("########## %s",currentCommandToken->prev->tokenName);
                switch (syType) {
                    case RedirectIn:
                    {
                        (*command)->input_mode = I_FILE;
                        break;
                    }
                    case RedirectInAppend:
                    {
                        (*command)->input_mode = I_APPND;
                        break;
                    }
                    case RedirectOut:
                    {
                        (*command)->output_mode = O_WRITE;
                        break;
                    }
                    case RedirectOutAppend:
                    {
                        (*command)->output_mode = O_APPND;
                        break;
                    }
                    case Pipe:
                    {
                        if ((*command)->output_mode != O_NULL) {
                            printf("\nAmbiguous Output Redirect.\n");
                            return status_failure;
                        }
                        
                        (*command)->output_mode = O_PIPE;
                        (*command)->next_command_exec_on = NEXT_ON_PIPE;
                        break;
                    }
                    case ExecuteOnFailure:
                    {
                        (*command)->next_command_exec_on = NEXT_ON_FAIL;
                        break;
                    }
                    case ExecuteOnSuccess:
                    {
                        (*command)->next_command_exec_on = NEXT_ON_SUCCESS;
                        break;
                    }
                    case Semicolon:
                    {
                        (*command)->next_command_exec_on = NEXT_ON_ANY;
                        break;
                    }
                    default:
                        break;
                }
                
            }
//            printf("----- syboltype %d \n",(int)syType);
        }
        
        
        
        
        if(NULL != *headCommand) {
            // walk till end of list
            Command *temp = NULL ;
            for (temp = *headCommand; NULL != temp->nextCommand; temp = temp->nextCommand) {
            }
            if (temp->output_mode == O_PIPE) {
                if ((*command)->input_mode == RedirectInAppend || (*command)->input_mode == RedirectIn) {
                    printf("\nAmbiguous Input Redirect.\n");
                    return status_failure;
                }
                
                (*command)->input_mode = I_PIPE;
            }
        }
        
        
        
        
        
        currentCommandToken = currentCommandToken->next;
        i++;
    }
    
    
    //     printCommand(*command);
    
    return status;
}
/*
 
 */
int getCommandChain(Command **head)
{
    int status = status_success;
    char *line = NULL;
    char *commandString = NULL;
    size_t tokenAryLength = 0;
    
    size_t lineLength = 0;
    size_t bufferLength = 0;
    
    
    
    size_t commandBufferLength = 0;
    size_t commandLength = 0;
    //    SymbolType symtype = Null;
    Command *headCommand = NULL, *tempCommand = NULL;
    
    // buffer is allocated by getline if the length is not sufficient
    if(-1 != (lineLength = getline(&line, &bufferLength, stdin))) {
//        printf("++%s %zu\n",line,lineLength);
        trim(line, &lineLength);
//        printf("--%s %zu\n",line,lineLength);
        headCommand = NULL;
        tempCommand = NULL;
        
        // commandTokenAry store command split by space
        //tokenAryLength store commandTokenAry length
        
        char *commandTokenAry[512] = {} ;
        
        if (status_failure == (status = splitLineToToken(line, commandTokenAry,&tokenAryLength))) {
            //            deleteCommandChain(headCommand);
            //            *head = NULL;
            return status;
        }
        
        //User directly enter without input anything will exit program
        if (tokenAryLength == 1 && isCommandEnd( *commandTokenAry[0])) {
            status = status_failure;
            return status;
        }
        
        
        //       tokenAryLength;
        
        // Store sperated Tokens with fixed array;
        char *tempCommandTokenAry[tokenAryLength];
        
        int k;
        for ( k = 0 ; k < tokenAryLength; k++) {
            tempCommandTokenAry[k] = commandTokenAry[k];
        }
        
        
        
        
        int beginIndex = 0;
        
        //Loop to (1) pop up token arrays seprated by divider, (2)then build the arrays to link list, (3)then parse these token link list into Command, (4)and add the Command to the chain.
        while (tokenAryLength - beginIndex > 0) {
            size_t tempTokenAryLength = tokenAryLength;
            size_t popedAryLength = 0;
            // int xxx = tokenAryLength - beginIndex;
            //            char *popedCommandTokenAry[] = {};
            
            
            //(1)Pop up token arrays seprated by divider;
            if (status_failure == (status = popNextCommandByDivider(&beginIndex,&popedAryLength,tempCommandTokenAry,tempTokenAryLength)))
            {
                deleteCommandChain(headCommand);
                *head = NULL;
                //                printf("\n &&&&&&&&&&&&&&&&&&&&&&  end mal form ---- Need Delete Command Chain");
                // goto exit1;
                return status_failure;
            }
            
            //(2)then build the arrays to link list
            
            CommandTokenChain *headToken = NULL;
            size_t k;
            for (k = beginIndex - popedAryLength ; k < beginIndex; k++) {
                
                CommandTokenChain *tempToken = NULL;
                if(status_failure == (status = allocateNewCommandToken(&tempToken)))
                {
                    tempToken = NULL;
                    deleteCommandChain(headCommand);
                    *head = NULL;
                    return status_failure;
                }
                tempToken->tokenName = tempCommandTokenAry[k];
                addCommandToken(&headToken, tempToken);
                
                //                CommandTokenChain *xChain = head->next;
                //                printf("token name\n%s",head->tokenName);
            }
            //            printCommandTokens(headToken);
            
            //(3)then parse these token link list into Command;
            
            tempCommand = NULL;
            if(status_failure == (status = parseCommandFromTokens(&tempCommand, &headToken,&headCommand)))
            {
                deleteCommandChain(headCommand);
                *head = NULL;
                return status_failure;
            }
            
            addCommand(&headCommand, tempCommand);
            
            //Finish Add Command and delete Token Array;
            deleteCommandTokens(headToken);
            headToken = NULL;
            
            
        }
    }
    
    *head = headCommand;
    
    //Phase1 finish
//    printCommandChain(*head);
    
exit1:
    return status;
}



int getComandChainLength(Command *head) {
    int length = 0;
    Command *temp = head;
    while (temp != NULL) {
        temp = temp->nextCommand;
        length++;
    }
//    printf("%d",length);
    
    return length;
}

int executeSingleCommand(Command *singleCommand)
{
    Command *tempCommand = singleCommand;
    int status = status_success;
    tempCommand->execute_state = status_success;
    pid_t cpid = fork();
    if (cpid < 0 ){    /* Rule 2: Exit if Fork failed */
        fprintf(stderr, "Fork Failed \n");
        tempCommand->execute_state = status_failure;
        exit(1);
    }
    
    else if (cpid == 0 ){
        
//        printf("child pid %d\n",getpid());
        char **arguments = buildArgv(tempCommand->arg_list);
        
        int dupRe;
        //        printf("%s %s %s",arguments[0],arguments[1],arguments[2]);
        //        execlp(....);
        if ((tempCommand->input_mode == I_FILE || tempCommand->input_mode == I_APPND) && tempCommand->input_file != NULL) {
            tempCommand->input_fd = open(tempCommand->input_file, O_RDONLY);
            dupRe = dup2(tempCommand->input_fd, 0);
            
           
            close(tempCommand->input_fd);
            if (dupRe == -1) {
                tempCommand->execute_state = status_failure;
                printf("error\n");
                exit(1);
            }
        }
        if ((tempCommand->output_mode == O_WRITE || tempCommand->output_mode == O_APPND) && tempCommand->output_file != NULL) {
            tempCommand->output_fd = open(tempCommand->output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dupRe =dup2(tempCommand->output_fd, 1);
            
            close(tempCommand->output_fd);
            if (dupRe == -1) {
                printf("error\n");
                tempCommand->execute_state = status_failure;
                
                status = status_failure;
                exit(1);
            }
        }
        
        
        if (execvp(tempCommand->commandName, arguments) < 0) {     /* execute the command  */
            printf("*** ERROR: exec failed\n");
            tempCommand->execute_state = status_failure;
//            printf("\n----%d\n",tempCommand->execute_state);
            status = status_failure;
            exit(1);
        }
        /*Rule 3: Always exit in child */
    }
    
    else{
        
        int statusx;
        int k;
        pid_t pid = 0;
//        for (k=0; k<2; k++) {
         pid = wait(&statusx);
//        printf("statusx %d pid %d",statusx,pid);
    
//        }

        if (statusx != 0) {
//            printf("jianghao");
            
            return status_failure;
        }
//        printf("\n >>>>>>>------- pid ----- <<<<<<<<< \n : %d",pid);
        /* Rule 4: Wait for child unless we need to launch another exec */
//        printf("Child caught\n");
//        exit(0);
//        printf("status1 %d",status);
    
    }
    
//    printf("status2 %d",status);
    return tempCommand->execute_state;
}
int getDividerIndexOfCommandChain(Command *command, int cChainLength)
{
    int index = 0;
    Command *temp = command;
    while (temp != NULL) {
        if (temp->next_command_exec_on == NEXT_ON_ANY || temp->next_command_exec_on == NEXT_ON_SUCCESS || temp->next_command_exec_on == NEXT_ON_FAIL) {
            break;
        }
        if (temp->next_command_exec_on != NEXT_NULL && temp->next_command_exec_on != NEXT_ON_PIPE) {
            return index;
        }
        index++;
        temp = temp->nextCommand;
    }
    if (index + 1 >= cChainLength) {
//        printf("\nNO divider\n");
        return -1;
    }else{
//        printf("\n>>>>>>>>>>> index :%d\n",index);
        return index;
        
    }
    
//    printf("%d",length);
    return index;
}
int execute1PipeCommand(Command *head)
{
    
    int status = status_success;
    Command *executeCommand = head;
//    char **arguments1 = buildArgv(executeCommand->nextCommand->arg_list);
//    printf("%s %s",arguments1[0],arguments1[1]);
 
//    printCommandChain(head);
  
    int pipefd[2];
    int pid;
    
    pipe(pipefd);
    
    pid = fork();
    
    if (pid == 0)
    {
        char **arguments = buildArgv(executeCommand->arg_list);
//        exit(0);
        if ((executeCommand->input_mode == I_FILE || executeCommand->input_mode == I_APPND) && executeCommand->input_file != NULL) {
            executeCommand->input_fd = open(executeCommand->input_file, O_RDONLY);
            int dupRe = dup2(executeCommand->input_fd, 0);
            
//            printf("---- %d\n",dupRe);
            close(executeCommand->input_fd);
            if (dupRe == -1) {
                printf("error\n");
                exit(1);
            }
        }
        dup2(pipefd[1], 1);

        close(pipefd[0]);
        
        //
        //        // execute grep
        //
//        printf("====%s %s %s",executeCommand->commandName,arguments[0],arguments[1]);
        if (execvp(executeCommand->commandName, arguments) < 0) {     /* execute the command  */
            printf("*** ERROR: exec failed\n");
            exit(1);
        }
        
     }else{
             
                
                int pid2;
                
                pid2 = fork();
                
                if (pid2==0) {
   
                    char **arguments1 = buildArgv(executeCommand->nextCommand->arg_list);
                    
                    //        printf("%s",executeCommand->commandName);
                    dup2(pipefd[0], 0);
                    //
                    //        // close unused hald of pipe
                    //
                    close(pipefd[1]);
                    
                    //
                    //        // execute grep
                    //
                    if (execvp(executeCommand->nextCommand->commandName, arguments1) < 0) {     /* execute the command  */
                        printf("*** ERROR: exec failed\n");
                        exit(1);
                    }

                }else{
                    
                        close(pipefd[0]);
                        close(pipefd[1]);
                    int stat;
                    int stat2;
                    int returnpid = 0;
                    int i;
//                    for (i =0; i<2; i++) {
//                        waitpid(<#pid_t#>, <#int *#>, <#int#>)
//                        returnpid = wait(NULL);
//                        //
//                        printf("execute one pipe %d",returnpid);
//                    }
                    waitpid(pid, &stat,0);
                    waitpid(pid2,&stat2,0);
//                    printf("pid %d stat %d \n",pid,stat);
//                    printf("pid2 %d stat2 %d \n",pid2,stat2);
//                    if (stat != 0) {
//                        printf("first wrong");
//                        return status_failure;
//                    }
                    if (stat2 != 0) {
//                        printf("seconde wrong");
                        return status_failure;
                    }
                    
//                    printf("all wait are good");
                }
         
         
    }
    
    return status;
}
int execute2PipeCommand(Command *head)
{
    int status = status_success;
    Command *executeCommand = head;

    
    int pipefd1[2];
    int pid1;
    int pid2;
    int pid3;
    pipe(pipefd1);
    
    pid1 = fork();
    
    if (pid1 == 0)
    {
        char **arguments = buildArgv(executeCommand->arg_list);
        //        exit(0);
        if ((executeCommand->input_mode == I_FILE || executeCommand->input_mode == I_APPND) && executeCommand->input_file != NULL) {
            executeCommand->input_fd = open(executeCommand->input_file, O_RDONLY);
            int dupRe = dup2(executeCommand->input_fd, 0);
            
//            printf("---- %d\n",dupRe);
            close(executeCommand->input_fd);
            if (dupRe == -1) {
                printf("error\n");
                exit(1);
            }
        }
        dup2(pipefd1[1], 1);
        
        close(pipefd1[0]);
        
        //
        //        // execute grep
        //
//        printf("====%s %s %s",executeCommand->commandName,arguments[0],arguments[1]);
        if (execvp(executeCommand->commandName, arguments) < 0) {     /* execute the command  */
            printf("*** ERROR: exec failed\n");
            exit(1);
        }
        
    }else{
        
        
        int pipefd2[2];
        
        
        pipe(pipefd2);
        
        pid2 = fork();
        
        if (pid2==0) {
            
            char **arguments1 = buildArgv(executeCommand->nextCommand->arg_list);
            
            //        printf("%s",executeCommand->commandName);
            dup2(pipefd1[0], 0);
            
            dup2(pipefd2[1], 1);
            //
            //        // close unused hald of pipe
            //
            close(pipefd1[1]);
            
            close(pipefd2[0]);
            
            
            //
            //        // execute grep
            //
            if (execvp(executeCommand->nextCommand->commandName, arguments1) < 0) {     /* execute the command  */
                printf("*** ERROR: exec failed\n");
                exit(1);
            }
            
        }else{
            
            
            pid3 = fork();
            if (pid3 == 0) {
                
                char **arguments2 = buildArgv(executeCommand->nextCommand->nextCommand->arg_list);
                
                dup2(pipefd2[0], 0);
                close(pipefd2[1]);
                close(pipefd1[0]);
                close(pipefd1[1]);
                
                if (execvp(executeCommand->nextCommand->nextCommand->commandName, arguments2) < 0) {     /* execute the command  */
                    printf("*** ERROR: exec failed\n");
                    exit(1);
                }
            }else{
                close(pipefd1[0]);
                close(pipefd1[1]);
                close(pipefd2[0]);
                close(pipefd2[1]);
            }
            
            //                    printf("all wait are good");
        }
        
        
    }
    
    
    int stat1;
    int stat2;
    int stat3;

    
    waitpid(pid1, &stat1,0);
    waitpid(pid2,&stat2,0);
    waitpid(pid3, &stat3, 0);
//    printf("pid %d stat %d \n",pid1,stat1);
//    printf("pid2 %d stat2 %d \n",pid2,stat2);
//    printf("pid3 %d stat3 %d \n",pid3,stat3);
    //                    if (stat != 0) {
    //                        printf("first wrong");
    //                        return status_failure;
    //                    }
    if (stat3 != 0) {
        //                        printf("seconde wrong");
        return status_failure;
    }

    
    return status;

}
int executeCommandChain(Command *head)
{
    
    int dividerIndex;
    
    Command *commandChainHead = head;
    
    
    

    
    while (commandChainHead != NULL) {
        int executeReturn = status_success;
        int commandLength = getComandChainLength(commandChainHead);
        
        dividerIndex = getDividerIndexOfCommandChain(commandChainHead,commandLength);
        
        
        int executeCommandChainLength = (dividerIndex == -1 ) ? commandLength : (dividerIndex + 1);
        
        switch (executeCommandChainLength) {
            case 1:
//                printf("executeSingleCommand\n");
               executeReturn = executeSingleCommand(commandChainHead);
//                executeReturn= commandChainHead->execute_state;
//                commandChainHead->execute_state = 1;
//                printf("return single command %d\n",executeReturn);
                break;
            case 2:
//                printf("execute1PipeCommand\n");
               executeReturn = execute1PipeCommand(commandChainHead);

//                printf("return from 1 pipe\n");
                break;
            case 3:
//                printf("execute2PipeCommand\n");
                executeReturn = execute2PipeCommand(commandChainHead);
//                printf("return from 2 pipe\n");
                break;
                //                case 4:
                //                    execute3PipeCommand();
                //                    break;
                
            default:
                break;
        }

        
        
        if (dividerIndex == -1) {
            commandChainHead = NULL;
        }else{
            int index;
            Command *restPart = commandChainHead;
            for (index = 0; index<=dividerIndex; index++) {
                restPart = restPart->nextCommand;
            }
            commandChainHead = restPart;
            
            int nextOn = commandChainHead->prevCommand->next_command_exec_on;
            if(nextOn == NEXT_ON_SUCCESS && executeReturn == status_failure)
            {
//                printf("should success\n");
                break;
            }
            if (nextOn == NEXT_ON_FAIL && executeReturn == status_success) {
//                printf("should fail\n");
                break;
            }

        }
        

    }
    
    
    
    return status_success;
}

int allocateNewPipes(PipeChain **pipe)
{
    PipeChain *temp = NULL;
    int status = status_success;
    
    if(NULL == (temp = (PipeChain*)malloc(sizeof(PipeChain)))) {
        printf("could not allocate memory for command\n");
        status = status_failure;
        *pipe = NULL;
        return status_failure;
    }
//    temp->pipefd = (int*)malloc(sizeof(int) * 2);
    temp->pipefd[0] = 0;
    temp->pipefd[1] = 0;
//    temp->pipefd;
//
//    temp->arg = NULL;
    temp->next = NULL;
    temp->prev = NULL;
    *pipe = temp;
    
    return status;
    
}
int addNewPipe(PipeChain **head, PipeChain *add)
{
    int status = status_success;
    PipeChain *temp;
    
    if(NULL == *head) {
        *head = add;
        return status;
    }
    
    // walk till end of list
    for (temp = *head; NULL != temp->next; temp = temp->next) {
    }
    
    temp->next = add;
    add->prev = temp;
    
    
    
    return status;
}
int deletePipes(PipeChain *head) {
    int status = status_success;
    
    PipeChain *temp = head;
    
    while (temp != NULL) {
        head = temp->next;
        free(temp);
        temp = head;
    }
    
    return status;
}
void printPIPE(PipeChain *head)
{
//    printf("pip\n");
    while (head != NULL) {
        printf("\n  -----  %d  ------- %d  \n",(int)head->pipefd[0],(int)head->pipefd[1]);
        //        DumpCommand(head);
        head = head->next;
    }
}
//void closePipes(PipeChain *head)
//{
//    while (head != NULL) {
//        close(head->pipefd[0]);
//        close(head->pipefd[1]);
//        //        DumpCommand(head);
//        head = head->next;
//    }
//}
int executeCommandWithCheckPipe(Command *commandChainHead, PipeChain *pipeHead )
{
    int status = status_success;
    
    pid_t cpid = fork();
    if (cpid < 0 ){    /* Rule 2: Exit if Fork failed */
        status = status_failure;
        exit(1);
    }
    
    else if (cpid == 0 ){
        
        char **arguments = buildArgv(commandChainHead->arg_list);
        
        int dupRe;
        
        if (commandChainHead->input_mode == I_PIPE) {
            PipeChain *temp1 ;
            for (temp1 = pipeHead ; NULL != temp1->next; temp1 = temp1->next) {
                
            }
            
            dup2(temp1->pipefd[0], 0);
            close(temp1->pipefd[1]);
            
            
            //                    if (temp1->prev!=NULL) {
            //                        close(temp1->prev->pipefd[0]);
            //                        close(temp1->prev->pipefd[1]);
            //                    }
            PipeChain *temp2;
            temp2 = temp1;
            while (temp2->prev!= NULL) {
                temp2 = temp2->prev;
                close(temp2->pipefd[0]);
                close(temp2->pipefd[1]);
            }
            
            
        }
        
        if ((commandChainHead->input_mode == I_FILE || commandChainHead->input_mode == I_APPND) && commandChainHead->input_file != NULL) {
            commandChainHead->input_fd = open(commandChainHead->input_file, O_RDONLY);
            dupRe = dup2(commandChainHead->input_fd, 0);
            
            
            close(commandChainHead->input_fd);
            
            if (dupRe == -1) {
                status = status_failure;
                printf("error\n");
                exit(1);
            }
        }
        if ((commandChainHead->output_mode == O_WRITE || commandChainHead->output_mode == O_APPND) && commandChainHead->output_file != NULL) {
            commandChainHead->output_fd = open(commandChainHead->output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dupRe =dup2(commandChainHead->output_fd, 1);
            
            close(commandChainHead->output_fd);
            if (dupRe == -1) {
                printf("error\n");
                
                status = status_failure;
                exit(1);
            }
        }
        
        
        if (execvp(commandChainHead->commandName, arguments) < 0) {     /* execute the command  */
            printf("*** ERROR: exec failed\n");
            status = status_failure;
            exit(1);
        }
        /*Rule 3: Always exit in child */
    }
    
    else{
        
        if (pipeHead!=NULL) {
            
            
            PipeChain *temp = pipeHead;
            while (temp != NULL) {
                close(temp->pipefd[0]);
                close(temp->pipefd[1]);
                wait(NULL);
                temp = temp->next;
            }
        }
        
        int statusx;
        pid_t pid = 0;
        pid = wait(&statusx);
        
        if (statusx != 0) {
            status =status_failure;
            //                    printf("single failure");
        }
        //        printf("\n >>>>>>>------- pid ----- <<<<<<<<< \n : %d",pid);
        /* Rule 4: Wait for child unless we need to launch another exec */
        //        printf("Child caught\n");
        //        exit(0);
        //        printf("status1 %d",status);
        
    }

    
    return status;
}
int executeCommandChainUsingLoop(Command *head)
{
    
    int status = status_success;
    Command *commandChainHead = head;

    PipeChain *pipeHead = NULL;
    
    while (commandChainHead != NULL) {

        
        int executeReturn = 0;
     
        if (commandChainHead->nextCommand != NULL && commandChainHead->next_command_exec_on == NEXT_ON_PIPE) {
            PipeChain *outpipe = NULL;
            if(status_failure == (status = allocateNewPipes(&outpipe)))
            {
                outpipe = NULL;
                
                pipeHead =NULL;
//                deleteCommandChain(headCommand);
//                *head = NULL;
                return status_failure;
            }
            
            addNewPipe(&pipeHead, outpipe);
            
            pid_t bpid;
            
            PipeChain *tempPipe;
            for (tempPipe = pipeHead ; NULL != tempPipe->next; tempPipe = tempPipe->next) {
            }
            
            pipe(tempPipe->pipefd);
            bpid = fork();
//            printf("before fork a child");
            
            
            if (bpid == 0) {
                
                char **arguments = buildArgv(commandChainHead->arg_list);
                //        exit(0);
                
                
                
                if (commandChainHead->input_mode == I_PIPE) {
                    
                    PipeChain *prevPipe = tempPipe;
                    if (prevPipe->prev!= NULL) {
                        dup2(prevPipe->prev->pipefd[0], 0);
                        close(prevPipe->prev->pipefd[1]);
                        
                        prevPipe = prevPipe->prev;
                        
                    }
                    while (prevPipe-> prev !=NULL) {
                        prevPipe = prevPipe->prev;
                        close(prevPipe->pipefd[1]);
                        close(prevPipe->pipefd[0]);
                    }
                    
                    
                    
                }
                
                if ((commandChainHead->input_mode == I_FILE || commandChainHead->input_mode == I_APPND) && commandChainHead->input_file != NULL) {
                    commandChainHead->input_fd = open(commandChainHead->input_file, O_RDONLY);
                    int dupRe = dup2(commandChainHead->input_fd, 0);
                    
                    //                printf("---- %d\n",dupRe);
                    close(commandChainHead->input_fd);
                    
                    if (dupRe == -1) {
                        printf("error\n");
                        exit(1);
                    }
                }
                
                dup2(tempPipe->pipefd[1], 1);
                
                close(tempPipe->pipefd[0]);
//                closePipes(pipeHead);
                
                if (execvp(commandChainHead->commandName, arguments) < 0) {     /* execute the command  */
                    printf("*** ERROR: exec failed\n");
                    exit(1);
                }
            }else{
//                printf("paranet fork a child");
                
            }
            

            
        }
        
        if (commandChainHead->next_command_exec_on != NEXT_ON_PIPE && commandChainHead->nextCommand != NULL) {
            
            
            executeReturn = executeCommandWithCheckPipe(commandChainHead, pipeHead);
            
            deletePipes(pipeHead);
            pipeHead = NULL;
            
            int nextOn = commandChainHead->next_command_exec_on;
            if(nextOn == NEXT_ON_SUCCESS && executeReturn == status_failure)
            {
//                printf("should success\n");
                break;
            }
            if (nextOn == NEXT_ON_FAIL && executeReturn == status_success) {
//                printf("should fail\n");
                break;
            }


        }
        
        

        
        if(commandChainHead->nextCommand == NULL)
        {
           
            
            executeReturn = executeCommandWithCheckPipe(commandChainHead, pipeHead);
            
            deletePipes(pipeHead);
            pipeHead = NULL;
            
        }
        
        commandChainHead = commandChainHead->nextCommand;

    }
    
    return status;
}

int main(int argc, const char * argv[]) {
    
    Command *head = NULL;
    
    int isExit = status_failure;
    
    // get the command chain
    int i = 0;
    for (i = 0; i<25; i++) {
        printf("\nosh>");
        if(status_failure == getCommandChain(&head)) {
            //        DumpCommandChain(head);
            //Success
            deleteCommandChain(head);
            head = NULL;
            
        }else{
            Command *temp = head;
            while (temp!=NULL) {
                int rc;
//                printf("loog");
                if (0 ==(rc = strcmp(temp->commandName, "exit"))) {
                    isExit = status_success;
                    break;
                }
                temp = temp->nextCommand;
            }
            
            if (isExit == status_success) {
                break;
            }
//            executeCommandChain(head);
            executeCommandChainUsingLoop(head);
            deleteCommandChain(head);
            head = NULL;
            
        }

    }
    printf("\n Exit Program \n");
    
    
//    printCommandChain(head);
    

    

exit1:
    return 0;
}
