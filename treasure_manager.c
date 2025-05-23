#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#define MAX_BUF 1024

typedef struct
{
    char id[32];
    char username[64];
    char lat[32];
    char lon[32];
    char clue[256];
    char value[32];

}Treasure_t; //Structure used as a buffer for writing in a file

//Where to get data for treasure?? (input file for the moment)
//Can you add the same treasure twice?

int createSymLink(char *huntID)
{
    char linkp[256];
    snprintf(linkp, sizeof(linkp), "logged_hunt-%s", huntID);
    struct stat st;
    if (lstat(linkp, &st) == 0) 
        return 1;
    char target[256];
    snprintf(target, sizeof(target), "%s/logged_hunt.txt", huntID);
    if (symlink(target, linkp) == 0)
        return 0;
     else 
        return -1;
}

int addTreasure (char *huntID, char *inputFile)
{
    Treasure_t treasure;
    int in = open(inputFile, O_RDONLY);
    if (!in)
        return -1;
    struct stat st;
    if (stat(huntID, &st) != 0)
         mkdir(huntID, 0755);   //Checks if file with the given name exists, if not makes a directory with given name
                                 //no point in checking if the file is a directory, since there are no directories in directories,
                                    //and each treasure has to be inside a directory.
    char buffer[MAX_BUF + 1];
    int bytes_read = read(in, buffer, sizeof(buffer) - 1);  //It is considered that an input file only contains information for one treasure
    buffer[bytes_read] = '\0';                              //since it's specified that you can only add a single treasure at a time
    char *line = strtok(buffer, "\n");
    if (line) 
        strcpy(treasure.id, line);
    line = strtok(NULL, "\n");
    if (line) 
        strcpy(treasure.username, line);                    //The function is pretty simple, just copies information from the input file to the buffer
    line = strtok(NULL, "\n");                              //and writes it in the output file, which is the "treasures" file from the given hunt
    if (line)
        sscanf(line, "%s %s", treasure.lat, treasure.lon);
    line = strtok(NULL, "\n");
    if (line) 
        strcpy(treasure.clue, line);
    line = strtok(NULL, "\n");
    if (line) 
        strcpy(treasure.value, line);
    char filep[256];
    snprintf(filep, sizeof(filep), "%s/%s", huntID, "treasures");
    int out = open(filep, O_APPEND | O_CREAT | O_WRONLY, 0644);
    if (!out)
    {
        close(in);
        return -1;
    }
    char outbuf[MAX_BUF];
    int len = snprintf(outbuf, sizeof(outbuf), "Treasure ID: %s\nUser: %s\nCoordinates: %s, %s\nClue: %s\nValue: %s\n", treasure.id, treasure.username, treasure.lat, treasure.lon, treasure.clue, treasure.value);
    write(out, outbuf, len);
    char logp[256];
    snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt.txt");
    int flog = open(logp, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (!flog)
    {
        close(in);
        close(out);
        return -1;
    }
    char log[MAX_BUF];
    len = snprintf(log, sizeof(log), "added %s to %s\n", treasure.id, huntID);
    write(flog, log, len);
    if (createSymLink(huntID) < 0)
        return -1;
    close(out);
    close(in);
    close(flog);
    return 0;
    
}

//Print at stdout or in a file? (for now, at stdout)

void listHunt(char *huntID)
{
    struct stat st;
    if (stat(huntID, &st) != 0) 
    {
        char err[MAX_BUF];
        snprintf(err, sizeof(err) - 1, "Hunt %s doesn't exist\n", huntID);
        err[sizeof(err)] = '\0';
        write(1, err, strlen(err));
    }
    else
    {
        char filep[256];
        snprintf(filep, sizeof(filep), "%s/%s", huntID, "treasures");
        int in = open(filep, O_RDONLY);
        if (in)
        {
            stat(filep, &st);
            char buff[MAX_BUF];
            long int eet_time = st.st_mtim.tv_sec + 3*3600;  //adjusts time according to EET
            strftime(buff, sizeof(buff), "%D %T", gmtime(&eet_time));
            char out[MAX_BUF*2];
            int bytes_read;
            snprintf(out, sizeof(out) - 1, "Hunt name: %s, Total size: %ld bytes, Last modification: %s\n", huntID, st.st_size, buff);
            out[sizeof(out)] = '\0';
            write(1, out, strlen(out));
            while ((bytes_read = read(in, buff, sizeof(buff))))
                write(1, buff, bytes_read); //1 represents the stdout, if necessary will change output to a different file
            char logp[256];
            snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt.txt");
            int flog = open(logp, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (!flog)
            {
                close(in);
                return;
            }
            char log[MAX_BUF];
            int len = snprintf(log, sizeof(log), "listed this hunt\n");
            write(flog, log, len);
            close(flog);
            close(in);
        }
        
    }
}

//Also print at stdout for now, will change if needed

void viewTreasure(char *huntID, char *treasureID)
{
    struct stat st;
    char err[MAX_BUF];
    if (stat(huntID, &st) != 0) 
    {
        snprintf(err, sizeof(err) - 1, "Hunt %s doesn't exist\n", huntID);
        err[sizeof(err)] = '\0';
        write(1, err, strlen(err));
    }
    char filep[256];
    snprintf(filep, sizeof(filep), "%s/%s", huntID, "treasures");
    int in = open(filep, O_RDONLY);
    if (in)
    {
        char buf[MAX_BUF + 1];
        int bytesRead = read(in, buf, MAX_BUF);
        buf[bytesRead] = '\0';
        char *line = strtok(buf, "\n");
        int found = 0;
        while (line != NULL) 
        {
            if (strncmp(line, "Treasure ID:", 12) == 0) //This function assumes that each treasure is stored in the same format
            {                                           
                char id[32];
                sscanf(line, "Treasure ID: %s", id);
                if (strcmp(id, treasureID) == 0) 
                {
                    found = 1;
                    write(1, line, strlen(line));
                    write(1, "\n", 1);
                    for (int i = 0; i < 4; i++) 
                    {
                        line = strtok(NULL, "\n");
                        if (line) 
                        {
                            write(1, line, strlen(line));
                            write(1, "\n", 1);
                        }
                    }
                    break; //Also assumes there is only one treasure with a given ID
                }          //If there can be multiple, just remove the break
            }
            line = strtok(NULL, "\n");
        }
        if (!found)
        {
            snprintf(err, sizeof(err) - 1, "Treasure with ID %s not found in hunt %s.\n", treasureID, huntID);
            err[sizeof(err)] = '\0';
            write(1, err, strlen(err));
        }
            
        else
        {
            char logp[256];
            snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt.txt");
            int flog = open(logp, O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (!flog)
            {
                close(in);
                return;
            }
            char log[MAX_BUF];
            int len = snprintf(log, sizeof(log), "viewed treasure %s from this hunt\n", treasureID);
            write(flog, log, len);
            close(flog);
        }
        close(in);
    }
}

int removeHunt (char *huntID)
{
    DIR *dirp = opendir(huntID);
    if (!dirp)
        return -1;
    struct dirent *file;
    char filep[300], logp[300], linkp[300];
    while ((file = readdir(dirp)) != NULL) //Works with one but also multiple files in the same directory
    {
        if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) //Ignores the two default subdirectories of every directory
            continue;
        snprintf(filep, sizeof(filep), "%s/%s", huntID, file->d_name);
        snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt.txt");
        snprintf(linkp, sizeof(linkp), "%s-%s", "logged_hunt", huntID);
        unlink(filep);
    }
    unlink(logp);
    unlink(linkp);
    closedir(dirp);
    rmdir(huntID);
    return 0;
}

int removeTreasure(char *huntID, char *treasureID) 
{
    struct stat st;
    if (stat(huntID, &st) != 0) 
        return -1;
    char filep[256], tempp[256];                                //If we dont want to leave empty lines, the easiest method is just to copy
    snprintf(filep, sizeof(filep), "%s/treasures", huntID);     //all the information from the old file to the new one, and ignore the treasure
    snprintf(tempp, sizeof(tempp), "%s/treasures_tmp", huntID); //that we want to delete
    int in = open(filep, O_RDONLY);
    if (!in)
        return -1; 
    int out = open(tempp, O_WRONLY | O_CREAT, 0644);
    if (!out)
    {
        close(in);
        return -1;
    }
    char buffer[MAX_BUF + 1];
    int bytesRead = read(in, buffer, MAX_BUF);
    if (bytesRead <= 0) 
    {
        close(in); 
        close(out);
        return -1;
    }
    buffer[bytesRead] = '\0';
    char *line = strtok(buffer, "\n");
    int lineCount = 0;
    char block[6][512]; //6 lines per treasure
    while (line) 
    {
        strncpy(block[lineCount], line, sizeof(block[lineCount]) - 1);
        block[lineCount][sizeof(block[lineCount]) - 1] = '\0';
        lineCount++;
        if (lineCount == 6) 
        {
            char id[64];
            sscanf(block[0], "Treasure ID: %s", id);
            if (strcmp(id, treasureID) != 0) // If not the one we're removing, write all 6 lines
            {
                for (int i = 0; i < 6; i++) 
                {
                    char lineOut[520];
                    int len = snprintf(lineOut, sizeof(lineOut), "%s\n", block[i]);
                    write(out, lineOut, len);
                }
            }
            lineCount = 0;
        }
        line = strtok(NULL, "\n");
    }
    close(in);
    close(out);
    rename(tempp, filep); // Replace original file with the new one
    unlink(tempp);
    char logp[256];
    snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt.txt");
    int flog = open(logp, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (!flog)
        return -1;
    char log[MAX_BUF];
    int len = snprintf(log, sizeof(log), "removed treasure %s from this hunt\n", treasureID);
    write(flog, log, len);
    close(flog);
    return 0;
}

void listHunts()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    DIR *dirp = opendir(cwd); 
    if (dirp == NULL) 
        return;
    write(1, "Current hunts:\n", strlen("Current hunts:\n"));
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
            write(1, entry->d_name, strlen(entry->d_name));
            write(1, "\n", strlen("\n"));
        }
    }
    closedir(dirp);
}

//Dont think its necessary to make the printing functions return anything on success,
//taking into account the fact that they will print information on success and not print on failure.
//However, if the functions will be changed to write in a different file,
//maybe also make them return some values

//For the logged_hunt maybe also add the time of each command
//Also is it just the attempted operation? Or does it have to be successful? (for now, it has to be successful)

int main(int argc, char **argv)
{
    char yes_no;
    int check = 1;
    char err[MAX_BUF];
    if (strcmp(argv[1], "add") == 0)
    {
        check = addTreasure(argv[2], argv[3]);
        if (check == 0)
        {
            snprintf(err, sizeof(err) - 1, "Treasure added successfully to hunt %s\n", argv[2]);
            err[sizeof(err)] = '\0';
            write(1, err, strlen(err));
        }
            
        else
        {
            snprintf(err, sizeof(err) - 1, "Error at adding treasure to hunt %s\n", argv[2]);
            err[sizeof(err)] = '\0';
            write(1, err, strlen(err));
        }
           
    }
    else
    if (strcmp(argv[1], "list") == 0)
        listHunt(argv[2]);
    else
    if (strcmp(argv[1], "view") == 0)
        viewTreasure(argv[2], argv[3]);
    else
    if (strcmp(argv[1], "list_all") == 0)
        listHunts();
    else
    if(strcmp(argv[1], "remove_treasure") == 0)
    {
        check = removeTreasure(argv[2], argv[3]);
        if(check == 0)
        {
            snprintf(err, sizeof(err) - 1, "Treasure %s removed successfully from hunt %s\n", argv[3], argv[2]);
            err[sizeof(err)] = '\0';
            write(1, err, strlen(err));
        }
        else
        {
            snprintf(err, sizeof(err) - 1, "Error at removing treasure %s from hunt %s\n", argv[3], argv[2]);
            err[sizeof(err)] = '\0';
            write(1, err, strlen(err));
        }
    }
    else
    if (strcmp(argv[1], "remove_hunt") == 0)
    {
        char question[MAX_BUF];
        snprintf(question, sizeof(question) - 1, "Are you sure you want to delete hunt %s?\n [y/n]  ", argv[2]);
        err[sizeof(question)] = '\0';
        write(1, question, strlen(question));
        if (read(0, &yes_no, 1) == -1) 
            perror("read failed");
        if (yes_no == 'y')
        {
            check = removeHunt(argv[2]);
            if (check == 0)
            {
                snprintf(err, sizeof(err) - 1, "Hunt %s removed successfully\n", argv[2]);
                err[sizeof(err)] = '\0';
                write(1, err, strlen(err));
            }
            else
            {
                snprintf(err, sizeof(err) - 1, "Error at removing hunt %s\n", argv[2]);
                err[sizeof(err)] = '\0';
                write(1, err, strlen(err));
            }
        }
        else
            write(1, "Operation terminated\n", strlen("Operation terminated\n"));
            
    }
    else
        write(1, "Command not recognized\n", strlen("Command not recognized\n"));
    return 0;
}