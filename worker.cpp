#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct Args {
  int port;
  int hearbeat_interval;
};

Args parse_args(int argc, char const *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <port> <heartbeat_interval>"
              << std::endl;
    exit(1);
  }
  Args args;
  args.port = std::atoi(argv[1]);
  args.hearbeat_interval = std::atoi(argv[2]);
  return args;
}

int main(int argc, char const *argv[]) {
  std::cout << "Worker started" << std::endl;

  Args args = parse_args(argc, argv);

  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("Setsockopt failed");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(args.port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t *)&addrlen)) < 0) {
    perror("Accept failed");
    exit(EXIT_FAILURE);
  }

  while (true) {
    int bytes_read = read(new_socket, buffer, 1024);
    if (bytes_read < 0) {
      perror("Read failed");
      exit(EXIT_FAILURE);
    }
    std::cout << "Received: " << buffer << std::endl;

    const char *response = "Heartbeat from worker";
    int response_len = strlen(response);
    int bytes_sent = send(new_socket, response, response_len, 0);
    if (bytes_sent != response_len) {
      perror("Send failed");
      exit(EXIT_FAILURE);
    }

    std::this_thread::sleep_for(std::chrono::seconds(args.hearbeat_interval));
  }

  close(new_socket);
  close(server_fd);

  return 0;
}
