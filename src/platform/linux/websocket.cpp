#include "platform/linux/websocket.hpp"

ws::context_ptr ws::on_tls_init(ws::handle hdl) {
  (void)hdl;
  ws::context_ptr ctx =
      websocketpp::lib::make_shared<boost::asio::ssl::context>(
          boost::asio::ssl::context::sslv23);

  try {
    ctx->set_options(boost::asio::ssl::context::default_workarounds |
                     boost::asio::ssl::context::no_sslv2 |
                     boost::asio::ssl::context::no_sslv3 |
                     boost::asio::ssl::context::single_dh_use);
  } catch (std::exception &e) {
    std::cerr << "Error in TLS context: " << e.what() << std::endl;
  }
  return ctx;
}
