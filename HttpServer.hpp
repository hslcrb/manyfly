#pragma once
#include "BackgroundWorker.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>


#pragma comment(lib, "Ws2_32.lib")

class HttpServer : public BackgroundWorker {
public:
  int port;
  SOCKET ListenSocket;
  HttpServer(int port = 8080) : port(port), ListenSocket(INVALID_SOCKET) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
  }
  ~HttpServer() {
    if (ListenSocket != INVALID_SOCKET)
      closesocket(ListenSocket);
    WSACleanup();
  }
  virtual void run() {
    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char portStr[10];
    sprintf(portStr, "%d", port);
    if (getaddrinfo(NULL, portStr, &hints, &result) != 0)
      return;

    ListenSocket =
        socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
      freeaddrinfo(result);
      return;
    }

    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) ==
        SOCKET_ERROR) {
      freeaddrinfo(result);
      closesocket(ListenSocket);
      return;
    }
    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
      closesocket(ListenSocket);
      return;
    }

    while (!this->get_cancelled()) {
      SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
      if (ClientSocket == INVALID_SOCKET) {
        break;
      }
      // Parse HTTP GET Request and perform match query using SearchWorker logic
      const char *httpResponse =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "Connection: close\r\n\r\n"
          "<html><body><h1>SwiftSearch Server Running</h1></body></html>";
      send(ClientSocket, httpResponse, (int)strlen(httpResponse), 0);
      closesocket(ClientSocket);
    }
  }
};
