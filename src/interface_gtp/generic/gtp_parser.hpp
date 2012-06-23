#ifndef __GTP_PARSER_HPP
#define __GTP_PARSER_HPP

#include <iostream>
#include <string>
#include "assert.h"

#include "gtp_parser_callbacks.hpp"

/*
    GTPParser decodes GTPv2-formatted go communications from an input stream
    and sends the data to the appropriate callback. It then outputs the response
    from that callback on the output stream.
    
    Typical use case: connected by stdin and stdout to KGS (internet go) client.
*/

class GTPParser {
    private:
        typedef std::map<std::string, GTPCallback*> CMType;
        CMType callback_map;
        
        GTPCallbackStatic cb_name, cb_protocol_version, cb_version;
        GTPCallbackListCommands cb_list_commands;
        GTPCallbackKnownCommand cb_known_command;        

        void handleSplitLine(const std::vector<std::string>& splitLine, std::ostream &out);
        
    public:

        GTPParser(const std::string& engine_name, const std::string& engine_version);
        
        /* Note: GTPParser will NOT deallocate the callback on destruction */
        void addCommandCallback(std::string command, GTPCallback* callback);
    
        void run(std::istream &in, std::ostream &out);
};

inline std::vector<std::string> splitByCharacter(const std::string& s, const char delim) {
    std::vector<std::string> ret;
    ret.push_back("");

    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.at(i);
        if (c == delim) {
            ret.push_back("");
        } else {
            ret[ret.size() - 1].push_back(c);
        }
    }
    return ret;
}


#endif

