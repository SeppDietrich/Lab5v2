#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

#define PORT 8080
#define MAX_CLIENTS 10

struct User {
    int socket;
    std::string roomID ="";
    std::string username= "";

    User(int socket) : socket(socket) {}
};
class Logger{
public:
    Logger(){}
    ~Logger(){}

    void saveMessage(std::string roomID, std::string message){
        std::string path="rooms/"+roomID+".txt";
        ofstream room;
        room.open(path.c_str(), fstream::app)
        room<<message<<"\n";
        room.close();
    }
};

class Server {
public:
    std::vector<User*> users;
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    Logger logger;

    Server() {}
    ~Server() {
        for (auto user : users) {
            delete user;
        }
    }

    void sendMessage(const std::string& message, int socket) {
        int bytesSent = send(socket, message.c_str(), message.size(), 0);  
        if (bytesSent < 0) {
            std::cerr << "Error sending message to client" << std::endl;
        }  
    }
    void broadcastMessage(std::string message, User* user){
        for(int i =0;i<users.size();i++){
            if(users[i]->roomID==user->roomID&& users[i]->socket!=user->socket){
                sendMessage(message, users[i]->socket);
            }
        }
        logger.saveMessage(user->roomID, message);

    }

    bool authenticationSuccess(std::string sentUsername, User* user) {  
        if (sentUsername.length() > 100) {
            return false;
        }
        user->username = sentUsername;  
        std::cout << "Client authenticated as: " << user->username << std::endl;
        return true;
    }
    bool joinSuccess(std::string roomID, User* user){
        if(roomID.length()>20){
            return false;
        }
        user->roomID=roomID;
        std::cout<<"Client "<< user->username<<" joined room: "<< user->roomID<<std::endl;
        return true;
    }
    bool controlClient(User* user){
        if(user->username==""){
            return false;
        }else if(user->roomID==""){
            return false;
        }
        return true;
    }

    void parseMessage(std::string message, std::string &returnMessage, User* user) {
        message.erase(message.find_last_not_of(" \t\n\r\f\v") + 1);
        if (message[0] != '/') {
            std::string manual =
                "/auth [username] - authenticate \n"
                "/join [chatNumber] - join chat \n"
                "/message [message] - message sent into chat \n"
                "/leave - leave chat \n";
            
            returnMessage = manual;
            return;
        }

        int spacePos = message.find(' ');
        std::string command = message.substr(0, spacePos);
        std::string data = (spacePos != std::string::npos) ? message.substr(spacePos + 1) : "";

        switch(command[1]) {
            case 'a': // /auth
                if(authenticationSuccess(data, user)) {
                    returnMessage= "Successfully logged in as " + data + "\n";
                }else{
                    returnMessage= "Authentication Failed";
                }
            break;
            
                
            case 'j': // /join
                if(joinSuccess(data, user)){
                    returnMessage="Successfully joined room: " + data + "\n";
                }else{
                    returnMessage="Failed to join room : " + data;   
                }
            break;
            case 'm': // /message
                if(controlClient(user)){
                    std::string finalMessage =user->username+ " : "+ data;
                    broadcastMessage(finalMessage, user);
                    returnMessage=finalMessage;
                }else{
                    returnMessage= "Client is neither authenticate or joined a room ";
                }
                
            break;
            // case 'l':{ // /leave
            //     return "Leaving current chat";
            // }
            default:
                returnMessage="Unknown command: " + command;
            break;
        }
    }

    void handleClient(User* user) {
        char buffer[1024];

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesRead = read(user->socket, buffer, sizeof(buffer));
            if (bytesRead <= 0) {
                std::cout << "Client disconnected." << std::endl;
                break;
            }
            buffer[bytesRead] = '\0';

            std::string message(buffer);
            std::string response;
            
            parseMessage(message, response, user);
            sendMessage(response, user->socket); 
        }
        close(user->socket);
    }

    void run() {
        while (true) {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket < 0) {
                std::cerr << "Accept failed." << std::endl;
                continue;
            }

            User* newUser = new User(clientSocket);
            users.push_back(newUser);

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            std::cout << "New client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;

            std::thread clientThread(&Server::handleClient, this, newUser);
            clientThread.detach();
        }

        close(serverSocket);
    }
    
    void initialize() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Socket creation failed." << std::endl;
            return;
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Bind failed." << std::endl;
            return;
        }

        if (listen(serverSocket, MAX_CLIENTS) < 0) {
            std::cerr << "Listen failed." << std::endl;
            return;
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