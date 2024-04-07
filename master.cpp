#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

const int HEARTBEAT_INTERVAL = 1;                    // in seconds
const int TIMEOUT_INTERVAL = 3 * HEARTBEAT_INTERVAL; // in seconds

struct Worker {

  enum WorkerStatus { EMBRYO, CONNECTED, DISCONNECTED, DEAD };

  int id;
  std::string ip;
  int port;
  int sock;
  WorkerStatus connection_state;
  std::chrono::time_point<std::chrono::system_clock> last_heartbeat;

  friend std::ostream &operator<<(std::ostream &os, const Worker &worker) {
    os << "Worker [" << worker.id << "] " << worker.ip << ":" << worker.port;
    return os;
  }
};

void connectWorker(Worker &worker) {
  worker.sock = socket(AF_INET, SOCK_STREAM, 0);
  if (worker.sock < 0) {
    std::cerr << "Error creating socket for " << worker << std::endl;
    return;
  }

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(worker.port);
  inet_pton(AF_INET, worker.ip.c_str(), &serv_addr.sin_addr);

  if (connect(worker.sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
      0) {
    std::cerr << "Error connecting to " << worker << std::endl;
    return;
  }

  worker.connection_state = Worker::CONNECTED;
}

void pingWorker(Worker &worker) {
  const char *response = "PING";
  int response_len = strlen(response);
  int bytes_sent = send(worker.sock, response, response_len, 0);
  if (bytes_sent != response_len) {
    std::cerr << "Error sending data to " << worker.ip << std::endl;
    return;
  }

  char buffer[1024] = {0};
  int bytes_received = recv(worker.sock, buffer, 1024, 0);
  if (bytes_received < 0) {
    std::cerr << "Error receiving data from " << worker.ip << std::endl;
    return;
  }

  if (bytes_received == 0) {
    std::cout << worker << " is unresponsive" << std::endl;
    worker.connection_state = Worker::DISCONNECTED;
    close(worker.sock);
  } else {
    std::cout << "Received " << worker << ": " << buffer << std::endl;
    worker.last_heartbeat = std::chrono::system_clock::now();
  }
}

int main() {
  std::vector<Worker> workers = {
      Worker{1, "127.0.0.1", 8100, 0, Worker::EMBRYO,
             std::chrono::system_clock::now()},
      Worker{2, "127.0.0.1", 8101, 0, Worker::EMBRYO,
             std::chrono::system_clock::now()},
  };

  while (true) {
    for (auto &worker : workers) {
      switch (worker.connection_state) {
      case Worker::EMBRYO:
        connectWorker(worker);
        break;
      case Worker::CONNECTED:
        pingWorker(worker);
        break;
      case Worker::DISCONNECTED:
        connectWorker(worker);
        break;
      case Worker::DEAD:
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL));
  }

  // TODO: catch system interrupts and implement graceful shutdown
  for (auto &worker : workers) {
    if (worker.connection_state == Worker::CONNECTED) {
      close(worker.sock);
    }
  }

  return 0;
}
