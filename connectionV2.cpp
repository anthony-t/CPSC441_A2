#include "connection.h"

udpSocket::udpSocket() {
	createSocket();
}

/*
*initSocket: create a socket 
*int port_number: the port number to connect the socket to
*Returns: void
*/
void udpSocket::createSocket() {
	sock_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_id < 0) {
		throw std::runtime_error("Could not create socket.");
	}
}

/*
Bind socket to listen to the incoming connections on the created server
Binding makes sure that the socket is bound to a specific port. Else, a random port number
would be assigned.  The client doesn't have to bind because the origin source port is not important
since the server will know the port once a message is recieved
Returns: void
*/
void udpSocket::bindSocket(struct sockaddr_in server_addr) {
	int status = bind(sock_id, (struct sockaddr *) &server_addr, sizeof(server_addr));

	if (status < 0) {
		throw std::runtime_error("Could not bind socket.");
	}
}

/*
* send: Send data from a buffer 
* Returns: number of bytes sent 
*/
int udpSocket::send(char * buffer, int buffer_len, struct sockaddr_in dest_addr) {
	int bytes_sent = sendto(sock_id, buffer, buffer_len, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));

	if (bytes_sent == -1) {
		throw std::runtime_error("Failed to send.");
	}

	return bytes_sent;
}


/*
*Recieve data from socket 
*Returns: number of bytes 
*/
int udpSocket::recieve(char * buffer, int buffer_len, struct sockaddr_in &src_addr, int &len) {
	int bytes_recieved = recvfrom(sock_id, buffer, buffer_len, 0, (struct sockaddr *) &src_addr, (socklen_t *)&len);

	if (bytes_recieved == -1) {
		throw std::runtime_error("Failed to recieve.");
	}

	return bytes_recieved;
}

/*
Close the udp socket
Returns: void
*/
void udpSocket::close_socket() {
	close(sock_id);
}

serverSocket::serverSocket(int port) :udpSocket() {
	struct sockaddr_in server_addr = getServerAddr(port);
	bindSocket(server_addr);
}

/*
*Set information about the server we want to connect to
*int port: port to recieve data on 
*Returns: the address of the socket
*/
struct sockaddr_in serverSocket::serverSocket::getServerAddr(int port) {

	// socket address used for the server
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	return server_addr;

}

clientSocket::clientSocket(int port_number, char * server_ip) : udpSocket() {
	setServerAddr(port_number, server_ip);
}

clientSocket::clientSocket(struct sockaddr_in addr) : udpSocket() {
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr = addr;	
	
	socklen_t len = sizeof(server_addr);
	if (getsockname(sock_id, (struct sockaddr *)&server_addr, &len) == -1)
		perror("getsockname");
	else
		printf("port number %d\n", ntohs(server_addr.sin_port));
}

/*
*Set information about the server we want to connect to
*char * server_addr: A string describing the address of the server
*int port_number: port number to send data from\
*returns: void
*/
void clientSocket::setServerAddr(int port_number, char * server_ip) {
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_number);

	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) == 0) {
		throw std::runtime_error("Could not convert from IPv4 to IPv6");
	}
}

int clientSocket::send(char * buffer, int buffer_len) {
	return udpSocket::send(buffer, buffer_len, server_addr);

}

int clientSocket::recieve(char * buffer, int buffer_len) {
	struct sockaddr_in dest_addr;
	int len = sizeof(dest_addr);
	return udpSocket::recieve(buffer, buffer_len, dest_addr, len);

}






