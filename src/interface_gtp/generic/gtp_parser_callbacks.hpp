#ifndef __GTP_PARSER_CALLBACKS_HPP
#define __GTP_PARSER_CALLBACKS_HPP

#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <map>

enum GTPResponseType {
    GTP_SUCCESS,
    GTP_FAILURE,
    GTP_QUIT,
    GTP_INVALID_FORMAT // when reading stuff that isn't GTP it's set to this
};


inline bool isInt(const std::string& s) {
    if (s.length() == 0) return false;

    for (unsigned int i = 0; i < s.length(); i++) {
        if (s.at(i) < '0' || s.at(i) > '9') return false;
    }
    return true;
}


inline float stringToFloat(const std::string& s) {
    return atof(s.c_str());
}

inline unsigned int stringToInt(const std::string& s) {
    return atoi(s.c_str());
}

inline void inPlaceReverse(std::string& s) {
    unsigned int n = (s.length() - 1) / 2;
    for (unsigned int i = 0; i < n; i++) {
        char tmp = s[s.length() - n];
        s[s.length() - n] = s[i];
        s[i] = tmp;
    }
}

template <typename T>
inline std::string toString(T i) {
    std::ostringstream oss;
    oss << i;
    return oss.str();
}

inline std::string intToString(int i) {
    return toString(i);
}

inline bool isFloat(const std::string& s) {
    if (s.length() == 0) return false;

    bool seen_decimal_point = false;
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.at(i);
        if (c == '.') {
            if (seen_decimal_point) return false;
            else seen_decimal_point = true;
        }
        else if (s.at(i) < '0' || s.at(i) > '9') return false;
    }
    return true;
}


struct GTPResponse {
private:
    GTPResponseType resptype;
    std::string msg;
    std::string id;
    
public:

    GTPResponse() :
        resptype(GTP_INVALID_FORMAT)
    {}
    
    GTPResponse(GTPResponseType _resptype, std::string _msg) :
        resptype(_resptype),
        msg(_msg)
    {}

    static GTPResponse fromString(const std::string& s) {
        GTPResponse ret;

        if (s.length() == 0) return ret; // invalid format

        char first = s[0];
        switch (first) {
            case '=':
                ret.resptype = GTP_SUCCESS;
                break;

            case '?':
                ret.resptype = GTP_FAILURE;
                break;

            default:
                ret.resptype = GTP_INVALID_FORMAT;
                ret.msg = s;
                return ret; // invalid format
        }

        unsigned int i = 1;
        for (; i < s.length() && s[i] == ' '; i++) {
            ret.id.push_back(s[i]);
        }

        ret.msg = s.substr(i);
        return ret;
    }

    void setId(std::string _id) {
        id = _id;
    }

    std::string getId() const {
        return id;
    }

    std::string getMsg() const {
        return msg;
    }
    
    std::string toString() const {
        switch (resptype) {
            case GTP_SUCCESS:
                return "=" + id + " " + msg + "\n\n";
            case GTP_FAILURE:
                return "?" + id + " " + msg + "\n\n";
            case GTP_QUIT:
                return "\n\n";
            default:
                std::cout << "GTPResponse.toString() failed because resptype was " << resptype << "\n";
                std::cout << "msg = " << msg << "\n";

                assert(false);
                abort();
        }
    }
    
    bool isQuit() {
        return resptype == GTP_QUIT;
    }
    
    bool isSuccess() {
        return resptype == GTP_SUCCESS;
    }
    
    bool isFailure() {
        return resptype == GTP_FAILURE;
    }

    bool isInvalidFormat() {
        return resptype == GTP_INVALID_FORMAT;
    }
};

/* used to tell GTPParser what code to run for what GTP command */
struct GTPCallback {
    public:
        virtual GTPResponse callback(const std::vector<std::string>& args) = 0;

        virtual ~GTPCallback() {}
};

/* used for protocol_version, name, version 
    responds to a command with no arguments with a fixed response string */
class GTPCallbackStatic : public GTPCallback {
    private:
        const std::string response;
        
    public:
        GTPCallbackStatic(const std::string& _response) : response(_response) {}
    
        virtual GTPResponse callback(const std::vector<std::string>& args) {
            if (args.size() != 0) {
                return GTPResponse(GTP_FAILURE, "unknown command # takes no arguments");
            } else {
                return GTPResponse(GTP_SUCCESS, response);
            }
        }
};

/* used for known_command */
class GTPCallbackKnownCommand : public GTPCallback {
    private:
        const std::map<std::string, GTPCallback*>& callback_map;
        
    public:
        GTPCallbackKnownCommand(const std::map<std::string, GTPCallback*>& _callback_map) : callback_map(_callback_map) {}
    
        virtual GTPResponse callback(const std::vector<std::string>& args) {
            if (args.size() != 1) {
                return GTPResponse(GTP_FAILURE, "unknown command # takes 1 argument");
            } else {
                const std::string& command = args[0];
                
                if (callback_map.find(command) != callback_map.end()) {
                    return GTPResponse(GTP_SUCCESS, "true");
                } else {
                    return GTPResponse(GTP_SUCCESS, "false");
                }
            }
        }
};


/* used for list_commands */
class GTPCallbackListCommands : public GTPCallback {
    private:
        typedef const std::map<std::string, GTPCallback*> CMType;
        CMType& callback_map;
        
    public:
        GTPCallbackListCommands(CMType& _callback_map) : callback_map(_callback_map) {}
    
        virtual GTPResponse callback(const std::vector<std::string>& args) {
            if (args.size() != 0) {
                return GTPResponse(GTP_FAILURE, "unknown command # takes no arguments");
            } else {
                std::string response;
                
                bool first = true;
                for (CMType::const_iterator it = callback_map.begin(); it != callback_map.end(); ++it) {

                    if (first) {
                        first = false;
                    } else {
                        response.append("\n");
                    }
                    
                    const std::string& cmd = it->first; 
                    
                    response.append(cmd);
                }
                
                return GTPResponse(GTP_SUCCESS, response);
            }
        }
};

#endif
