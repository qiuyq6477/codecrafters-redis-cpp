#include <iostream>
#include <thread>
#include <vector>
#include <netinet/in.h>

#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

const int BUFFER_SIZE = 1024;

int get_length(char *p)
{
  int len = 0;
  while(*p != '\r') {
      len = (len*10)+(*p - '0');
      p++;
  }
  p++;
  return len;
}


void parse(int client_socket, char *p)
{
  // *2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n
  // *1\r\n$4\r\nPING\r\n
  p++;
  int num = get_length(p);
  std::cout << "num:" << num << std::endl;
  for(int i =0; i < num; i++)
  {
    p++;
    int len = get_length(p);
    if(memcmp(p, "ECHO", len) == 0)
    {
      p += len + 2;
      p++;
      len = get_length(p);
      std::cout << "len:" << len << std::endl;
      char buffer[len + 3] = {'+'};
      memcpy(buffer+1, p, len + 2);
      send(client_socket, buffer, len + 3, 0);
    }
    else if(memcmp(p, "PING", 4) == 0)
    {
      send(client_socket, "+PONG\r\n", 7, 0);
    }
  }
}

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE];
  int bytes_received;
  
  while(true){
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if(bytes_received <= 0){
      break;
    }
    parse(client_socket, buffer);
  }
  close(client_socket);
  std::cout << "Client disconnected." << std::endl;
}

int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  std::vector<std::thread> threads;
  while (true) {
      int client_socket;
      struct sockaddr_in client_addr;
      socklen_t client_addr_len = sizeof(client_addr);

      // Accept a new connection
      if ((client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
          perror("Accept failed");
          continue;
      }

      std::cout << "New client connected." << std::endl;

      // Create a new thread to handle the client
      threads.emplace_back(std::thread(handle_client, client_socket));
  }
  
  // Join threads
  for (auto &thread : threads) {
      if (thread.joinable()) {
          thread.join();
      }
  }

  close(server_fd);

  return 0;
}
