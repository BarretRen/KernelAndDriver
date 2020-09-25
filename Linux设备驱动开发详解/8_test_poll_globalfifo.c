//使用select测试一下编写的poll_golbalfifo驱动的poll函数是否OK
#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20

void main(void)
{
    int fd, num;
    char rd_ch[BUFFER_LEN];
    fd_set rfds, wfds; /* 读 / 写文件描述符集 */

    /* 以非阻塞方式打开 /dev/globalfifo 设备文件 */
    fd = open("/dev/globalfifo", O_RDONLY | O_NONBLOCK);
    if (fd != -1) 
    {
        /* FIFO 清 0 */
        if (ioctl(fd, FIFO_CLEAR, 0) < 0)
            printf("ioctl command failed\n");

        while (1)
        {
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_SET(fd, &rfds);
            FD_SET(fd, &wfds);

            select(fd + 1, &rfds, &wfds, NULL, NULL);
            /* 数据可获得 */
            if (FD_ISSET(fd, &rfds))
                printf("Poll monitor:can be read\n");
            /* 数据可写入 */
            if (FD_ISSET(fd, &wfds))
                printf("Poll monitor:can be written\n");
        }
    }
    else
    {
        printf("Device open failure\n");
    }
}
