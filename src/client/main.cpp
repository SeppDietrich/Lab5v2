#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>

using namespace std;

void readThread(int sock) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(sock, buffer, sizeof(buffer));
        if (bytes <= 0) break;
        cout<< buffer << endl;
    }
}
void clearline(){
    std::cout << "\033[1A";
    std::cout << "\r";
    std::cout << "\033[2K";
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket error" << endl;
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("209.38.41.107");

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connect error" << endl;
        return -1;
    }
    string manual =
                "/auth [username] - authenticate \n"
                "/join [chatNumber] - join chat \n"
                "/message [message] - message sent into chat \n"
                "/leave - leave chat \n";
    cout << "Connected!" << endl;
    cout <<manual<<endl;

    
    thread reader(readThread, sock);
    cout << "Enter command: ";
    char msg[1024];
    while (true) {
        
        cin.getline(msg, sizeof(msg));
        clearline();
        send(sock, msg, strlen(msg), 0);
    }

    reader.join();
    close(sock);
    return 0;
}