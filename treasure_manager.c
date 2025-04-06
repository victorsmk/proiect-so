#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>

void addTreasure (char *huntID)
{
    struct stat dir;
    if (stat(huntID, &dir) != 0) 
        mkdir(huntID);       
}

int main(int argc, char **argv)
{
    if (strcmp(argv[1], "add") == 0)
    {
        addTreasure(argv[1]);
    }
    return 0;
}