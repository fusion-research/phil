#pragma once

#include <cstdint>
#include <cstdio>
#include <sys/time.h>
#include <string>
#include <netinet/in.h>

namespace phil {

constexpr uint16_t kPort = 6789;

struct data_t {
  double left_encoder_rate;
  double right_encoder_rate;
  double rio_send_time_s;
  double tk1_recv_time_s;
};

inline void print_data_t(data_t d) {
  printf("rio:%f, main:%f\n", d.rio_send_time_s, d.tk1_recv_time_s);
};

inline double timeval_to_sec(struct timeval tv) {
  return tv.tv_sec + (float) tv.tv_usec / 1e6;
}

constexpr size_t data_t_size = sizeof(data_t);
extern socklen_t sockaddr_size;

class UDPServer {
 public:
  UDPServer();

  /**
   * Blocks until the next packet is received
   * @param response the functions fills this pointer with data
   * @param response_size the amount of data you expect to receive in bytes
   * @return the number of bytes actuall received and put in data
   */
  ssize_t Read(uint8_t *response, size_t response_size);

  /**
   * Sets the timeout for future calls to sendto and recvfrom
   * @param timeout timeout
   */
  void SetTimeout(timeval timeout);

 private:
  int socket_fd;
};

class UDPClient {
 public:
  explicit UDPClient(const std::string &server_hostname);

  /**
   * Sends data to TK1. This assumes data has been filled and stamped. This function may block for up to 1 second.
   * @return The data you send the TK1 but now with the TK1 time stamp in it
   */
  data_t Transaction(data_t data);

  /**
   * Blocks until the next packet is received
   * @param response the functions fills this pointer with data
   * @param response_size the amount of data you expect to receive in bytes
   * @return the number of bytes actuall received and put in data
   */
  ssize_t Read(uint8_t *response, size_t response_size);

  void RawTransaction(uint8_t *request, size_t request_size, uint8_t *response, size_t response_size);

  /**
   * Sets the timeout for future calls to sendto and recvfrom
   * @param timeout timeout
   */
  void SetTimeout(timeval timeout);

 private:
  int socket_fd;
  std::string server_hostname;
  struct sockaddr_in client_addr;
  struct sockaddr_in server_addr;

};

} // end namespace
