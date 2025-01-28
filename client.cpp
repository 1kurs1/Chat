#include <iostream>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

void receiveMessages(int clientSocket){
    char buffer[1024];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        int receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if(receivedBytes <= 0){
            std::cerr << "connection lost!\n";
            break; 
        }
        std::cout << "Received: " << buffer << '\n';
    }
}

int main(){
    #ifdef _WIN32
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        std::cerr << "WSAStartup failed!\n";
        return EXIT_FAILURE;
    }
    #endif

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket == -1){
        std::cerr << "failed to create client socket!\n";
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if(connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
        std::cerr << "failed to connect to server!\n";
        #ifdef _WIN32
        closesocket(clientSocket);
        #else
        close(clientSocket);
        #endif
        return EXIT_FAILURE;
    }

    std::cout << "connected to server. Message:\n";

    std::thread(receiveMessages, clientSocket).detach();

    char buffer[1024];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        std::cin.getline(buffer, sizeof(buffer));send(clientSocket, buffer, strlen(buffer), 0);
    }

    #ifdef _WIN32
    closesocket(clientSocket);
    WSACleanup();
    #else
    close(clientSocket);
    #endif
    return EXIT_SUCCESS;
}