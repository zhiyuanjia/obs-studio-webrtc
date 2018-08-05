#include "SportsBoothWebsocketClientImpl.h"
#include "json.hpp"

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

SportsBoothWebsocketClientImpl::SportsBoothWebsocketClientImpl()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    client.init_asio();
}

SportsBoothWebsocketClientImpl::~SportsBoothWebsocketClientImpl()
{
    //Disconnect just in case
    disconnect(false);
}

bool SportsBoothWebsocketClientImpl::connect(std::string url, long long room, std::string username, std::string token, WebsocketClient::Listener* listener)
{
    websocketpp::lib::error_code ec;

    try
    {
        // Register our message handler
        client.set_message_handler( [=](websocketpp::connection_hdl con, message_ptr frame) {
            const char* x = frame->get_payload().c_str();
            //get response

            std::vector<std::string> messageLines;
            std::string token;
            std::istringstream tokenStream(x);
            while (std::getline(tokenStream, token))
            {
                messageLines.push_back(token);
            }

            if (messageLines.front() == "CONNECTED") {
                // once connected, start the send thread and get websocket session info from the server
                is_running.store(true);
                thread_send = std::thread([&]() {
                    SportsBoothWebsocketClientImpl::sendThread();
                });
                std::string randomNum = std::to_string(rand() % 100000);
                std::string destination = "/topic/session.info." + randomNum;
                subscribeToPath(destination, "session-info" + randomNum);
            }

            if (messageLines.front() == "MESSAGE") {
                auto msg = json::parse(messageLines[8]);
                std::string method = "session-info";
                if (msg.find("method") != msg.end()) {
                    method = msg["method"];
                }
                std::cout << method << std::endl;
                if (method == "sessionInfo" && session_id.length() <= 0) {
                    std::cout << "Got SessionID from SportsBooth!" << std::endl;
                    std::string parsedData = msg["data"];
                    std::cout << parsedData << std::endl;
                    auto parsedDataJson = json::parse(parsedData);
                    session_id = parsedDataJson["sessionId"];
                    login = parsedDataJson["login"];
                    std::string randomNum = std::to_string(rand() % 100000);
                    subscribeToPath("/topic/tree.status-user" + session_id, "tree" + randomNum);
                    listener->onLogged(1);
                } else if (method == "obsAnswer") {
                    std::cout << "Got SDP Answer from SportsBooth!" << std::endl;
                    std::string parsedData = msg["data"];
                    std::cout << parsedData << std::endl;
                    auto parsedDataJson = json::parse(parsedData);
                    std::string sdpAnswer = parsedDataJson["sdpAnswer"];
                    listener->onOpened(sdpAnswer);
                } else if (method == "presenterResponse") {
                    std::cout << "Got Presenter Response from SportsBooth!" << std::endl;
                    std::string parsedData = msg["data"];
                    std::cout << parsedData << std::endl;
                    auto parsedDataJson = json::parse(parsedData);
                    std::string errorMessage = parsedDataJson["message"];
                    this -> disconnect(false);
                    listener->onDisconnected();
                } else if (method == "iceCandidate") {
                    std::cout << "Got ICE Candidate from SportsBooth!" << std::endl;
                    std::string parsedData = msg["data"];
                    auto parsedDataJson = json::parse(parsedData);
                    std::string candidate = parsedDataJson["candidate"];
                    int sdpMLineIndex = parsedDataJson["sdpMLineIndex"];
                    std::string sdpMid = parsedDataJson["sdpMid"];
                    listener->onTrickle(sdpMid, sdpMLineIndex, candidate);
                } else {
                    std::cout << "Unhandled method" << std::endl;
                }
            }
            if (messageLines.size() == 1) {
                std::cout << "Heartbeat" << std::endl;
                msgs_to_send.push("\n");
            }
        });


        //When we are open
        client.set_open_handler([=](websocketpp::connection_hdl con){
            //Launch event
            listener->onConnected();
            // pad this with a null byte
            std::string connectMessage = "CONNECT\nstream-key:" + token + "\naccept-version:1.1,1.0\nheart-beat:10000,10000\n\n";
            connection->send(connectMessage.c_str(), connectMessage.length() + 1);
        });
        //Set close hanlder
        client.set_close_handler([=](...) {
            //Call listener
            listener->onDisconnected();
        });
        //Set failure handler
        client.set_fail_handler([=](...) {
            //Call listener
            listener->onDisconnected();
        });
        //Register our tls hanlder
        client.set_tls_init_handler([&](websocketpp::connection_hdl connection) {
            //Create context
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);
            try {
                ctx->set_options(asio::ssl::context::default_workarounds |
                                 asio::ssl::context::no_sslv2 |
                                 asio::ssl::context::single_dh_use);
            }
            catch (std::exception& e) {
                std::cout << e.what() << std::endl;
            }
            return ctx;
        });
        //Get connection
        //Create websocket connection and token
        std::string wss = url;
        std::cout << "Connecting to " + wss << std::endl;

        connection = client.get_connection(wss, ec);

        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        client.connect(connection);

        //Async
        thread = std::thread([&]() {
            // Start the ASIO io_service run loop
            // this will cause a single connection to be made to the server. c.run()
            // will exit when this connection is closed.

            client.run();
        });
    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
        return false;
    }
    //OK
    return true;
}

bool SportsBoothWebsocketClientImpl::open(const std::string &sdp, const std::string& codec)
{
    // send the offer in the Presenter Method
    try
    {
        json data = {
                { "user" ,       login },
                { "sdpOffer",     sdp },
                { "mediaType",    "video" },
                { "privacy",      "public" },
                { "splitscreen",  false },
                { "obsInput",     true },
        };
        //Login command
        json open = {
                { "method", "presenter" },
                { "treeId", login },
                { "data",   data.dump() }
        };

        sendMessage("/topic/tree.status.update", open.dump());
    }
    catch (websocketpp::exception const & e) {
        std::cout << "Websocket Exception Caught!" << std::endl;
        std::cout << e.what() << std::endl;
        return false;
    }
    // OK
    return true;
}
bool SportsBoothWebsocketClientImpl::trickle(const std::string &mid, int index, const std::string &candidate, bool last)
{
    try
    {
        if (!last)
        {
            json data = {
                    { "user" ,       login },
                    { "treeId", login },
                    { "candidate", candidate },
                    { "sdpMLineIndex", index },
                    { "sdpMid", mid },
            };
            json trickle = {
                    { "method", "onIceCandidate" },
                    { "treeId", login },
                    { "data",   data.dump() }
            };

            sendMessage("/topic/tree.status.update", trickle.dump());
        }
        else
        {
            json data = {
                    { "user" ,       login },
                    { "treeId", login },
                    { "candidate", { "completed", true } }
            };
            json trickle = {
                    { "method", "onIceCandidate" },
                    { "treeId", login },
                    { "data",   data.dump() }
            };

            sendMessage("/topic/tree.status.update", trickle.dump());
        }
    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
        return false;
    }
    //OK
    return true;
}

bool SportsBoothWebsocketClientImpl::disconnect(bool wait)
{
    std::cout << "Disconnecting!" << std::endl;
    try
    {
        // Stop send thread
        if (thread_send.joinable()){
            json data = {
                    { "user", login }
            };
            json stopMsg = {
                    { "method"    , "stop" },
                    { "treeId" , login },
                    { "data", data.dump() }
            };
            sendMessage("/topic/tree.status.update", stopMsg.dump());
            while (!msgs_to_send.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::cout << "Stop Message Sent" << std::endl;

            is_running.store(false);
            thread_send.join();
        }

        //Stop client
        client.close(connection, websocketpp::close::status::normal, std::string("disconnect"));
        client.stop();

        //Don't wait for connection close
        if (thread.joinable())
        {
            //If sswe have to wait
            if (wait) {
                thread.join();
            }
            else {
                //Remove handlers
                client.set_open_handler([](...){});
                client.set_close_handler([](...){});
                client.set_fail_handler([](...) {});
                //Detach thread
                thread.detach();

            }
        }
    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
        return false;
    }
    // OK
    return true;
}

bool SportsBoothWebsocketClientImpl::subscribeToPath(const std::string &path, const std::string &sub_id)
{
    std::cout << "Subscribing to: " + path << std::endl;
    std::string subscribeMessage = "SUBSCRIBE\nid:sub-" + sub_id + "\ndestination:" + path + "\n\n";
    msgs_to_send.push(subscribeMessage);
    return true;
}
bool SportsBoothWebsocketClientImpl::sendMessage(const std::string &path, const std::string &content)
{
    std::cout << "Sending message to: " + path << std::endl;
    std::string message = "SEND\ndestination:" + path + "\ncontent-length:" + std::to_string(content.length()) + "\n\n" + content;
    msgs_to_send.push(message);
    return true;
}

void SportsBoothWebsocketClientImpl::sendThread()
{
    while (is_running.load())
    {
        if (connection && !msgs_to_send.empty()) {
            std::string message = msgs_to_send.front();
            msgs_to_send.pop();
            if (message == "\n") {
                std::cout << "Hearbeat sent" << std::endl;
                connection->send("\n", 1);
            } else {
                std::cout << "Sending message:\n" + message << std::endl;
                // pad the message with a null byte if it's not a hearbeat
                connection->send(message.c_str(), message.length() + 1);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};