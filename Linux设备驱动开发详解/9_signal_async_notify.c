//使用信号实现异步通知的app code
//当用户输入一串字符后，标准输入设备释放SIGIO信号，这个信号与对应的处理函数
//signalt_handler()得以执行，并将用户输入打印出来
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LEN 100

void signal_handler(int signo)
{
    char data[MAX_LEN];
    int len;
    //读取STDIN_FILENO上的内容并打印
    len = read(STDIN_FILENO, &data, MAX_LEN);
    data[len] = '\0';
    printf("input available:%s\n", data);
}

int main()
{
    int flags;
    //注册信号处理函数
    signal(SIGIO, signal_handler);
    //设置本进程为STDIN_FILENO的拥有者，这样内核才能把信号发到此进程
    fcntl(STDIN_FILENO, F_SETOWN, getpid());
    flags = fcntl(STDIN_FILENO, F_GETFL);//获取设备当前配置
    fcntl(STDIN_FILENO, F_SETFL, flags | FASYNC);//添加异步配置

    while(1);
    return 0;
}
