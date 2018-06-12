#include "WebsocketClient.h"

//Use http://think-async.com/ insted of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class SportsBoothWebsocketClientImpl : public WebsocketClient
{
public:
    SportsBoothWebsocketClientImpl();
    ~SportsBoothWebsocketClientImpl();
    virtual bool connect(std::string url, long long room, std::string username, std::string token, WebsocketClient::Listener* listener);
    virtual bool open(const std::string &sdp, const std::string& codec = "");
    virtual bool trickle(const std::string &mid, int index, const std::string &candidate, bool last);
    virtual bool subscribeToPath(const std::string &path, const std::string &sub_id);
    virtual bool sendMessage(const std::string &path, const std::string &content);
    virtual bool disconnect(bool wait);
    void sendThread();

private:
    std::string login;
    std::string session_id;

    std::atomic<bool> is_running;
    std::future<void> handle;
    std::thread thread;
    std::thread thread_send;
    std::queue<std::string> msgs_to_send;

    Client client;
    Client::connection_ptr connection;
};

