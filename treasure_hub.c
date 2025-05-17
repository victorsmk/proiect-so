#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <dirent.h>
#define MAX_BUF 1024

pid_t monitor_pid = -1;   //Uses these 3 parameters to track the monitor process globally,
int monitor_running = 0;  //much simpler than passing as parameters
int stop_issued = 0;
int pipe_fd[2] = {-1, -1};

void handle_sigterm() //stop_monitor
{
    write(1, "[Monitor] Shutting down in 3 seconds...\n", strlen("[Monitor] Shutting down in 3 seconds...\n"));
    usleep(3000000);
    exit(0);
}

void handle_sigusr1() //list_hunts
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
        perror("execvp");
        exit(1);
    }
}

void handle_sigusr2() //list_treasures <huntID>
{
    char buf[256];
    int in = open("monitor_data.txt", O_RDONLY);
    if (in == -1) 
    {
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

        write(1, "[Monitor] Listing treasures...\n", strlen("[Monitor] Listing treasures...\n"));
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
            exit(1);
        }
    }
}

void handle_sighup() //view_treasure <huntID> <treasureID>
{
    char buf[256];
    int in = open("monitor_data.txt", O_RDONLY);
    if (in == -1) 
    {
        perror("data file");
        return;
    }
    int n = read(in, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    if (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[n - 1] = '\0';
    close(in);
    if (n > 0) 
    {
        char *hunt = strtok(buf, " \n");
        char *treasure = strtok(NULL, " \n");
        write(1, "[Monitor] Viewing treasure...\n", strlen("[Monitor] Viewing treasure...\n"));
        pid_t pid = fork();
        if (pid == 0) 
        {
            char *args[] = { "./treasure_manager", "view", hunt, treasure, NULL};
            if ((execvp("./treasure_manager", args)) == -1)
            {
                perror("execvp failed");
                exit(-1);
            }
            perror("execvp");
            exit(1);
        }
    }
}

void handle_sigint() //calculate_score
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    DIR *dirp = opendir(cwd); 
    if (dirp == NULL) 
    {
        perror("current directory");
        exit(-1);
    }
    struct dirent *entry;
    struct stat statbuf;
    while ((entry = readdir(dirp)) != NULL) 
    {
        char fullpath[MAX_BUF*2];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", cwd, entry->d_name);
        if (stat(fullpath, &statbuf) == -1) 
            continue;
        if (S_ISDIR(statbuf.st_mode) && entry->d_name[0] == 'h')
        {
            int local_pipe[2];
            if (pipe(local_pipe) == -1) 
            {
                perror("pipe");
                continue;
            }
            pid_t pid;
            pid = fork();
            if (pid < 0)
            {
                perror("fork");
                exit(-1);
            }
            else
            if (pid == 0)
            {
                close(local_pipe[0]);
                dup2(local_pipe[1], STDOUT_FILENO);
                close(local_pipe[1]);
                write(1, "Hunt: ", strlen("Hunt: "));
                write(1, entry->d_name, strlen(entry->d_name));
                write(1, "\n", strlen("\n"));
                char *args[] = { "./calc", entry->d_name, NULL};
                if ((execvp("./calc", args)) == -1)
                {
                    perror("execvp failed");
                    exit(-1);
                }
                perror("execvp");
                exit(1);
            }
            else
            {
                close(local_pipe[1]);
                waitpid(pid, NULL, 0);
                char buf[MAX_BUF];
                int n;
                while ((n = read(local_pipe[0], buf, sizeof(buf))) > 0) 
                    write(STDOUT_FILENO, buf, n); //the stdout is already redirected through the pipe_fd[1]
                close(local_pipe[0]);
            }
        }
    }
    closedir(dirp);
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
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &handle_sighup;
    sigaction(SIGHUP, &sa, NULL);
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &handle_sigint;
    sigaction(SIGINT, &sa, NULL);
    while (1) 
        pause(); //Waits until it receives a signal
}

void read_from_monitor() 
{
    char output[MAX_BUF];
    int n = read(pipe_fd[0], output, sizeof(output) - 1);
    if (n > 0) 
    {
        output[n] = '\0';
        write(1, output, n);
    }
}

void compile() //Creates an executable file to be used in the list and view functionalities of the treasure hub
{
    if (system("gcc -Wall -o treasure_manager treasure_manager.c") != 0) 
    {
        perror("compile treasure manager");
        exit(-1);
    }
    if (system("gcc -Wall -o calc calculate_score.c") != 0) 
    {
        perror("compile score calculator");
        exit(-1);
    }
}

int main(void)
{
    compile();
    char command[MAX_BUF];
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
                if (pipe(pipe_fd) == -1)
                {
                    perror("pipe");
                    exit(-1);
                }
                monitor_pid = fork();
                if (monitor_pid == 0)
                {
                    close(pipe_fd[0]);
                    dup2(pipe_fd[1], STDOUT_FILENO);
                    close(pipe_fd[1]);
                    run_monitor();
                }
                else 
                if (monitor_pid > 0) 
                {
                    close(pipe_fd[1]);
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
                read_from_monitor();
            }
            else 
                write(1, "Monitor is not running.\n", strlen("Monitor is not running.\n"));
                
        }
        else
        if (strncmp(command, "list_treasures", 14) == 0)
        {
            if (monitor_running)
            {
                char *hunt = strtok(command, " ");
                hunt = strtok(NULL, " ");
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
                read_from_monitor();
            }
            else 
                write(1, "Monitor is not running.\n", strlen("Monitor is not running.\n"));
        }
        else
        if (strncmp(command, "view_treasure", 13) == 0)
        {
            if (monitor_running)
            {
                char *hunt = strtok(command, " ");
                hunt = strtok(NULL, "\n");
                int in = open("monitor_data.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666); 
                if (in == -1)                                                          
                {
                    perror("monitor data file");
                    exit(-1);
                }
                write(in, hunt, strlen(hunt));
                close(in);
                kill(monitor_pid, SIGHUP);
                usleep(500000);
                read_from_monitor();
            }
            else 
                write(1, "Monitor is not running.\n", strlen("Monitor is not running.\n"));
        }
        else
        if (strcmp(command, "calculate_score") == 0)
        {
            if (monitor_running)
            {
                kill(monitor_pid, SIGINT);
                usleep(500000);
                read_from_monitor();
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
                read_from_monitor();
                stop_issued = 1;
                waitpid(monitor_pid, NULL, 0);
                close(pipe_fd[0]);
                write(1, "\nMonitor has stopped\n", strlen("\nMonitor has stopped\n"));
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