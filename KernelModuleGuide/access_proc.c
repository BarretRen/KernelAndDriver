//使用IO系统调用访问/proc中指定文件，获取相关信息
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc ,char **argv)
{
    int fd=-1;
    char buf[500]={0};
    if(argc!=2)
    {
        printf("eg: %s -d|-v\n",argv[0]);
        return -1;
    }
    if(!strcmp(argv[1],"-v"))
    {
        fd=open("/proc/version",O_RDONLY);
        read(fd,buf,sizeof(buf));
        printf("结果:\n%s\n",buf);
    }
    else if(!strcmp(argv[1],"-d"))
    {
        fd=open("/proc/devices",O_RDONLY);
        read(fd,buf,sizeof(buf));
        printf("结果:\n%s\n",buf);
    }
}
