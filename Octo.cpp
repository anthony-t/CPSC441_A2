#include "Octo.h"

/*
*Get a list where each element represents an octoblock and the value of the element is the
*size of each octoleg in the octoblock
*Returns: Size of octoleg in each octoblock
*/
void getList(int n, std::vector<int> * v, int division, int max) {
	if (n == 0) {
		return;
	}
	else if (n <= division) {
		v->push_back(1);
		return;
	}
	else if (n >= max) {
		v->push_back(max / division);
		getList(n - max, v, division, max);
	}
	else {
		int piece_size = n / division;
		v->push_back(piece_size);
		getList(n % division, v, division, max);
	}
}
