#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_BUF 4096
#define MAX_LINE 256
#define MAX_USERS 100

typedef struct 
{
    char username[50];
    int total_value;
} UserScore;

UserScore scores[MAX_USERS];
int score_count = 0;

void add_score(char* user, int value) 
{
    for (int i = 0; i < score_count; i++) 
    {
        if (strcmp(scores[i].username, user) == 0) 
        {
            scores[i].total_value += value;
            return;
        }
    }
    // New user
    strcpy(scores[score_count].username, user);
    scores[score_count].total_value = value;
    score_count++;
}

int main(int argc, char **argv) //Calculates the user scores for any given hunt
{
    if (argc != 2)
    {
        printf("incorrect num. of arguments\n");
        exit(-1);
    }
    char filep[MAX_BUF];
    snprintf(filep, sizeof(filep), "%s/treasures", argv[1]);
    int in = open(filep, O_RDONLY);
    if (in < 0) 
    {
        perror("error opening file");
        exit(-1);
    }
    char buff[MAX_BUF];
    char line[MAX_LINE];
    int line_len = 0;
    char current_user[50] = "";
    int current_value = 0;
    int bytes_read;
    while ((bytes_read = read(in, buff, sizeof(buff))) > 0) 
    {
    for (int i = 0; i < bytes_read; i++) {
        if (buff[i] == '\n') 
        {
            line[line_len] = '\0';
            // Process the line
            if (strncmp(line, "User:", 5) == 0) 
                sscanf(line, "User: %s", current_user);
            else 
            if (strncmp(line, "Value:", 6) == 0) 
            {
                sscanf(line, "Value: %d", &current_value);
                if (strlen(current_user) > 0) 
                    add_score(current_user, current_value);
            }

            line_len = 0; // reset for next line
        } 
        else 
        if (line_len < MAX_LINE - 1) 
            line[line_len++] = buff[i];
        }
    }
    close(in);
    char num[32];
    write(1, "User Scores:\n", strlen("User Scores:\n"));
    for (int j = 0; j < score_count; j++) 
    {
        write(1, scores[j].username, strlen(scores[j].username));
        write(1, ": ", strlen(": "));
        int len = snprintf(num, sizeof(num), "%d", scores[j].total_value);
        write(1, num, len);
        write(1, "\n", strlen("\n"));
    }
    return 0;
}
