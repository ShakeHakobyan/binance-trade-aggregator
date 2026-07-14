#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "trade_queue.h"

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using WebSocket = boost::beast::websocket::stream<boost::beast::ssl_stream<tcp::socket>>;

class BinanceClient {
  public:
    BinanceClient(std::vector<std::string> pairs, TradeQueue& queue);

    void run();
    void stop();

    static std::string buildSubscribeMessage(const std::vector<std::string>& pairs, int id);
    static int nextBackoffSeconds(int currentBackoff, int maxBackoff);

  private:
    void connect(WebSocket& ws, tcp::resolver& resolver);
    void handshake(WebSocket& ws);
    void setupControl(WebSocket& ws);
    void subscribe(WebSocket& ws);
    void setReadTimeout(WebSocket& ws, int timeoutSeconds);
    void readLoop(WebSocket& ws);
    void connectAndListen();

    std::vector<std::string> pairs_;
    TradeQueue& queue_;

    net::io_context ioc_;
    boost::asio::ssl::context sslCtx_{boost::asio::ssl::context::tlsv12_client};
    std::unique_ptr<WebSocket> ws_;

    std::atomic<bool> stopping_{false};
};
