#include "binance_client.h"
#include "json_trade_parser.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <nlohmann/json.hpp>

#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

using json = nlohmann::json;

namespace {
const std::string kHost = "stream.binance.com";
const std::string kPort = "9443";
const std::string kTarget = "/stream";
constexpr int kReadTimeoutSeconds = 2; // how often readLoop rechecks stopping_
} // namespace

BinanceClient::BinanceClient(std::vector<std::string> pairs, TradeQueue& queue)
    : pairs_(std::move(pairs)), queue_(queue) {
}

void BinanceClient::stop() {
    stopping_ = true;
    // No socket manipulation needed here — readLoop() polls stopping_ on
    // every read timeout (see setReadTimeout / kReadTimeoutSeconds), so it
    // will notice and exit within kReadTimeoutSeconds at most.
}

void BinanceClient::run() {
    int backoffSeconds = 1;
    const int maxBackoffSeconds = 30;

    while (!stopping_) {
        try {
            connectAndListen();
            backoffSeconds = 1;
        } catch (const std::exception& e) {
            if (stopping_) break;

            std::cerr << "[BinanceClient] " << e.what() << ". Reconnecting in " << backoffSeconds
                      << "s\n";
            std::this_thread::sleep_for(std::chrono::seconds(backoffSeconds));
            backoffSeconds = std::min(backoffSeconds * 2, maxBackoffSeconds);
        }
    }
    std::cout << "[BinanceClient] Stopped.\n";
}

void BinanceClient::connect(WebSocket& ws, tcp::resolver& resolver) {
    auto results = resolver.resolve(kHost, kPort);
    net::connect(beast::get_lowest_layer(ws), results);
}

void BinanceClient::handshake(WebSocket& ws) {
    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), kHost.c_str())) {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()));
    }

    ws.next_layer().handshake(ssl::stream_base::client);
    ws.handshake(kHost, kTarget);

    std::cout << "[BinanceClient] Connected\n";
}

void BinanceClient::setupControl(WebSocket& ws) {
    ws.control_callback([](websocket::frame_type type, beast::string_view) {
        if (type == websocket::frame_type::ping) {
            std::cout << "[BinanceClient] ping\n";
        }
    });
}

void BinanceClient::subscribe(WebSocket& ws) {
    json msg = {{"method", "SUBSCRIBE"}, {"params", pairs_}, {"id", 1}};
    std::string request = msg.dump();
    ws.write(net::buffer(request));

    std::cout << "[BinanceClient] subscribed\n";
}

void BinanceClient::setReadTimeout(WebSocket& ws, int timeoutSeconds) {
    struct timeval tv;
    tv.tv_sec = timeoutSeconds;
    tv.tv_usec = 0;

    int fd = beast::get_lowest_layer(ws).native_handle();
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void BinanceClient::readLoop(WebSocket& ws) {
    while (!stopping_) {
        beast::flat_buffer buffer;
        beast::error_code ec;
        ws.read(buffer, ec);

        if (ec) {
            // A read timeout is expected (we set it deliberately) — just
            // loop again, which re-checks stopping_.
            if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
                continue;
            }
            // Any other error is a real connection problem — propagate so
            // run() treats it as a disconnect and reconnects with backoff.
            throw beast::system_error(ec);
        }

        std::string message = beast::buffers_to_string(buffer.data());

        Trade trade;
        if (JsonTradeParser::parse(message, trade)) {
            queue_.push(trade);
        }
    }
}

void BinanceClient::connectAndListen() {
    net::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.set_default_verify_paths();
    tcp::resolver resolver(ioc);
    WebSocket ws(ioc, ctx);

    connect(ws, resolver);
    handshake(ws);
    setupControl(ws);
    subscribe(ws);
    setReadTimeout(ws, kReadTimeoutSeconds);
    readLoop(ws);
}