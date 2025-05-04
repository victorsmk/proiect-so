#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#define MAX_BUF 1024

pid_t monitor_pid = -1;   //Uses these 3 parameters to track the monitor process globally,
int monitor_running = 0;  //much simpler than passing as parameters
int stop_issued = 0;

void handle_sigterm() 
{
    write(1, "[Monitor] Shutting down in 3 seconds...\n", strlen("[Monitor] Shutting down in 3 seconds...\n"));
    usleep(1000000);
    write(1, "[Monitor] Shutting down in 2 seconds...\n", strlen("[Monitor] Shutting down in 2 seconds...\n"));
    usleep(1000000);
    write(1, "[Monitor] Shutting down in 1 seconds...\n", strlen("[Monitor] Shutting down in 1 seconds...\n"));
    usleep(1000000);
    exit(0);
}

void handle_sigusr1()
{
    write(1, "[Monitor] Listing hunts...\n", strlen("[Monitor] Listing hunts...\n"));
    pid_t pid = fork();
    if (pid == 0) 
    {
        char *args[] = {"treasure_manager", "list_all", NULL};
        if ((execvp("./treasure_manager", args)) == -1)
        {
            perror("execvp failed");
            exit(-1);
        }
        exit(0);
    }
}

void handle_sigusr2()
{
    char buf[256];
    int in = open("monitor_data.txt", O_RDONLY);
    if (in == -1) {
        perror("data file");
        return;
    }
    int n = read(in, buf, sizeof(buf) - 1);
    close(in);
    if (n > 0) 
    {
        buf[n] = '\0';
        int len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        write(1, "[Monitor] Viewing treasures...\n", strlen("[Monitor] Viewing treasures...\n"));
        pid_t pid = fork();
        if (pid == 0) 
        {
            char *args[] = { "./treasure_manager", "list", buf, NULL };
            if ((execvp("./treasure_manager", args)) == -1)
            {
                perror("execvp failed");
                exit(-1);
            }
            perror("execvp");
            exit(0);
        }
    }
}


void run_monitor()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &handle_sigusr1;
    sigaction(SIGUSR1, &sa, NULL);
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &handle_sigusr2;
    sigaction(SIGUSR2, &sa, NULL);
    while (1) 
        pause(); //Waits until it receives a signal
}

int main(void)
{
    //Creates an executable file to be used in the list and view functionalities of the treasure hub
    char *compile_command = "gcc -Wall -o treasure_manager treasure_manager.c";
    int ret = system(compile_command); 
    if (ret) 
    {
        perror("Could not compile treasure_manager\n");
        exit(-1);
    }
    char command[256];
    char buff[MAX_BUF];
    while(1) //Ensures the process doesn't terminate after one action and keeps waiting for signals
    {
        write(1, ">> ", 3);
        int len = read(0, command, sizeof(command) - 1);
        command[len] = '\0';
        if (command[len - 1] == '\n') 
            command[len - 1] = '\0';
        if (strcmp(command, "start_monitor") == 0)
        {
            if (monitor_running) 
            {
                write(1, "Monitor is already running\n", strlen("Monitor is already running\n"));
            }
            else
            {
                monitor_pid = fork();
                if (monitor_pid == 0)
                    run_monitor();
                else 
                if (monitor_pid > 0) 
                {
                    monitor_running = 1;
                    snprintf(buff, sizeof(buff) - 1,  "Monitor started, PID: %d\n", monitor_pid);
                    buff[sizeof(buff)] = '\0';
                    write(1, buff, strlen(buff));
                } 
                else 
                {
                    perror("fork");
                    exit(1);
                }
            }
            
        }
        else 
        if (strcmp(command, "list_hunts") == 0)
        {
            if (monitor_running)
            {
                kill(monitor_pid, SIGUSR1);
                usleep(500000);
            }
            else 
                write(1, "Monitor is not running.\n", strlen("Monitor is not running.\n"));
                
        }
        else
        if (strncmp(command, "list_treasures", 14) == 0)
        {
            if (monitor_running)
            {
                char *token = strtok(command, " ");
                char *hunt = strtok(NULL, " ");
                int in = open("monitor_data.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666); //Write the hunt that we want to print in a file
                if (in == -1)                                                          //with a known name, to avoid using a global "command" variable
                {
                    perror("monitor data file");
                    exit(-1);
                }
                write(in, hunt, strlen(hunt));
                close(in);
                kill(monitor_pid, SIGUSR2);
                usleep(500000);
            }
            else 
                write(1, "Monitor is not running.\n", strlen("Monitor is not running.\n"));
        }
        else 
        if (strcmp(command, "stop_monitor") == 0) 
        {
            if (monitor_running && !stop_issued) 
            {
                kill(monitor_pid, SIGTERM);
                stop_issued = 1;
                waitpid(monitor_pid, NULL, 0);
                write(1, "Monitor has stopped\n", strlen("Monitor has stopped\n"));
                monitor_running = 0;
                stop_issued = 0;
            }
            else 
                write(1, "Monitor is not running\n", strlen("Monitor is not running\n"));
        }
        else 
        if (strcmp(command, "exit") == 0) 
        {
            if (monitor_running) 
                write(1, "Monitor is still running! Stop it first\n", strlen("Monitor is still running! Stop it first\n"));
             else 
                break;
        }
        else 
            write(1, "Command not recognized\n", strlen("Command not recognized\n"));
    }
    return 0;
}