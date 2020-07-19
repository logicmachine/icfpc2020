#include <iostream>
#include <regex>
#include <string>
#include <cassert>
#include <string>
#include <cinttypes>
#include "httplib.h"

class RequestHandler {
private:
    httplib::Client* client;
    std::string alienSendUrl;
    
public:
    int8_t returnedStatus;
    RequestHandler() = default;
    RequestHandler(const std::string &serverUrl,
                   const std::string &playerKey)
        : alienSendUrl(serverUrl + "/aliens/send") {
        std::cout << "ServerUrl: " << serverUrl << "; PlayerKey: " << playerKey << std::endl;
	
        const std::regex urlRegexp("http://(.+):(\\d+)");
        std::smatch urlMatches;
        if (!std::regex_search(serverUrl, urlMatches, urlRegexp) || urlMatches.size() != 3) {
            std::cout << "Unexpected server response:\nBad server URL" << std::endl;
            returnedStatus = 1;
            return;
        }
        const std::string serverName = urlMatches[1];
        const int serverPort = std::stoi(urlMatches[2]);
        client = new httplib::Client(serverName, serverPort);
        const std::shared_ptr<httplib::Response> serverResponse = 
            client->Post(serverUrl.c_str(), playerKey.c_str(), "text/plain");

        if (!serverResponse) {
            std::cout << "Unexpected server response:\nNo response from server" << std::endl;
            returnedStatus = 1;
            return;
        }
	
        if (serverResponse->status != 200) {
            std::cout << "Unexpected server response:\nHTTP code: " << serverResponse->status
                      << "\nResponse body: " << serverResponse->body << std::endl;
            returnedStatus = 2;
            return;
        }

        std::cout << "Server response: " << serverResponse->body << std::endl;
        returnedStatus = 0;
    }
    
    std::string modulate(const std::string& command) const {
        if(command == "cons") return "11";
        if(command == "nil") return "00";
        if(command == "ap") assert(false);

        const long long x = stoll(command);
        if(x == 0) return "010";
    
        const long long y = std::abs(x);
        std::string result = (x >= 0 ? "01" : "10");
        const int bits = (sizeof(long long) * 8 - __builtin_clzl(y) + 3) & ~3;
        for(int i = 0; i < bits; i += 4){ result += "1"; }
        result += "0";
        for(int i = bits - 1; i >= 0; --i) {
            result += (char)('0' + ((y >> i) & 1));
        }
        return result;
    }

    std::string modulate(const std::vector< std::string >& commands) const {
        std::string result = "";
        for(const auto& command : commands) {
            if(command == "ap") continue;
            result += modulate(command);
        }
        return result;
    }

    std::string makeCons(const std::string cmd_1, const std::string cmd_2) const {
        return "ap ap cons " + cmd_1 + " " + cmd_2;
    }

    // JOIN
    std::string makeJoinRequest(const std::string& playerKey) const {
        return makeCons("2", makeCons(playerKey, "nil"));
    }

    // SEND
    std::string send(const std::string& request) const {
        const auto response = client->Post(alienSendUrl.c_str(), request, "text/plain");
        std::cout << "send: response status " << response->status << std::endl;
        return response->body;
    }
};

class Solver : RequestHandler {
public:
    Solver(const std::string &serverUrl,
           const std::string &playerKey)
        : RequestHandler(serverUrl, playerKey) {
        std::cout << "returned status = " << returnedStatus << std::endl;

        // join
        std::string joinCmd = makeJoinRequest(playerKey);
        std::cout << "join request = " << joinCmd << std::endl;
        send(joinCmd);

        // start
    }
};

int main(int argc, char* argv[])
{
	const std::string serverUrl(argv[1]);
	const std::string playerKey(argv[2]);

    Solver solver(serverUrl, playerKey);
	return 0;
}

