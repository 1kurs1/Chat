#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

// ws2_32.lib

std::mutex mtx;
std::vector<int> clientSockets;

void handleClient(int clientSocket){
    char buffer[1024];
    while(true){
        memset(buffer, 0, sizeof(buffer));
        int receivedBytes = recv(clientSocket,buffer, sizeof(buffer), 0); 
        if(receivedBytes <= 0){
            std::lock_guard<std::mutex> lock(mtx);
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            #ifdef _WIN32
            closesocket(clientSocket);
            #else
            close(clientSocket);
            #endif
            std::cout << "client disconnected! Clients: " << clientSockets.size() << '\n';
            break; 
        }

        std::lock_guard<std::mutex> lock(mtx);
        for(int sock: clientSockets){
            if(sock != clientSocket){
                send(sock, buffer, receivedBytes, 0);
            }
        }
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

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        std::cerr << "failed to create socket!\n";
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(12345);
    if(bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1){
        std::cerr << "failed to bind socket!\n";
        #ifdef _WIN32
        closesocket(serverSocket);
        #else
        close(serverSocket);
        #endif
        return EXIT_FAILURE;
    }

    if(listen(serverSocket, 5) == -1){
        std::cerr << "failed to listen on socket!\n";
        #ifdef _WIN32
        closesocket(serverSocket);
        #else
        close(serverSocket);
        #endif
        return EXIT_FAILURE;
    }

    std::cout << "server is started on port 12345...\n";

    while(true){
        sockaddr_in clientAddress{};
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
        if(clientSocket == -1){
            std::cerr << "failed to acccept connection from client!\n";
            continue;
        }

        std::lock_guard<std::mutex> lock(mtx);
        clientSockets.push_back(clientSocket);
        std::cout << "success! new client connected! Clients: " << clientSockets.size() <<'\n';

        std::thread(handleClient, clientSocket).detach();
    }

    #ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
    #else
    close(serverSocket);
    #endif
    return EXIT_SUCCESS;
}