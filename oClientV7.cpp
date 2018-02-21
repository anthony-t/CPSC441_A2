#include <cstdlib>
#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <vector>
#include <thread>
#include <string>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include "connection.h"
#include "Octo.h"

using namespace std::chrono_literals;

std::condition_variable cv_box[OCTOLEGS_NUM * 2];
std::mutex mutex_box[OCTOLEGS_NUM * 2];
bool ack_box[OCTOLEGS_NUM * 2];

int state;

/*
*Send an octoleg with a specified length and sequence number
*Returns: void
*/
void sendOctoleg(char * message, int message_length, clientSocket * client_socket, char seq_num) {

	while (1) {
		client_socket->send((char *)message, message_length);

		std::cout << "SEQ Num Sent: " << (int)seq_num << " Bytes sent: " << message_length << " First byte: " << (int)message[1] << std::endl;

		//If ACK has not been recieved within 1000ms, resend 
		std::unique_lock<std::mutex> lock(mutex_box[seq_num]);
		if (cv_box[seq_num].wait_for(lock, 1000ms, [seq_num] {return (ack_box[seq_num] == true); })) {
			return;
		}
	}
}

/*
 *Read from a file stream. Fills buffer with a single ' ' character if we are at the end of the file. A buffer
 *containing the read data otherwise. Returns the number of bytes filled in the buffer
*/
template <typename T>
int padRead(char * buffer, int read_size, T& in_stream) {
	if (in_stream.peek() == EOF) {
		buffer[0] = ' ';
		return 1;
	}
	else {
		in_stream.read(buffer, read_size);
		return in_stream.gcount();
	}
}

/*
*Send an octoblock specified as a stream from a specified socket
*Returns: void
*/
template <typename T>
void sendStream(T& in_stream, int stream_size, clientSocket * client_socket) {

	//Get a list of how large each octoblock should be 
	std::vector<int> octoblock_list;
	getList(stream_size, &octoblock_list, OCTOLEGS_NUM, OCTOBLOCK_SIZE);

	//Buffers to store octolegs. Add 1 byte for the sequence number
	char octo_buffers[OCTOLEGS_NUM][OCTOLEG_SIZE + 1] = { 0 };

	std::thread octo_threads[OCTOLEGS_NUM];

	//Send octoblocks one at a time 
	char ack[1];
	for (std::vector<int>::iterator it = octoblock_list.begin(); it != octoblock_list.end(); it++) {
		int acks_recieved = 0;

		//Send octolegs
		for (char seq_num = (OCTOLEGS_NUM * state); seq_num < OCTOLEGS_NUM*(1+state); seq_num = seq_num + 1) {
			//Set ACK for seq num to false
			ack_box[seq_num] = false;

			char seq_num_safe = seq_num % OCTOLEGS_NUM;

			//The first byte is the sequence number
			octo_buffers[seq_num_safe][0] = seq_num;

			//Read in the file 
			int bytes_read = padRead(octo_buffers[seq_num_safe] + 1, *it, in_stream);

			octo_threads[seq_num_safe] = std::thread(sendOctoleg, octo_buffers[seq_num_safe], bytes_read + 1, client_socket, seq_num);
		}
		//Wait for ACKs
		while (acks_recieved < OCTOLEGS_NUM) {
			client_socket->recieve(ack, sizeof(ack));

			char seq_num = ack[0];

			if (!ack_box[seq_num]) {
				acks_recieved++;
				ack_box[seq_num] = true;

				//Notify the thread to return
				cv_box[seq_num].notify_one();

				std::cout << "Acks recieved: " << (int) seq_num << std::endl;
			}
		}
		for (char i = 0; i < sizeof(octo_threads)/sizeof(octo_threads[0]); i = i + 1) {
			octo_threads[i].join();
		}

		//Flip set of sequence numbers
		state = (state + 1) % 2;
	}
}

/*
*Send an integer
*Returns: void
*/
void sendInt(int value, clientSocket * client_socket) {
	//Turn the integer into a string and send the string representation
	std::string num = std::to_string(value);

	//An integer must consist of only a single octoblock.  Therefore, we must pad the string 
	//so that it is a multiple of 8 and so will not be split into more than one octoblock
	//when sent. If the string is a multiple of 8, pad with 8 spaces.
	num.append(OCTOLEGS_NUM - (num.size() % OCTOLEGS_NUM), ' ');

	std::stringstream ss(num);

	sendStream(ss, num.size(), client_socket);
}

/*
*Send a string as octoblocks
*/
void sendString(std::string str, clientSocket * client_socket) {
	std::stringstream ss(str);

	ss.seekg(0, std::ios::end);
	int ss_size = ss.tellg();
	ss.seekg(0, std::ios::beg);

	//Send the size of the string, then the string
	sendInt(ss_size, client_socket);
	sendStream(ss, ss_size, client_socket);
}

void sendFile(char * filename, clientSocket * client_socket) {

	//Get a reference to the file
	std::ifstream file_stream(filename, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
	int file_size = file_stream.tellg();
	file_stream.seekg(0, std::ios::beg);

	sendString(std::string(filename), client_socket);
	sendInt(file_size, client_socket);
	sendStream(file_stream, file_size, client_socket);

	file_stream.close();
}

int main(int argc, char ** argv)
{
	if (argc < 4) {
		std::cout << "Usage: oClient [filename] [dest port] [dest ip]" << std::endl;
		return 1;
	}

	char * filename = argv[1];
	int dest_port = atoi(argv[2]);
	char * dest_ip = argv[3];

	//0: Seq num 0 - 7
	//1: Seq num 8 - 15
	int state = 0;

	clientSocket client_socket(dest_port, dest_ip);
	sendFile(filename, &client_socket);
	client_socket.close_socket();

	return 0;



}