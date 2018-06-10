#include "SportsBoothWebsocketClientImpl.h"
#include "json.hpp"
#include <boost/algorithm/string.hpp>

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
  
  //reset loggin flag
  logged = false;
  try
  {
    // Register our message handler
    client.set_message_handler( [=](websocketpp::connection_hdl con, message_ptr frame) {
      const char* x = frame->get_payload().c_str();
      //get response
//      auto msg = json::parse(frame->get_payload());

        std::vector<std::string> strs;
        boost::split(strs,x,boost::is_any_of("\n"));

        std::cout << "* size of the vector: " << strs.size() << std::endl;
        for (size_t i = 0; i < strs.size(); i++)
            std::cout << strs[i] << std::endl;


        if (strs.front() == "CONNECTED") {
            subscribeToPath("/topic/chat.stream.ruddell", "chat");
            // todo figure out how to wait here
            std::this_thread::sleep_for(std::chrono::seconds(5));
//            sleep(5);
        }

        if (strs.front() == "MESSAGE") {
            auto msg = json::parse(strs[8]);
            std::string method = "chat";
            if (msg.find("method") != msg.end()) {
                method = msg["method"];
            }
            std::cout << method << std::endl;
            if (method == "chat") {
                std::cout << "Got SessionID from Worsh!" << std::endl;
                session_id = msg["sessionId"];
                subscribeToPath("/topic/tree.status-user" + session_id, "tree");
                sleep(2);
                listener->onLogged(1);
            } else if (method == "obsAnswer") {
                std::cout << "Got SDP Answer from Worsh! - Need to parse and send to plugin" << std::endl;
                std::string parsedData = msg["data"];
                std::cout << parsedData << std::endl;
                auto parsedDataJson = json::parse(parsedData);
//                std::string sdpAnswer = parsedData["sdpAnswer"];
                listener->onOpened(parsedDataJson["sdpAnswer"]);
            } else if (method == "iceCandidate") {
                std::cout << "Got ICE Candidate from Worsh! - Need to parse and send to plugin" << std::endl;
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
        if (strs.size() == 2) {
            std::cout << "Hearbeat" << std::endl;
            connection->send("\n", 1);
            if (session_id.empty()) {
                std::cout << "Sending chat to get session id" << std::endl;
                sendMessage("/topic/chat.stream.send.ruddell", "{\"message\":\"from obs!\",\"userImg\":\"https://cdn.sportsbooth.tv/profile/default_thumb.jpg\"}");
            }
        }
//
//      //Check if it is an event
//      if (msg.find("janus") == msg.end())
//        //Ignore
//        return;
//
//      std::string id = msg["janus"];
//
//      if (msg.find("ack") != msg.end()){
//        // Ignore
//        return;
//      }
//
//      //Get response
//      std::string response = msg["janus"];
//      //Check type
//      if (id.compare("success") == 0)
//      {
//        //Get transaction id
//        if (msg.find("transaction") == msg.end())
//          //Ignore
//          return;
//
//        if (msg.find("data") == msg.end()){
//          //Ignore
//          return;
//        }
//        //Get the Data session
//        auto data = msg["data"];
//
//        //Server is sending response twice, ingore second one
//        if (!logged)
//        {
//          //Get response code
//          session_id = data["id"];
//          //Launch logged event
//          //create handle command
//          json attachPlugin = {
//            {"janus", "attach" },
//            {"transaction", std::to_string(rand()) },
//            {"session_id", session_id},
//            {"plugin", "janus.plugin.videoroom"},
//          };
//
////          connection->send(attachPlugin.dump());
//          //Logged
//          logged = true;
//
//          //Keep the connection alive
//          is_running.store(true);
//          thread_keepAlive = std::thread([&]() {
//            SportsBoothWebsocketClientImpl::keepConnectionAlive();
//          });
//        }else {
//          handle_id = data["id"];
//
//          json joinRoom = {
//            {"janus", "message" },
//            {"transaction", std::to_string(rand())},
//            {"session_id", session_id},
//            {"handle_id", handle_id},
//            { "body" ,
//              {
//                { "room" , room },
//                {"display" , "OBS"},
//                {"ptype"  , "publisher"},
//                {"request" , "join"}
//              }
//            }
//          };
////          connection->send(joinRoom.dump());
//          listener->onLogged(session_id);
//        }
//      }
    });


    //When we are open
    client.set_open_handler([=](websocketpp::connection_hdl con){
      //Launch event
      listener->onConnected();
        // TODO: THE NULL TERMINATOR IS STRIPPED, ERROR RETURNED WITH []
      connection->send("CONNECT\naccept-version:1.1,1.0\nheart-beat:10000,10000\n\n\u0000", 56);

        //Login command
//      json login = {
//        {"janus", "create"},
//        {"transaction",std::to_string(rand()) },
//        { "payload",
//          {
//            { "username", username},
//            { "token", token},
//            { "room", room}
//          }
//        }
//      };
//      //Serialize and send
//      connection->send(login.dump());

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
    std::string wss = url + "?access_token=" + token;
      std::cout << "Connecting to " + wss << std::endl;

      connection = client.get_connection(wss, ec);

    if (ec) {
      std::cout << "could not create connection because: " << ec.message() << std::endl;
      return 0;
    }
//    connection->add_subprotocol("janus-protocol");

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
              { "user" ,       "ruddell" },
              { "sdpOffer",     sdp },
              { "mediaType",    "video" },
              { "privacy",      "public" },
              { "splitscreen",  false },
              { "obsInput",     true },
      };
    //Login command
    json open = {
      { "method", "presenter" },
      { "treeId", "ruddell" },
      { "data",   data.dump() }
    };

      if (sendMessage("/topic/tree.status.update", open.dump())) {
          return true;
      }
  }
  catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
    return false;
  }
  //OK
  return true;
}
bool SportsBoothWebsocketClientImpl::trickle(const std::string &mid, int index, const std::string &candidate, bool last)
{
  try
  {
      // TODO: wait for ice connection state?
//      std::this_thread::sleep_for(std::chrono::seconds(5));
      //Check if it is last
    if (!last)
    {
        json data = {
                { "user" ,       "ruddell" },
                { "treeId", "ruddell" },
                { "candidate", candidate },
                { "sdpMLineIndex", index },
                { "sdpMid", mid },
        };
        json trickle = {
                { "method", "onIceCandidate" },
                { "treeId", "ruddell" },
                { "data",   data.dump() }
        };

        if (sendMessage("/topic/tree.status.update", trickle.dump()))
            return false;

          //OK
        return true;
    }
    else
    {
        json data = {
                { "user" ,       "ruddell" },
                { "treeId", "ruddell" },
                { "candidate", { "completed", true } }
        };
        json trickle = {
                { "method", "onIceCandidate" },
                { "treeId", "ruddell" },
                { "data",   data.dump() }
        };

      if (connection->send(trickle.dump()))
        return false;

    }
  }
  catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
    return false;
  }
  //OK
  return true;
}

void SportsBoothWebsocketClientImpl::keepConnectionAlive()
{
  while (is_running.load())
  {
    if (connection)
    {
      json keepaliveMsg = {
        { "janus"    , "keepalive" },
        { "session_id" , session_id },
        { "transaction" , "keepalive-" + std::to_string(rand()) },
      };
      try
      {
//        connection->send(keepaliveMsg.dump());
      }
      catch (websocketpp::exception const & e)
      {
        std::cout << e.what() << std::endl;
//        return;
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
};

bool SportsBoothWebsocketClientImpl::disconnect(bool wait)
{

    try
    {
        // Stop keepAlive
        if (thread_keepAlive.joinable()){
            is_running.store(false);
            thread_keepAlive.join();
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
                //Remov hanlders
                client.set_open_handler([](...){});
                client.set_close_handler([](...){});
                client.set_fail_handler([](...) {});
                //Detach trhead
                thread.detach();

            }
        }

    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
        return false;
    }
//OK
    return true;
}

bool SportsBoothWebsocketClientImpl::subscribeToPath(const std::string &path, const std::string &sub_id)
{
    std::cout << "Subscribing to: " + path << std::endl;
    try
    {
        std::string subscribeMessage = "SUBSCRIBE\nid:sub-" + sub_id + "\ndestination:" + path + "\n\n\u0000";
        if (connection->send(subscribeMessage.c_str(), subscribeMessage.length() + 1))
            return false;
        //OK
        return true;
    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
        return false;
    }
}
bool SportsBoothWebsocketClientImpl::sendMessage(const std::string &path, const std::string &content)
{
    std::cout << "Sending message to: " + path << std::endl;
    try
    {
        std::string message = "SEND\ndestination:" + path + "\ncontent-length:" + std::to_string(content.length()) + "\n\n" + content + "\u0000";
        if (connection->send(message.c_str(), message.length() + 1))
            return false;
        //OK
        return true;
    }
    catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
        return false;
    }
}