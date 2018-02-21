#include <vector>
#include <streambuf>

#define OCTOLEGS_NUM 8
#define OCTOLEG_SIZE 1111
#define OCTOBLOCK_SIZE OCTOLEGS_NUM * OCTOLEG_SIZE

/*
struct charbuffer : std::streambuf {
	charbuffer(char * begin, char * end) {
		this->setg(begin, begin, end);
	}
};*/

void getList(int n, std::vector<int> * v, int division, int max);