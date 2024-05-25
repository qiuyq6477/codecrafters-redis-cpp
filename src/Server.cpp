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


#pragma region Dict

#define TABLE_SIZE 100

typedef struct KeyValue {
    char *key;
    char *value;
} KeyValue;

typedef struct HashTableEntry {
    KeyValue *pair;
    struct HashTableEntry *next;
} HashTableEntry;

typedef struct Dictionary {
    HashTableEntry **table;
} Dictionary;


unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % TABLE_SIZE;
}


void insert(Dictionary *dict, const char *key, const char *value) {
    unsigned long hash = hash_function(key);
    HashTableEntry *entry = dict->table[hash];
    
    // Update value if key already exists
    while (entry != NULL) {
        if (strcmp(entry->pair->key, key) == 0) {
            free(entry->pair->value);
            entry->pair->value = strdup(value);
            return;
        }
        entry = entry->next;
    }

    // Insert new key-value pair
    KeyValue *new_pair = (KeyValue *)malloc(sizeof(KeyValue));
    new_pair->key = strdup(key);
    new_pair->value = strdup(value);

    HashTableEntry *new_entry = (HashTableEntry *)malloc(sizeof(HashTableEntry));
    new_entry->pair = new_pair;
    new_entry->next = dict->table[hash];
    dict->table[hash] = new_entry;
}

char *search(Dictionary *dict, const char *key) {
    unsigned long hash = hash_function(key);
    HashTableEntry *entry = dict->table[hash];

    while (entry != NULL) {
        if (strcmp(entry->pair->key, key) == 0) {
            return entry->pair->value;
        }
        entry = entry->next;
    }
    return NULL;
}

void delete_key(Dictionary *dict, const char *key) {
    unsigned long hash = hash_function(key);
    HashTableEntry *entry = dict->table[hash];
    HashTableEntry *prev = NULL;

    while (entry != NULL) {
        if (strcmp(entry->pair->key, key) == 0) {
            if (prev == NULL) {
                dict->table[hash] = entry->next;
            } else {
                prev->next = entry->next;
            }
            free(entry->pair->key);
            free(entry->pair->value);
            free(entry->pair);
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

Dictionary *create_dictionary() {
    Dictionary *dict = (Dictionary *)malloc(sizeof(Dictionary));
    dict->table = (HashTableEntry **)malloc(TABLE_SIZE * sizeof(HashTableEntry *));
    for (int i = 0; i < TABLE_SIZE; i++) {
        dict->table[i] = NULL;
    }
    return dict;
}

void destroy_dictionary(Dictionary *dict) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashTableEntry *entry = dict->table[i];
        while (entry != NULL) {
            HashTableEntry *temp = entry;
            entry = entry->next;
            free(temp->pair->key);
            free(temp->pair->value);
            free(temp->pair);
            free(temp);
        }
    }
    free(dict->table);
    free(dict);
}


#pragma endregion


int get_length(char **p1)
{
  char *p = *p1;
  int len = 0;
  while(*p != '\r') {
      len = (len*10)+(*p - '0');
      p++;
  }
  p+=2;
  *p1 = p;
  return len;
}

void parse(int client_socket, char *p, Dictionary *dict)
{
  // *2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n
  // *1\r\n$4\r\nPING\r\n
  p++;
  int num = get_length(&p);
  std::cout << "num:" << num << std::endl;
  p++;
  int len = get_length(&p);
  std::cout << "len1:" << len << std::endl;
  if(memcmp(p, "ECHO", len) == 0)
  {
    p += len + 2;
    p++;
    len = get_length(&p);
    std::cout << "len2:" << len << std::endl;
    char *buffer = new char[len + 3];
    buffer[0] = '+';
    memcpy(buffer+1, p, len + 2);
    send(client_socket, buffer, len + 3, 0);
  }
  else if(memcmp(p, "PING", 4) == 0)
  {
    send(client_socket, "+PONG\r\n", 7, 0);
  }
  else if(memcmp(p, "SET", 3) == 0)
  {
    p += len + 2;
    p++;
    len = get_length(&p);
    std::cout << "len2:" << len << std::endl;
    char *buffer1 = new char[len];
    memcpy(buffer1, p, len);
    p+=len + 2 + 1;

    len = get_length(&p);
    std::cout << "len3:" << len << std::endl;
    char *buffer2 = new char[len];
    memcpy(buffer2, p, len);

    insert(dict, buffer1, buffer2);

    send(client_socket, "+OK\r\n", 5, 0);
    
  }
  else if(memcmp(p, "GET", 3) == 0)
  {
    p += len + 2;
    p++;
    len = get_length(&p);
    std::cout << "len2:" << len << std::endl;
    char *buffer1 = new char[len];
    memcpy(buffer1, p, len);

    char *ret = search(dict, buffer1);
    std::cout<<ret<<std::endl;
    int size = strlen(ret);
    char *buffer2 = new char[1024];
    sprintf(buffer2, "$%d\r\n%s\r\n", size, ret);
    send(client_socket, buffer2, 1024, 0);
  }
}

void handle_client(int client_socket) {
  char buffer[1024];
  int bytes_received;
  Dictionary *dict = create_dictionary();
  
  while(true){
    bytes_received = recv(client_socket, buffer, 1024, 0);
    if(bytes_received <= 0){
      break;
    }
    parse(client_socket, buffer, dict);
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
