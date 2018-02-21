#include "connection.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Octo.h"

using namespace std;
int state;

std::string trim(std::string s) {
	int last_char_ind = s.find_last_not_of(' ');
	return s.substr(0, last_char_ind);
}

int recieveOctoblock(serverSocket * server_socket, char octoblock[OCTOBLOCK_SIZE]) {
	//Buffers to store octolegs. Add 1 byte for the sequence number
	char octo_buffers[OCTOLEGS_NUM][OCTOLEG_SIZE + 1] = { 0 };

	int octoleg_size;
	int octolegs_seen_num = 0;
	bool octolegs_seen[OCTOLEGS_NUM] = { false };

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	//Recieve octolegs 
	do{
		//Add one byte for the sequence number header
		char octoleg[OCTOLEG_SIZE + 1];

		octoleg_size = server_socket->recieve(octoleg, sizeof(octoleg), client_addr, client_addr_len) - 1;

		char seq_num = octoleg[0];

		//Check that the sequence number is in the expected range 
		if (seq_num < (OCTOLEGS_NUM * state) || seq_num >= OCTOLEGS_NUM*(1 + state)) {
			continue;
		}

		cout << "SEQ Num Recieved: " << (int)seq_num << " Bytes recieved: " << octoleg_size << "First byte: " << (int)octoleg[1] << endl;

		char arr_safe_seq_num = seq_num % OCTOLEGS_NUM;

		//Only save the data if we have not see it before
		if (octolegs_seen[arr_safe_seq_num] == false) {
			//Contents of octoleg are after the first byte 
			memcpy(octo_buffers[arr_safe_seq_num], octoleg + 1, octoleg_size);

			octolegs_seen[arr_safe_seq_num] = true;
			octolegs_seen_num++;

			sleep(1);
		}

		//Send the ACK
		server_socket->send(octoleg, 1, client_addr);

	} while (octolegs_seen_num < OCTOLEGS_NUM);

	//Put octolegs in one contiguous array (octoblock)
	for (int i = 0; i < sizeof(octo_buffers)/sizeof(octo_buffers[0]); i++) {
		memcpy(octoblock + (octoleg_size*i), octo_buffers[i], octoleg_size);
	}

	//Flip set of expected sequence numbers
	state = (state + 1) % 2;

	return octoleg_size * OCTOLEGS_NUM;
}

/*
 *Recieve data to a stream
 *Returns: void
*/
template <typename T>
void recieveStream(serverSocket * server_socket, T & out_stream, int data_size) {
	vector<int> octoblock_list;
	getList(data_size, &octoblock_list, OCTOLEGS_NUM, OCTOBLOCK_SIZE);

	int total_bytes_read = 0;

	for (vector<int>::iterator it = octoblock_list.begin(); it != octoblock_list.end(); it++) {
		char octoblock[OCTOBLOCK_SIZE] = "";
		int bytes_read = recieveOctoblock(server_socket, octoblock);

		//If more bytes are read than expected, then there is likely padding that needs to be removed
		if (total_bytes_read + bytes_read <= data_size) {
			out_stream.write(octoblock, bytes_read);
		}
		else{
			out_stream.write(octoblock, data_size - total_bytes_read);
		}

		total_bytes_read += bytes_read;
	}
}

/*
* Recieve and integer sent as a single octoblock 
* Returns: The integer recieved
*/
int recieveInt(serverSocket * server_socket) {
	char num[OCTOBLOCK_SIZE] = "";
	recieveOctoblock(server_socket, num);

	//Remove padding spaces
	std::string n(num);
	n = trim(n);

	return std::stoi(num);
}

/*
 * Recieve a string
 * Returns: The string recieved
*/
string recieveString(serverSocket * server_socket) {
	int string_size = recieveInt(server_socket);

	stringstream ss;
	recieveStream(server_socket, ss, string_size);
	return ss.str();
}

void recieveFile(serverSocket * server_socket) {

	string filename = recieveString(server_socket) + "1";
	cout << "File name: " << filename << endl;

	ofstream file_stream(filename.c_str(), fstream::out | fstream::binary | fstream::app);

	int filesize = recieveInt(server_socket);
	recieveStream(server_socket, file_stream, filesize);

	file_stream.close();

}

int main(int argc, char ** argv)
{
	if (argc < 2) {
		cout << "Usage: oServer [listen port]" << endl;
		return 1;
	}

	int port = atoi(argv[1]);

	state = 0;

	serverSocket server_socket(port);
	recieveFile(&server_socket);
	server_socket.close_socket();

	return 0;
}