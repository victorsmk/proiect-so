#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
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

//Where to get data for treasure?? (input file maybe for the moment?)
//Can you add the same treasure twice?

void addTreasure (char *huntID, char *inputFile)
{
    Treasure_t treasure;
    int in = open(inputFile, O_RDONLY);
    if (in)
    {
        struct stat st;
        if (stat(huntID, &st) != 0)  //Checks if file with the given name exists, if not makes a directory with given name
            mkdir(huntID, 0755);     //no point in checking if the file is a directory, since there are no directories in directories,
        DIR *dirp = opendir(huntID);    //and each treasure has to be inside a directory.
        if (dirp)
        {
            char buffer[MAX_BUF];
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
            if (out)
            {
                char outbuf[MAX_BUF];
                int len = snprintf(outbuf, sizeof(outbuf), "Treasure ID: %s\nUser: %s\nCoordinates: %s, %s\nClue: %s\nValue: %s\n", treasure.id, treasure.username, treasure.lat, treasure.lon, treasure.clue, treasure.value);
                write(out, outbuf, len);
                close(out);
            }
            closedir(dirp);
        }
        close(in);
    }
}

//Print at stdout or in a file? (for now, at stdout)

void listHunt(char *huntID)
{
    struct stat st;
    if (stat(huntID, &st) != 0) 
        printf("Nu exista hunt-ul cu id-ul dat\n");
    else
    {
        DIR *dirp = opendir(huntID); 
        if (dirp)
        {
            char filep[256];
            snprintf(filep, sizeof(filep), "%s/%s", huntID, "treasures");
            int in = open(filep, O_RDONLY);
            if (in)
            {
                stat(filep, &st);
                char buff[100];
                long int eet_time = st.st_mtim.tv_sec + 3*3600;  //adjusts time according to EET
                strftime(buff, sizeof(buff), "%D %T", gmtime(&eet_time));
                printf("Hunt name: %s, Total size: %ld bytes, Last modification: %s\n", huntID, st.st_size, buff);
                int bytes_read;
                while ((bytes_read = read(in, buff, sizeof(buff))))
                    write(1, buff, bytes_read); //1 represents the stdout, if necessary will change output to a different file
                close(in);
            }
            close(dirp);
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
        close(in);
    }
}


int main(int argc, char **argv)
{
    if (strcmp(argv[1], "add") == 0)
        addTreasure(argv[2], argv[3]);
    if (strcmp(argv[1], "list") == 0)
        listHunt(argv[2]);
    if (strcmp(argv[1], "view") == 0)
        viewTreasure(argv[2], argv[3]);
    return 0;
}