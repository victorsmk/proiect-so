#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_BUF 1024

pid_t monitor_pid = -1;   //Uses these 3 parameters to track the monitor process globally,
int monitor_running = 0;  //much simpler than passing as parameters
int stop_issued = 0;

void handle_sigterm(int sig) 
{
    write(1, "[Monitor] Shutting down in 3 seconds...\n", strlen("[Monitor] Shutting down in 3 seconds...\n"));
    usleep(3000000);
    exit(0);
}

void run_monitor()
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask); //Allows signal handler to process other signals while it's running
    sa.sa_flags = 0;  //No special behaviour for signal handler
    sa.sa_handler = &handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    while (1) 
        pause(); //Waits until it receives a signal
}

int main(void)
{
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
                continue;
            }
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
        else 
        if (strcmp(command, "stop_monitor") == 0) 
        {
            if (monitor_running && !stop_issued) 
            {
                char termination_msg[MAX_BUF];
                int status;
                kill(monitor_pid, SIGTERM);
                stop_issued = 1;
                waitpid(monitor_pid, &status, 0);
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