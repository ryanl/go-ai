#ifndef __CONSOLE_ARGUMENTS_HPP
#define __CONSOLE_ARGUMENTS_HPP

#include <string>
#include <map>

#include <boost/optional.hpp>

/*!
@class ConsoleArguments

@brief Stores and provides easy queries of values provided
       as command-line arguments to the program.

@author Ryan Lothian
*/
class ConsoleArguments {
private:
    typedef std::map<std::string, std::string> map_t;
    map_t m;

public:
    ConsoleArguments() {}

    std::set<std::string> getKeys() const {
        std::set<std::string> ret;

        for (map_t::const_iterator it = m.begin(); it != m.end(); ++it) {
            ret.insert(it->first);
        }
        return ret;
    }

    bool has(std::string name) const {
        return m.find(name) != m.end();
    }

    boost::optional<std::string> get(const std::string& name) const {
        std::map<std::string, std::string>::const_iterator it = m.find(name);

        if (it == m.end()) {
            return boost::optional<std::string>();
        } else {
            return boost::optional<std::string>(it->second);
        }
    }

    std::string get(const std::string& name, const std::string& default_value) const {
        std::map<std::string, std::string>::const_iterator it = m.find(name);

        if (it == m.end()) {
            return default_value;
        } else {
            return it->second;
        }
    }

    void parse(unsigned int argc, char* argv[]) {
        std::string key, value;

        for (unsigned int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                if (key != "") {
                    add(key, value);
                }

                key = std::string(argv[i] + 1); // remove - from beginning
                value = "";
            } else {
                if (value != "") {
                    value += " ";
                }

                value += std::string(argv[i]);
            }
        }

        if (key != "") {
            add(key, value);
        }
    }

    void add(const std::string& name) {
        m[name] = "";
    }

    void add(const std::string& name, const std::string& value) {
        m[name] = value;
    }

    void add(const std::string& name, const boost::optional<std::string>& value) {
        if (value) {
            m[name] = *value;
        } else {
            m[name] = "";
        }
    }
};

#endif

