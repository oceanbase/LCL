#ifndef TELNET_H
#define TELNET_H

#include <string>
#include <vector>
#include <atomic>

class NetWork;
class Commander
{
public:
	Commander(NetWork &n);
	Commander(const Commander &) = delete;
	Commander(const Commander &&) = delete;
	Commander &operator=(const Commander &) = delete;
	Commander &operator=(const Commander &&) = delete;
	virtual ~Commander();
	void loop();
	void stop();

private:
	void apply(const std::string &);
	std::string cmd_buffer;
	int listenfd;
	NetWork &net;
	std::atomic<bool> running;
};
#endif
