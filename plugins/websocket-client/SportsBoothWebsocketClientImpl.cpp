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
  
  //reset loggin flag
  logged = false;
  try
  {
    // Register our message handler
    client.set_message_handler( [=](websocketpp::connection_hdl con, message_ptr frame) {
      const char* x = frame->get_payload().c_str();
      //get response
//      auto msg = json::parse(frame->get_payload());

      std::cout << x << std::endl << std::endl << std::endl ;
      std::cout << "test" << std::endl << std::endl << std::endl ;

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
//      if (msg.find("jsep") != msg.end())
//      {
//        std::string sdp = msg["jsep"]["sdp"];
//        listener->onOpened(sdp);
//        return;
//      }
//
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
      connection->send("CONNECT\naccept-version:1.0,1.1\nlogin: ruddell\nheart-beat:30000,0\n\n\u0000", 67);

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
    connection = client.get_connection(url, ec);
    
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
    //Serialize and send
    if (connection->send(open.dump()))
      return false;
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

      //Serialize and send
      if (connection->send(trickle.dump()))
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
