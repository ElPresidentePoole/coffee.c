#define _POSIX_C_SOURCE 202306L // stdio pls

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define PORT 13934
#define BACKLOG 3
#define MAX_HEADER_LENGTH 128 // good luck lol

// FIXME: handle broken pipes!

void print_and_die(char* s) {
  printf("Failed %s\n", s);
  exit(1);
}

long get_file_length(FILE* f) {
  fseek(f, 0L, SEEK_END);
  long size = ftell(f);
  rewind(f);
  return size;
}

char* build_response() {
  // Builds a char array/ptr to send to the connecting client
  // !! IT IS YOUR RESPONSIBILITY TO free() SAID PTR AFTERWARDS !!
  // Open file
  FILE* f = fopen("./index.html", "r");
  if(f == NULL) print_and_die("Failed to open file containing bullshit.");

  long page_contents_len = get_file_length(f);
  char* page_contents = malloc(sizeof(char) * page_contents_len);

  // Copy from file into page_contents, byte by byte
  int i = 0, c;
  while((c = fgetc(f)) != EOF) {
    page_contents[i] = c;
    i++;
  }
  // page_contents[page_contents_len] = '\0'; // is this necessary or is it redundant?

  char* header_format = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
  char* header_ptr = malloc(sizeof(char) * MAX_HEADER_LENGTH); // header_format is 80 char rn, will get bigger.  should handle up to 50 digits.
  sprintf(header_ptr, header_format, sizeof(char) * strlen(page_contents)); // do we need a null terminator or does it handle this?
  printf("HEADER: %s\n", header_ptr);

  char* full_response = calloc(page_contents_len + strlen(header_ptr) + 1, sizeof(char));
  strcat(full_response, header_ptr);
  strcat(full_response, page_contents);
  full_response[page_contents_len + strlen(header_ptr) + 1] = '\0';

  printf("FULL RESPONSE: %s\n", full_response);

  free(page_contents);
  free(header_ptr);
  fclose(f);

  return full_response;
}

void send_bullshit_to_client(int client_fd) {

  char* response = build_response();

  while(write(client_fd, response, sizeof(char) * strlen(response)) != -1);

  free(response);
}

int main() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  printf("%i", server_fd);
  if(server_fd == -1) print_and_die("socket");

  // if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, NULL, NULL) != 0) print_and_die("setsockopt");

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  if(bind(server_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) print_and_die("bind");

  if(listen(server_fd, BACKLOG) != 0) print_and_die("listen");

  bool server_running = true;
  while(server_running) {
    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if(client_fd < 0) print_and_die("accept");

    send_bullshit_to_client(client_fd);

    close(client_fd);
  }
  close(server_fd);

  return 0;
}
