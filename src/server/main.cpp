#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <mutex>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Forward declarations
class ChatRoom;
class User;

// Global mutex for thread safety
std::mutex mtx;

// User class definition
class User {
    std::string username;
    ChatRoom* current_room = nullptr;
    int socket_fd;
    
public:
    User(const std::string& name, int sock) : username(name), socket_fd(sock) {}
    
    void join_room(ChatRoom* room);
    void leave_room();
    
    ChatRoom* get_room() const { return current_room; }
    const std::string& get_name() const { return username; }
    int get_socket() const { return socket_fd; }
    
    void send_message(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mtx);
        int bytesSent = send(socket_fd, msg.c_str(), msg.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending message to " << username << std::endl;
        }
    }
};

// ChatRoom class definition
class ChatRoom {
    std::string room_name;
    std::vector<User*> users;
    
public:
    ChatRoom(const std::string& name) : room_name(name) {}
    
    const std::string& get_name() const { return room_name; }
    
    void add_user(User* user) {
        std::lock_guard<std::mutex> lock(mtx);
        if (std::find(users.begin(), users.end(), user) == users.end()) {
            users.push_back(user);
            user->join_room(this);
            broadcast_system_message(user->get_name() + " joined the room");
        }
    }
    
    void remove_user(User* user) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = std::find(users.begin(), users.end(), user);
        if (it != users.end()) {
            users.erase(it);
            broadcast_system_message(user->get_name() + " left the room");
        }
    }
    
    void broadcast(const std::string& sender, const std::string& message) {
        std::string formatted_msg = "[" + room_name + "] " + sender + ": " + message + "\n";
        std::lock_guard<std::mutex> lock(mtx);
        for (User* user : users) {
            if (user->get_name() != sender) {
                user->send_message(formatted_msg);
            }
        }
    }
    
    void broadcast_system_message(const std::string& message) {
        std::string formatted_msg = "[System] " + message + "\n";
        std::lock_guard<std::mutex> lock(mtx);
        for (User* user : users) {
            user->send_message(formatted_msg);
        }
    }
};

// Implement User methods that depend on ChatRoom
void User::join_room(ChatRoom* room) {
    if (current_room) {
        current_room->remove_user(this);
    }
    current_room = room;
}

void User::leave_room() {
    if (current_room) {
        current_room->remove_user(this);
        current_room = nullptr;
    }
}

// Server class to manage everything
class Server {
    std::unordered_map<std::string, std::unique_ptr<User>> users;
    std::unordered_map<std::string, std::unique_ptr<ChatRoom>> rooms;
    int server_socket;
    
public:
    Server() : server_socket(-1) {}
    
    void initialize() {
        struct sockaddr_in server_addr;
        
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            throw std::runtime_error("Socket creation failed");
        }
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);
        
        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            throw std::runtime_error("Bind failed");
        }
        
        if (listen(server_socket, MAX_CLIENTS) < 0) {
            throw std::runtime_error("Listen failed");
        }
        
        // Create default room
        create_room("General");
        
        std::cout << "Server is listening on port " << PORT << std::endl;
    }
    
    void create_room(const std::string& room_name) {
        std::lock_guard<std::mutex> lock(mtx);
        if (rooms.find(room_name) == rooms.end()) {
            rooms[room_name] = std::make_unique<ChatRoom>(room_name);
        }
    }
    
    void handle_client(int client_socket) {
        char buffer[BUFFER_SIZE];
        std::string username;
        User* user = nullptr;
        
        try {
            // Authentication phase
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) throw std::runtime_error("Client disconnected during auth");
            
            username = std::string(buffer, bytes_read);
            username.erase(std::remove(username.begin(), username.end(), '\n'), username.end());
            
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (users.find(username) != users.end()) {
                    send(client_socket, "ERROR: Username already taken\n", 30, 0);
                    throw std::runtime_error("Username " + username + " already taken");
                }
                
                users[username] = std::make_unique<User>(username, client_socket);
                user = users[username].get();
                send(client_socket, "AUTH_SUCCESS\n", 13, 0);
            }
            
            // Join default room
            rooms["General"]->add_user(user);
            
            // Message handling loop
            while (true) {
                memset(buffer, 0, BUFFER_SIZE);
                bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
                if (bytes_read <= 0) break;
                
                std::string message(buffer, bytes_read);
                message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
                
                // Handle commands
                if (message.rfind("/join ", 0) == 0) {
                    std::string room_name = message.substr(6);
                    if (rooms.find(room_name) != rooms.end()) {
                        rooms[room_name]->add_user(user);
                    } else {
                        user->send_message("Room does not exist\n");
                    }
                }
                else if (message == "/leave") {
                    if (user->get_room()) {
                        user->leave_room();
                    }
                }
                else if (message == "/rooms") {
                    std::string room_list = "Available rooms:\n";
                    for (const auto& room : rooms) {
                        room_list += "- " + room.first + "\n";
                    }
                    user->send_message(room_list);
                }
                else if (user->get_room()) {
                    user->get_room()->broadcast(username, message);
                }
                else {
                    user->send_message("You're not in any room. Use /join <room> to join one.\n");
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error with client " << username << ": " << e.what() << std::endl;
        }
        
        // Cleanup
        if (user) {
            user->leave_room();
            std::lock_guard<std::mutex> lock(mtx);
            users.erase(username);
        }
        close(client_socket);
    }
    
    void run() {
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
            if (client_socket < 0) {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::cout << "New client connected from " << client_ip << std::endl;
            
            std::thread([this, client_socket]() {
                this->handle_client(client_socket);
            }).detach();
        }
    }
    
    ~Server() {
        if (server_socket >= 0) close(server_socket);
    }
};

int main() {
    try {
        Server server;
        server.initialize();
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}