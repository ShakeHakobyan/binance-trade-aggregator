#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <string>
#include <vector>

#include "trade_queue.h"

using tcp = boost::asio::ip::tcp;
using WebSocket = boost::beast::websocket::stream<boost::beast::ssl_stream<tcp::socket>>;

class BinanceClient {
  public:
    BinanceClient(std::vector<std::string> pairs, TradeQueue &queue);

    void run();

  private:
    void connect(WebSocket &ws, tcp::resolver &resolver);
    void handshake(WebSocket &ws);
    void setupControl(WebSocket &ws);
    void subscribe(WebSocket &ws);
    void readLoop(WebSocket &ws);
    void connectAndListen();

    std::vector<std::string> pairs_;
    TradeQueue &queue_;
};
