#include "gtp_parser.hpp"

namespace {

std::string cleanString(const std::string& s) {
    std::string ret = "";
    ret.reserve(s.length());

    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.at(i);
        switch (c) {
            case '#': return ret; // comment

            case '\r': break;
            case '\t': ret.push_back(' '); break;

            default:
                ret.push_back(c);
        }
    }

    return ret;
}

} // end anonymous namespace

void GTPParser::handleSplitLine(const std::vector<std::string>& splitLine, std::ostream& out) {
    assert(splitLine.size() > 0);
    
    std::vector<std::string> args;
    std::string cmd_id, cmd;
    
    //check  first part of the line is the command id (this is optional)
    if (isInt(splitLine[0])) {
        cmd_id = splitLine[0];
        cmd = splitLine[1];
        
        // we get rid of the command id part of the line, but remember its value in cmd_id
        args.resize(splitLine.size() - 2);
        for (unsigned int i = 2; i < splitLine.size(); i++) {
            args[i - 2] = splitLine[i];
        }
    } else {
        cmd_id = "";
        cmd = splitLine[0];

        args.resize(splitLine.size() - 1);

        for (unsigned int i = 1; i < splitLine.size(); i++) {
            args[i - 1] = splitLine[i];
        }
    }
    

    GTPResponse resp = GTPResponse(GTP_FAILURE, "unknown command: " + cmd);    
    CMType::iterator it = callback_map.find(cmd);
    if (it != callback_map.end()) {
        resp = it->second->callback(args);
    }

    resp.setId(cmd_id);

    // write our program's response to stderr
    // std::cerr << resp.toString();

    // output response
    out << resp.toString();
}

GTPParser::GTPParser(const std::string& engine_name, const std::string& engine_version) :
    cb_name(engine_name),
    cb_protocol_version("2"),
    cb_version(engine_version),
    cb_list_commands(callback_map),
    cb_known_command(callback_map)
{
    addCommandCallback("protocol_version", &cb_protocol_version);
    addCommandCallback("name",             &cb_name);
    addCommandCallback("version",          &cb_version);
    addCommandCallback("known_command",    &cb_known_command);
    addCommandCallback("list_commands",    &cb_list_commands);
}

void GTPParser::addCommandCallback(std::string command, GTPCallback* callback) {
    callback_map[command] = callback;
}

void GTPParser::run(std::istream& in, std::ostream& out) {
    std::string line;

    while (!in.eof()) {
        std::getline(in, line);

        //std::cerr << "Read line: " << line << "\n";
        line = cleanString(line);
        //std::cerr << "Cleaned line was: " << line << "\n";

        if (line == "") {
            std::cerr << "Ignoring blank line" << "\n";
        } else {
            //std::cerr << "Line not blank" << "\n";

            std::vector<std::string> splitLine = splitByCharacter(line, ' ');
            handleSplitLine(splitLine, out);
        }
    }
}
