#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

class MPServer {
private:
    std::vector<int> sockets;


public:
    void listenForClients(unsigned int time_secs) {

    }


    template <typename T>
    void sendToAll(const T& message) {

    }

    void messageReceived(unsigned int source, const char* message) = 0;
};
