all:
	g++ -std=c++14 oClientV7.cpp connectionV2.cpp Octo.cpp -lpthread -o client
	g++ -std=c++14 oServerV5.cpp connectionV2.cpp Octo.cpp -lpthread -o server
