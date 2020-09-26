//测试10字符驱动的异步通知功能，接受SIGIO信号处理
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void signal_handler(int signo)
{
    printf("release a ginal from globalfifo, signum:%d\n", signo);
}

int main()
{
    int fd,flags;
    fd = open("/dev/globalfifo", O_RDWR, S_IRUSR | S_IWUSR);
    if (fd != -1)
    {
        //注册信号处理函数
        signal(SIGIO, signal_handler);
        //设置本进程为fd的拥有者，这样内核才能把信号发到此进程
        fcntl(fd, F_SETOWN, getpid());
        flags = fcntl(fd, F_GETFL);//获取设备当前配置
        fcntl(fd, F_SETFL, flags | FASYNC);//添加异步配置

        while(1)
            sleep(100);
    }
    else
        printf("open device failed\n");
    return 0;
}
