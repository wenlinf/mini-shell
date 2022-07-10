#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#define MAX_BUFFER_SIZE 80

typedef struct node{
    char* command;
    struct node* next;
}node_t;

typedef struct linkedlist{
    node_t* head;
}linkedlist_t;

linkedlist_t* CreateLinkedList(){
    linkedlist_t* list = (linkedlist_t*)malloc(sizeof(linkedlist_t));
    list->head = NULL;
    return list;
}

void FreeLinkedList(linkedlist_t* list){
    if (list->head == NULL) {
        free(list);
        return;
    }
    node_t* curr = list->head;
    node_t* next = curr->next;
    while(next != NULL) {
        free(curr->command);
        free(curr);
        curr = next;
        next = curr->next;
    }
    free(curr->command);
    free(curr);
    free(list);
}

void AppendToLinkedList(linkedlist_t* list, char* command){
    node_t* newNode = (node_t*)malloc(sizeof(node_t));
    newNode->command = strdup(command);
    newNode->next = NULL;
    if (list->head == NULL) {
        list->head = newNode;
    } else {
        node_t* curr = list->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newNode;
    }
}

int PrintLinkedList(linkedlist_t* list){
    node_t* curr = list->head;
    while (curr != NULL) {
        printf("%s\n", curr->command);
        curr = curr->next;
    }
    return 1;
}

void sigint_handler(int sig) {
    write(1,"Minishell terminated.\n",22); 
    exit(0);
}

void split_line(char* args, char** split_args, char* sep) {
    char* arg;
    arg = strtok(args, sep);
    int i = 0;
    split_args[i] = arg;
    i++;
    while (arg != NULL) {
        arg = strtok(NULL, sep);
        split_args[i] = arg;
        i++;
    }
}

int execute_funcs(char** args) {
    pid_t pid1;
    pid1 = fork();
    int child_status;
    if (pid1 == 0) {
        execvp(args[0], args);
        printf("Command not found. Try again.\n");
        exit(1);
    }
     waitpid(pid1, &child_status, 0); 
    return 1;
}

int  builtin_cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "must provide a directory.\n");
    }else {
        chdir(args[1]);
    }
    return 1;
}

int builtin_help(char** args) {
    printf("========Welcome to minishell========\n");
    printf("Built-in functions in this minishell:\n");
    printf("- cd\n");
    printf("- exit\n");
    printf("- help\n");
    printf("- history\n");
    printf("Use man for details of other commands.\n");
    return 1;
}

int builtin_exit(char** args) {
    printf("Bye\n");
    exit(0);
    return 0;
}

int execute_pipe(char** args, char** pipeargs) {
    int fd[2];
    pipe(fd);
    int child_status;
    if (fork() == 0) {
        close(STDOUT_FILENO);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(args[0], args);
        printf("Command not found. Try again.\n");
        exit(1);
    } else {
      if (fork() == 0) {
        close(STDIN_FILENO);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        close(fd[1]);
        execvp(pipeargs[0], pipeargs);
        printf("Command not found. Try again.\n");
        exit(1);
      }
    }
    close(fd[0]);
    close(fd[1]);
    wait(0);
    wait(0);
    return 1;
}

int main(){
  alarm(180); // Please leave in this line as the first statement in your program.
              // This will terminate your shell after 180 seconds,
              // and is useful in the case that you accidently create a 'fork bomb'
  signal(SIGINT, sigint_handler);
  
  char line[MAX_BUFFER_SIZE];
  
 
  char* builtins[] = {"cd", "help", "exit"};
  int (*builtin_funs[])(char**) = {&builtin_cd, &builtin_help, &builtin_exit};

  linkedlist_t* history = CreateLinkedList();

  while (1) {
    printf("%s@mini-shell> ", getenv("USER"));

    fgets(line, MAX_BUFFER_SIZE, stdin);

    // strip the endline
    line[strlen(line) - 1] = '\0';
    if (strlen(line) == 0) {
        printf("Enter a valid command.\n");
        continue;
    }
    AppendToLinkedList(history, line);
    
    int pipe_executed = 0;
    
    if (strchr(line, '|') != NULL) {
       char* pipes[2];
       split_line(line, pipes, "|");
       char* before[4];
       split_line(pipes[0], before, " ");
       char* after[4];
       split_line(pipes[1], after, " "); 
       pipe_executed = execute_pipe(before, after); 
    }
    if (pipe_executed) continue;
    
    // split the command the user typed in
    char* split[5];
    split_line(line, split, " ");
    
    int i;
    int history_executed = 0;
    if (!strcmp(split[0], "history")) {
        history_executed = PrintLinkedList(history);
    } 
    if (history_executed) continue;
    int builtin_executed = 0;
    if (!strcmp("exit", split[0])) {
        FreeLinkedList(history);
    }
    for (i = 0; i < 3; i++) {
        if (!strcmp(builtins[i], split[0])) {
            builtin_executed = builtin_funs[i](split);
        }
    }
    if (builtin_executed) continue;
    execute_funcs(split);
}
  return 0;
}
