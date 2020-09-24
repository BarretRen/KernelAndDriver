//netlink套接字监听热插拔事件
#include <linux/netlink.h>
#include <sys/poll.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

static void die(char* s)
{
	write(2, s, strlen(s));
	exit(1);
}

int main()
{
	struct sockaddr_nl nls;
	struct pollfd pfd;
	char buf[512];
	memset(&nls, 0, sizeof(struct sockaddr_nl));
	
	pfd.events = POLLIN; //设置监听插入事件
	pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (pfd.fd == -1)
		die("not root\n");
	
	nls.nl_family = AF_NETLINK;//设置netlink监听，不是普通socket的ip类型
	nls.nl_pid = getpid();
	nls.nl_groups = -1;
	if (bind(pfd.fd, (void*)&nls, sizeof(struct sockaddr_nl)))
		die("bind socket failed\n");
	
	while(poll(&pfd, 1, -1))
	{
		int i, len = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
		if (len == -1)
			die("recv nothing\n");
		
		i = 0;
		while(i < len)
		{
			printf("%s\n", buf+i);
			i+= strlen(buf+i) + 1;
		}
	}
	die("poll\n");
	return 0;
}