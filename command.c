#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int command_exec(char *commandline, char *output, int size)
{
    FILE *fp;
    const int buff_len = 1024;
    char *buff[buff_len];
    fp = popen(commandline, "r");
    if (fp == NULL)
    {
        printf("open %s fail\n", commandline);
		return -1;

    }
    while(!feof(fp))
    {
        if (fgets(buff, buff_len, fp))
        {
            strcat(output, buff);
        }
    }
    
    pclose(fp);
    return 0;
    
}

