#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <string>
#include <iostream>

class udpSocket {
	public:
		udpSocket();
		int send(char * buffer, int buffer_len, struct sockaddr_in dest_addr);
		int recieve(char * buffer, int buffer_len, struct sockaddr_in &src_addr, int &len);
		void bindSocket(struct sockaddr_in server_addr);
		void close_socket();

	protected:
		void createSocket();
		int sock_id;
};

class clientSocket : public udpSocket {
	public:
		clientSocket(int port_number, char * server_addr);
		clientSocket(struct sockaddr_in addr);
		int send(char * buffer, int buffer_len);
		int recieve(char * buffer, int buffer_len);
	private:
		void setServerAddr(int port_number, char * server_addr);
		sockaddr_in server_addr;
};

class serverSocket : public udpSocket {
	public:
		serverSocket(int port_number);
	private:
		sockaddr_in getServerAddr(int port);
		
		
};