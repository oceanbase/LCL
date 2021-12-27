#include "Commander.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>

#include "easylogging++.h"
#include "NetWork.h"

Commander::Commander(NetWork &n) : net(n), running(true)
{
	struct sockaddr_in sockaddr;

	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockaddr.sin_port = htons(1234); //This port could be adjusted.

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listenfd != -1);

	int ret = bind(listenfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	if (ret == -1)
	{
		LOG(INFO) << "!!error " << strerror(errno);
	}
	assert(ret != -1);
	ret = listen(listenfd, 1);
	if (ret == -1)
	{
		LOG(INFO) << "!!error " << strerror(errno);
	}
	assert(ret != -1);

	LOG(INFO) << "Commander is waiting for the Command,listening on 1245 port.";
}

Commander::~Commander()
{
	close(listenfd);
}

void Commander::loop()
{
	int connfd, n;
	int buffer_size = 50;
	char buff[buffer_size];
	int sendbuffer_size = 50;
	char sendbuff[buffer_size];
	while (running)
	{
		if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1)
		{
			LOG(INFO) << "accpet socket error: " + std::string(strerror(errno));
			continue;
		}
		while (running)
		{
			n = recv(connfd, buff, buffer_size - 1, 0);
			if (n == -1)
			{
				cmd_buffer.clear();
				break;
			}
			send(connfd, buff, n, 0);
			buff[n] = '\0';
			LOG(INFO) << std::string(buff);
			if (std::string(buff) == "stop")
			{
				LOG(INFO) << "stop command ";
				running = false;
				break;
			}
			apply(buff);
		}
	}
}

void Commander::stop()
{
	running = false;
}

void Commander::apply(const std::string &cmd)
{
	if (cmd == "printG")
	{
		net.PrintGraph();
	}
	if (cmd == "LCL_Periodic")
	{
		net.LCL_Periodic();
	}
	if (cmd == "LCL_Once")
	{
		net.LCL_Once(true);
	}
	if (cmd == "ALL_TO_DB_easy")
	{
		net.ALL_TO_DB_easy();
	}
	if (cmd == "ALL_TO_DB")
	{
		net.ALL_TO_DB();
	}
}
