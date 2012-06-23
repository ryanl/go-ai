#include <iostream>
#include <string>

#include <pstreams/pstream.h> // for process control functions
//#include <boost/thread.hpp>

#include "gtp_parser.hpp"

class ExternalGTPProcess {
    std::string command_line;
    redi::pstream child;

public:
    ExternalGTPProcess(const std::string& _command_line) :
        command_line(_command_line),
        child(_command_line)
    {}

    const std::string& getCommandLine() {
        return command_line;
    }

    GTPResponse sendGTPQuery(const std::string& command) {
        std::cerr << "Sent '" << command_line << "': " << command << "\n";
        child << command << "\n" << std::flush; // flush is essential

        std::string response, tmp;
        for (;;) {
            getline(child, tmp);
            if (tmp == "") break;

            if (response != "") response += "\n";
            response += tmp;
        }

        //std::cerr << "Received response: " << response << "\n";
        return GTPResponse::fromString(response);
    }


    void sendGTPCommand(const std::string& command) {
        GTPResponse response = sendGTPQuery(command);

        if (!response.isSuccess() || response.getMsg() != "") {
            std::cout << "Expected response: = (from " << command_line << ")\n"
                      << "Received: " << response.toString() << "\n";
            exit(1);
        }
    }
};
