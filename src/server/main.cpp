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
    User(int socket): socket(socket){}

};


class Server
{
public:
    std::vector<User*> users;

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    Server();
    ~Server();
    void sendMessage(const std::string& message, int socket){
        int bytesSent = send(socket, message.c_str(), message.size(), 0);  
        if (bytesSent < 0) {
            std::cerr << "Error sending message to client" << std::endl;
        }  
    }
    bool authenticationSuccess(std::string sentUsername, User user) {  
        
        if(sentUsername.length()>100){
            return false;
        }
        user.username = sentUsername;  
        std::cout<<"Client authenticated as : " <<user.username<<std::endl;
        return true;
    }


    void parseMessage(std::string message, std::string &returnMessage, User user){
        message.erase(message.find_last_not_of(" \t\n\r\f\v") + 1);
        if(message[0]!='/'){
            std::string manual=
                "/auth [username] - authenticate \n"
                "/join [chatNumber] - join chat \n"
                "/message [message] -message sent into chat \n"
                "/leave -leave chat \n";
            
            returnMessage= manual;
        }
        int spacePos = message.find(' ');
        std::string command = message.substr(0, spacePos);
        std::string data = (spacePos != std::string::npos) ? message.substr(spacePos + 1) : "";

        
        switch(command[1]) {
            case 'a':{ // /auth
                if(authenticationSuccess(data, user)) {
                    returnMessage= "Successfully logged in as " + data + "\n";
                }
                returnMessage= "Authentication Failed";
            }
                
            // case 'j':{ // /join
            //     int roomId=std::stoi(data);
            //     roomHandler.joinRoom(this, data);

            //     return "Joining chat: " + data;
            // }
            // case 'm':{ // /message
                
            //     return "Sending message: " + data;
            // }
            // case 'l':{ // /leave
            //     return "Leaving current chat";
            // }
            default:{
                returnMessage="Unknown command: " + command;
            }
        }

        
    }

    void run(){
        while (true) {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket < 0) {
                std::cerr << "Accept failed." << std::endl;
                continue;
            }
            users.emplace_back(clientSocket);
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            std::cout << "New client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;

            std::thread clientThread(handleClient, clientSocket, users.end());
            clientThread.detach();
        }

        close(serverSocket);
    }
    
    void static handleClient(int socket, User user) {
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
            
            
            
            parseMessage(message, message, user);
        
            //std::cout << fullMessage << "\n";
            
            sendMessage(fullMessage, socket); 
        }
        close(socket);
    }
    
    void initialize(){

        

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






int main() {
    Server server;
    server.initialize();
    server.run();

    
    return 0;
}