#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


/*
#define EVENTS_NUMBER 12
#define DELAY_CUTLINE 1


char *values[EVENTS_NUMBER][3] = {
    {"3", "57", "1152"},//开始接触设备的唯一标识号
    {"3", "53", ""},//X
    {"3", "54", ""},//Y
    {"3", "48", "123"},//接触面椭圆长轴，非必需
    {"3", "50", "8"},//未知
    {"1", "330", "1"},//按下
    {"3", "0", "0"},//SYN_REPORT，这里简单理解为与io的flush类似
    {"3", "1", "-2147483647"},//未知
    {"0", "0", "0"},
    {"3", "57", "-2147483647"},
    {"1", "330", "0"},//释放
    {"0", "0", "0"},   
};


int work(int fd, struct input_event eventIn, char *data[]) {
    memset(&eventIn, 0, sizeof(eventIn));
    fprintf(stderr, "%s, %s, %s\n", data[0], data[1], data[2]);
    eventIn.type = atoi(data[0]);
    eventIn.code = atoi(data[1]);
    eventIn.value = atoi(data[2]);
    ssize_t ret = write(fd, &eventIn, sizeof(eventIn));

    if(ret < (ssize_t) sizeof(eventIn)) {
        fprintf(stderr, "write event failed, %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}


int main(int argc, char *argv[]) {
    int fd;
    ssize_t ret;
    int version;
    struct input_event events[EVENTS_NUMBER];
    
    if(argc != 4) {
        fprintf(stderr, "use: %s X Y DURATION\n", argv[0]);
        return 1;
    }

    fd = open("/dev/input/event0", O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", argv[optind], strerror(errno));
        return 1;
    }

    values[1][2] = argv[1];
    values[2][2] = argv[2];

    if (ioctl(fd, EVIOCGVERSION, &version)) {
        fprintf(stderr, "could not get driver version for %s, %s\n", argv[optind], strerror(errno));
        return 1;
    }
    

    
    for (int i = 0; i < EVENTS_NUMBER; i++) {
        
        int ret = work(fd, events[i], values[i]);
        if (ret == -1) {
            fprintf(stderr, "return value: %d\n", ret);
            return ret;
        }
    }

    return 0;
}
*/

#define touch_panel  "/dev/input/event0"
#define EVENTS_NUMBER 12
#define DELAY_CUTLINE 1


int values[EVENTS_NUMBER][3] = {
    {3, 57, 1152},//开始接触设备的唯一标识号
    {3, 53, 0},//X
    {3, 54, 0},//Y
    {3, 48, 123},//接触面椭圆长轴，非必需
    {3, 50, 8},//未知
    {1, 330, 1},//按下
    {3, 0, 0},//SYN_REPORT，这里简单理解为与io的flush类似
    {3, 1, 0x7fffffff},//未知
    {0, 0, 0},
    {3, 57, 0x7fffffff},
    {1, 330, 0},//释放
    {0, 0, 0},   
};


int work(int fd, struct input_event eventIn, int data[]) {
    memset(&eventIn, 0, sizeof(eventIn));
    fprintf(stderr, "%d, %d, %d\n", data[0], data[1], data[2]);
    eventIn.type = data[0];
    eventIn.code = data[1];
    eventIn.value = data[2];
    ssize_t ret = write(fd, &eventIn, sizeof(eventIn));

    if(ret < (ssize_t) sizeof(eventIn)) {
        fprintf(stderr, "write event failed, %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}


char *click_error()
{

}
char *click_OK()
{

    
}

int click(int x, int y)
{
    int fd;
    ssize_t ret;
    struct input_event events[EVENTS_NUMBER];
        
    fd = open(touch_panel, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", touch_panel, strerror(errno));
        return 1;
    }

    values[1][2] = x;
    values[2][2] = y;

    for (int i = 0; i < EVENTS_NUMBER; i++) {

        system("sleep 0.05");
        int ret = work(fd, events[i], values[i]);
        if (ret == -1) {
            fprintf(stderr, "return value: %d\n", ret);
            return ret;
        }
    }
    
    return 0;

}


