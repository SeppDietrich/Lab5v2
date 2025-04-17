#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>


#define PORT 8080
#define MAX_CLIENTS 10

struct User{
    int socket;
    std::string username;

}


class Server
{
public:
    std::vector<User*> users;
    Server();
    ~Server();

    void run(){
        while (true) {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket < 0) {
                std::cerr << "Accept failed." << std::endl;
                continue;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            std::cout << "New client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;

            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();
        }

        close(serverSocket);
    }
    
    void handleClient(int socket) {
        char buffer[1024];

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesRead = read(socket, buffer, sizeof(buffer));
            if (bytesRead <= 0) {
                std::cout << "Client disconnected." << std::endl;
                break;
            }
            buffer[bytesRead] = '\0';

            
            // Trim any trailing newlines or whitespace
            std::string message(buffer);
            
            
            
            std::string fullMessage = message;
        
            //std::cout << fullMessage << "\n";
            
            sendMessage(fullMessage, socket); 
        }
        close(socket);
    }
    
    void initialize(){

        int serverSocket, clientSocket;
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Socket creation failed." << std::endl;
            return -1;
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Bind failed." << std::endl;
            return -1;
        }

        if (listen(serverSocket, MAX_CLIENTS) < 0) {
            std::cerr << "Listen failed." << std::endl;
            return -1;
        }

        std::cout << "Server is listening on port " << PORT << std::endl;
    }

    
};


void sendMessage(const std::string& message, int socket){
        int bytesSent = send(socket, message.c_str(), message.size(), 0);  
        if (bytesSent < 0) {
            std::cerr << "Error sending message to client" << std::endl;
        }  
    }



int main() {
    

    
    return 0;
}