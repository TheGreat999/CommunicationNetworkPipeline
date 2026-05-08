#include <bits/stdc++.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>

using namespace std;

#define CAP 2
#define JOINPORT 8000
#define VIDEOPORT 8001
#define BUFFSIZE 2048

#define eventqueuesize 128

string generateRoomID() {
    const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string id = "";
    for (int i = 0; i < 6; i++) {
        id += chars[rand() % chars.size()];
    }
    return id;
}

struct client{
    sockaddr_in sock;//tcp
    string username;
    int fd;
    client(sockaddr_in sock, string username, int fd) : sock(sock), username(username), fd(fd) {}
};

int epollfd = epoll_create1(0);
struct epoll_event events[eventqueuesize];

vector<client>clients;
struct sockaddr_in discover;//TCP
struct sockaddr_in room;//UDP
int sockfd_tcp;
int sockfd_udp;
socklen_t disclen;
socklen_t roomlen;

enum AdminState{
    WAIT_COMMAND,
    WAIT_REMOVE_USERNAME,
    WAIT_BROADCAST_MESSAGE
};

void TCPBroadcast(string msg){
    for(client c : clients){
        const char* m = msg.c_str();
        send(c.fd, m, sizeof(m), 0);
    }
}

void addToEpoll(int fd){
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev);
}

void removeUser(int fd){
    int idx = 0;
    for(auto& c: clients){
        if(fd == c.fd){
            close(fd);
            break;
        }
        idx ++;
    }

    if(idx == clients.size()){
        perror("Message received from an unknown client\n");
        return;
    }
    
    clients.erase(clients.begin() + idx);
    string m = clients[idx].username + " left the room.\n";
    cout<<m;
    TCPBroadcast(m);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
}
void removeUser(string username){
    int idx = 0;
    for(auto& c:clients){
        if(c.username == username){
            char msg[] = "The admin removed you\n";
            send(c.fd ,msg ,sizeof(msg),0);
            close(c.fd);
            break;
        }
        idx ++;
    }
    if(idx == clients.size()){
        cout<<"User not found\n";
        return;
    }
    string m = clients[idx].username + " left the room.\n";
    cout<<"Successfully removed "<<clients[idx].username<<'\n';
    clients.erase(clients.begin() + idx);
    TCPBroadcast(m);
}


void initstdio(){           
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
    addToEpoll(0);
} 

void initTCPsock(){
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_tcp < 0) {
        perror("tcp socket");
        exit(EXIT_FAILURE);
    }
    memset(&discover, 0, sizeof(discover));
    discover.sin_family = AF_INET;
    discover.sin_addr.s_addr = INADDR_ANY;
    discover.sin_port = htons(JOINPORT);

    if (bind(sockfd_tcp, (struct sockaddr *)&discover, sizeof(discover)) < 0) {
        perror("tcp bind");
        close(sockfd_tcp);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd_tcp, CAP) < 0) { 
        perror("Listen failed");
        close(sockfd_tcp);
        exit(EXIT_FAILURE);
    }

    //adding server tcp fd in events
    fcntl(sockfd_tcp, F_SETFL, O_NONBLOCK);
    addToEpoll(sockfd_tcp);


}

void initUDPsock(){
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_udp < 0) {
        perror("udp socket");
        exit(EXIT_FAILURE);
    }

    memset(&room, 0, sizeof(room));
    room.sin_family = AF_INET;
    room.sin_addr.s_addr = INADDR_ANY;
    room.sin_port = htons(VIDEOPORT);

    if (bind(sockfd_udp, (struct sockaddr *)&room, sizeof(room)) < 0) {
        perror("bind");
        close(sockfd_tcp);
        exit(EXIT_FAILURE);
    }

    //adding server udp fd in events
    fcntl(sockfd_udp, F_SETFL, O_NONBLOCK);
    addToEpoll(sockfd_udp);

}



int main(){

    if(epollfd == -1){
        cout<<"cant create an epoll\n";
        return -1;
    }

    initstdio();
    initTCPsock();
    initUDPsock();
    
    string room_id = "abcdef";

    cout<<"Room created with room id \""<<room_id<<"\" on port "<<JOINPORT<<"\n";

    string control_panel = R"(
----------CONTROL PANEL----------
Enter command to perform an action
    remove : remove an user
    close : close the meeting
    list : list all users
    broadcast : broadcast a message to all users

)";
    cout<<control_panel;
    AdminState admin_state = WAIT_COMMAND;
    
    while(1){
        int n = epoll_wait(epollfd,events,eventqueuesize,-1);
        for(int i=0; i<n; i++){
            int fd = events[i].data.fd;

            if(fd==0){//stdin
                
                char com[BUFFSIZE];
                int n = read(fd,com,BUFFSIZE);
                string input(com, n);
                if(!input.empty() && input.back() == '\n') input.pop_back();
                switch(admin_state){

                    case WAIT_COMMAND:

                        if(input == "remove"){
                            cout << "Enter username\n";
                            admin_state = WAIT_REMOVE_USERNAME;
                        }

                        else if(input == "broadcast"){
                            cout << "Enter message\n";
                            admin_state = WAIT_BROADCAST_MESSAGE;
                        }

                        else if(input == "list"){
                            for(client c : clients){
                                cout<<c.username<<' ';
                            }
                            cout<<'\n';
                            cout<<control_panel;
                        }

                        else if(input == "close"){
                            string m ="The room is closed by the admin\n";
                            TCPBroadcast(m);
                            for(client c : clients){
                                removeUser(c.fd);
                            }
                            close(sockfd_tcp);
                            return 0;
                        }
                        else{
                            cout << "Unknown command\n";
                        }

                        break;

                    case WAIT_REMOVE_USERNAME:
                        removeUser(input);
                        admin_state = WAIT_COMMAND;
                        cout<<control_panel;
                        break;

                    case WAIT_BROADCAST_MESSAGE:
                        TCPBroadcast(input);
                        admin_state = WAIT_COMMAND;
                        cout<<control_panel;
                        break;

                }

                //do things

                // cout<<"console boilerplate\n";
            }

            else if(fd == sockfd_tcp){//new client joins
                while(1){
                    sockaddr_in client_addr;
                    socklen_t addrlen = sizeof(client_addr);

                    int clientfd = accept(sockfd_tcp, (sockaddr*)&client_addr, &addrlen);

                    if (clientfd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // no more pending clients
                        } else {
                            perror("client not accepted\n");
                            break;

                        }
                    }

                    if(clients.size()==CAP){
                        char msg[] = "sorry the room is full\n";
                        send(clientfd ,msg ,sizeof(msg),0);
                        close(clientfd);
                    }

                    //make non-blocking
                    fcntl(clientfd, F_SETFL, O_NONBLOCK);
    
                    //validating user
                    char buffer[BUFFSIZE];
                    int n = read(clientfd, buffer, BUFFSIZE);
                    if(n<=6){
                        close(clientfd); 
                        continue;
                    }
                    string recvid(buffer,6);
                    if(recvid != room_id){
                        char msg[] = "incorrect roomid\n";
                        send(clientfd ,msg ,sizeof(msg),0);
                        close(clientfd);
                    }
                    string username(buffer+6,n-6);
                    for(auto& c:clients){
                        if(c.username == username){
                            char msg[] = "user already present\n";
                            send(clientfd ,msg ,sizeof(msg),0);
                            close(clientfd);
                        }
                    }
                    char msg[] = "You joined successfully\n";
                    send(clientfd, msg, sizeof(msg), 0);
                    cout<<"user "<<username<<" joined\n";

                    string m = username + " joined the room.\n";
                    for(client c : clients){
                        send(c.fd, m.c_str(),sizeof(m.c_str()),0);
                    }
                    // create client object
                    client new_client(client_addr, username, clientfd);
                    clients.push_back(new_client);

                    //adding to epoll
                    addToEpoll(clientfd);
                }
            }

            else if(fd == sockfd_udp){//video data receive

            }

            else{//user want to leave
                removeUser(fd);
            }
        }
    }

    

    return 0;
};      