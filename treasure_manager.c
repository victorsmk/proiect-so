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
    snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt");
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
        printf("Nu exista hunt-ul %s\n", huntID);
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
            printf("Hunt name: %s, Total size: %ld bytes, Last modification: %s\n", huntID, st.st_size, buff);
            int bytes_read;
            while ((bytes_read = read(in, buff, sizeof(buff))))
                write(1, buff, bytes_read); //1 represents the stdout, if necessary will change output to a different file
            char logp[256];
            snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt");
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
                    printf("%s\n", line);
                    for (int i = 0; i < 4; i++) 
                    {
                        line = strtok(NULL, "\n");
                        if (line) 
                            printf("%s\n", line);
                    }
                    break; //Also assumes there is only one treasure with a given ID
                }          //If there can be multiple, just remove the break
            }
            line = strtok(NULL, "\n");
        }
        if (!found)
            printf("Treasure with ID %s not found in hunt %s.\n", treasureID, huntID);
        else
        {
            char logp[256];
            snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt");
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
    char filep[300], logp[300];
    while ((file = readdir(dirp)) != NULL) //Works with one but also multiple files in the same directory
    {
        if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) //Ignores the two default subdirectories of every directory
            continue;
        snprintf(filep, sizeof(filep), "%s/%s", huntID, file->d_name);
        snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt");
        unlink(filep);
        unlink(logp);
    }
    closedir(dirp);
    rmdir(huntID);
    return 0;
}

int removeTreasure(char *huntID, char *treasureID) 
{
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
    snprintf(logp, sizeof(logp), "%s/%s", huntID, "logged_hunt");
    int flog = open(logp, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (!flog)
        return -1;
    char log[MAX_BUF];
    int len = snprintf(log, sizeof(log), "removed treasure %s from this hunt\n", treasureID);
    write(flog, log, len);
    close(flog);
    return 0;
}

//Dont think its necessary to make the printing functions return anything on success,
//taking into account the fact that they will print information on success and not print on failure.
//However, if the functions will be changed to write in a different file,
//maybe also make them return some values

//For the logged_hunt maybe also add the time of each command
//Also is it just the attempted operation? Or does it have to be successful? (for now, it has to be successful)
//Maybe also make the functions return different values depending on type of error

int main(int argc, char **argv)
{
    char yes_no;
    int check = 1;
    if (strcmp(argv[1], "add") == 0)
    {
        check = addTreasure(argv[2], argv[3]);
        if (check == 0)
            printf("Treasure added successfully to hunt %s\n", argv[2]);
        else
            printf("Error at adding treasure to hunt %s\n", argv[2]);
    }
    if (strcmp(argv[1], "list") == 0)
        listHunt(argv[2]);
    if (strcmp(argv[1], "view") == 0)
        viewTreasure(argv[2], argv[3]);
    if(strcmp(argv[1], "remove_treasure") == 0)
        {
            check = removeTreasure(argv[2], argv[3]);
            if(check == 0)
                printf("Treasure %s removed successfully from hunt %s\n", argv[3], argv[2]);
            else
                printf("Error at removing treasure %s from hunt %s\n", argv[3], argv[2]);
        }
    if (strcmp(argv[1], "remove_hunt") == 0)
    {
        printf("Are you sure you want to delete hunt %s?\n [y/n]  ", argv[2]);
        scanf("%c", &yes_no);
        if (yes_no == 'y')
        {
            check = removeHunt(argv[2]);
            if (check == 0)
                printf("Hunt %s removed successfully\n", argv[2]);
            else
                printf("Error at removing hunt %s\n", argv[2]);

        }
        else
            printf("Operation terminated\n");
    }
    return 0;
}