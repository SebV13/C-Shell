#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>  
#include <sys/wait.h>

#define MAX_BUF 160
#define MAX_TOKS 100

int main(int argc, char **argv) 
{
    char *delim = " \n";   
    char *tok;
    char *path;
    char s[MAX_BUF];
    char *toks[MAX_TOKS];
    time_t rawtime;
    struct tm *timeinfo;
    static const char prompt[] = "msh> ";
    FILE *infile;
    int in;
    int out;
    in = 0;
    out = 0;

   /* 
    * process command line options
    */

    if (argc > 2) {
        fprintf(stderr, "msh: usage: msh [file]\n");
        exit(EXIT_FAILURE);
    }
    if (argc == 2) {
        /* read from script supplied on the command line */
        infile = fopen(argv[1], "r");
        if (infile == NULL) {
            fprintf(stderr, "msh: cannot open script '%s'.\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    } else {
        infile = stdin;
    }

    while (1) 
    {
        in = out = 0;              for (int i = 0; i < MAX_TOKS; i++)
            toks[i] = NULL;

        // prompt for input, if interactive input
        if (infile == stdin) {
            printf(prompt);
        }

    /*
     * read a line of input and break it into tokens 
     */

        // read input 
        char *status = fgets(s, MAX_BUF-1, infile);

        // exit if ^d or "exit" entered
        if (status == NULL || strcmp(s, "exit\n") == 0) {
            if (status == NULL && infile == stdin) {
                printf("\n");
            }
            exit(EXIT_SUCCESS);
        }


        // break input line into tokens 
        char *rest = s;
        int i = 0;

        tok = strtok_r(rest, delim, &rest);
        while(tok != NULL && i < MAX_TOKS) 
        {
            toks[i] = tok;
            if(strcmp(tok, "<") == 0)
            {
                in = i;                    i--;
            }
            else if(strcmp(tok, ">")==0)
            {
                out = i;                   i--;
            }
            i++;
            tok = strtok_r(NULL, delim, &rest);
        }

        if (i == MAX_TOKS) {
            fprintf(stderr, "msh: too many tokens");
            exit(EXIT_FAILURE);
        }
        toks[i] = NULL;

    /*
     * process a command
     */

        // do nothing if no tokens found in input
        if (i == 0) {
            continue;
        }

        // if a shell built-in command, then run it 
        if (strcmp(toks[0], "help") == 0) {
            // help 
            printf("enter a Linux command, or 'exit' to quit\n");
            continue;
        } 
        if (strcmp(toks[0], "today") == 0) {
            // today
           time(&rawtime);
           timeinfo = localtime(&rawtime);
           printf("%02d/%02d/%4d\n", 1 + timeinfo->tm_mon, timeinfo->tm_mday,  
           1900 + timeinfo->tm_year);
           continue;            
        }
        if (strcmp(toks[0], "cd") == 0) 
        {
            // cd 
            if (i == 1) {
                path = getenv("HOME");
            } else {
                path = toks[1];
            }
            int cd_status = chdir(path);
            if (cd_status != 0) 
            {
                switch(cd_status) 
                {
                    case ENOENT:
                        printf("msh: cd: '%s' does not exist\n", path);
                        break;
                    case ENOTDIR:
                        printf("msh: cd: '%s' not a directory\n", path);
                        break;
                    default:
                        printf("msh: cd: bad path\n");
                }
            }
            continue;
        }

        // not a built-in, so fork a process that will run the command
        pid_t rc = fork(), rcstatus;       /* use type pid_t, not int */
        if (rc < 0) 
        {
            fprintf(stderr, "msh: fork failed\n");
            exit(1);
        }
        if (rc == 0) 
        {
            if(in)
            {
                int fd0;
                if((fd0 = open(toks[in], O_RDONLY, 0)) == -1)
                {
                    perror(toks[in]);
                    exit(EXIT_FAILURE);
                }
                dup2(fd0, 0);
                close(fd0);
                toks[in] = NULL;
            }

            if(out)
            {
                int fd1;
                if((fd1 = open(toks[out], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT, 
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
                { 
                    perror (toks[out]);
                    exit( EXIT_FAILURE);
                }
                dup2(fd1, 1);
                close(fd1);
                toks[out] = NULL;
            }

            // child process: run the command indicated by toks[0]
            execvp(toks[0], toks);
            /* if execvp returns than an error occurred */
            printf("msh: %s: %s\n", toks[0], strerror(errno));
            exit(1);
        } 
        else 
        {
            // parent process: wait for child to terminate
            while (wait (&rcstatus) != rc)
                continue;
        }
    }
}
