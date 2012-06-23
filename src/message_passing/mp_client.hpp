// Based upon http://gnosis.cx/publish/programming/sockets.html

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

class MPClient {
private:
    int sock = 0;

public:
    MPClient(const std::string& server_ip, const int port_number) {
        sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock < 0) {
            std::cerr << "Failed to create socket";
            assert(false); abort();
        }

        sockaddr_in server_details;
        memset(&server_details, 0, sizeof(server_details));
        server_details.sin_family = AF_INIT;
        server_details.sin_addr.s_addr = inet_addr(argv[1]);
        server_details.sin_port = htons(port_number);

        if (connect(sock, &server_details, sizeof(server_details)) < 0) {
            std::cerr << "Failed to connect to " << server_ip;
            assert(false); abort();
        }
    }

    void sendMessage(const std::string& msg) {

    }
};
