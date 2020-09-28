//second cdev的用户空间测试程序
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;
    int counter = 0;
    int old_counter = 0;

    /* 打开 /dev/second 设备文件 */
    fd = open("/dev/second", O_RDONLY);
    if (fd != -1)
    {
        while (1)
        {
            read(fd, &counter, sizeof(unsigned int)); /* 读目前经历的秒数 */
            if (counter != old_counter)
            {
                printf("seconds after open /dev/second :%d\n", counter);
                old_counter = counter;
            }
        }
    }
    else
    {
        printf("Device open failure\n");
    }
}