#ifndef __CALLBACKS_HPP
#define __CALLBACKS_HPP

#include "../go_gtp_utils.hpp"
#include "../generic/gtp_parser_callbacks.hpp"
#include "../../go_mechanics/go_state.hpp"

class GoGTPInterface;

class GTPCallbackBoardsize : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackBoardsize(GoGTPInterface* _parent) : parent(_parent) {}
        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackClearBoard : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackClearBoard(GoGTPInterface* _parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackShowboard : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackShowboard(GoGTPInterface* _parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackKomi : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackKomi(GoGTPInterface* _parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackPlay : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackPlay(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackGenmove : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackGenmove(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackFinalScore : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackFinalScore(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackTimeLeft : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackTimeLeft(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackTimeSettings : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackTimeSettings(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackQuit : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackQuit(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackCputime : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackCputime(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

class GTPCallbackLoadSGF : public GTPCallback {
    private:
        GoGTPInterface *parent;

    public:
        GTPCallbackLoadSGF(GoGTPInterface *_parent) : parent(_parent) {}

        virtual GTPResponse callback(const std::vector<std::string>& args);
};

#endif
