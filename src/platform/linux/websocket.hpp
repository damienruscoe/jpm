#pragma once

#include <string>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <boost/asio/ssl.hpp>

namespace ws {

using context_ptr = websocketpp::lib::shared_ptr<boost::asio::ssl::context>;
using message_ptr = websocketpp::config::asio_client::message_type::ptr;
using client = websocketpp::client<websocketpp::config::asio_tls_client>;
using handle = websocketpp::connection_hdl;

context_ptr on_tls_init(handle hdl);

void process_websocket_message(const auto &on_parsed,
                               [[maybe_unused]] ws::handle hdl,
                               ws::message_ptr msg) {
  on_parsed(msg->get_payload());
}

void connect(const std::string &uri, const auto &on_parsed) {
  try {
    client c;

    // Set logging to be pretty verbose (everything except message payloads)
    c.set_access_channels(websocketpp::log::alevel::all);
    c.clear_access_channels(websocketpp::log::alevel::frame_payload);
    c.set_tls_init_handler(websocketpp::lib::bind(
        &ws::on_tls_init, websocketpp::lib::placeholders::_1));

    // Initialize ASIO
    c.init_asio();

    // Register our message handler
    c.set_message_handler([&](auto &&...args) {
      return process_websocket_message(on_parsed,
                                       std::forward<decltype(args)>(args)...);
    });

    websocketpp::lib::error_code ec;
    client::connection_ptr con = c.get_connection(uri, ec);
    if (ec) {
      std::cerr << "could not create connection because: " << ec.message()
                << std::endl;
      return;
    }

    c.connect(con);
    c.run();
  } catch (websocketpp::exception const &e) {
    std::cerr << e.what() << std::endl;
  }
}

} // namespace ws
