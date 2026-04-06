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

void initstdio(){           
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &ev);
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
    epoll_event ev1;
    ev1.events = EPOLLIN | EPOLLRDHUP;
    ev1.data.fd = sockfd_tcp;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd_tcp, &ev1);


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
    epoll_event ev2;
    ev2.events = EPOLLIN | EPOLLRDHUP;
    ev2.data.fd = sockfd_udp;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd_udp, &ev2);

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
)";
    cout<<control_panel;
    
    while(1){
        int n = epoll_wait(epollfd,events,eventqueuesize,-1);
        for(int i=0; i<n; i++){
            int fd = events[i].data.fd;

            if(fd==0){//stdin
                
                string command;
                cin>>command;
                // read(fd,)
                if(command == "remove"){
                    cout<<"Enter username of the user to remove\n";
                    string user;

                    cin>>user;
                    bool found = false;
                    for(auto& c:clients){
                        if(c.username == user){
                            char msg[] = "The admin removed you\n";
                            send(c.fd ,msg ,sizeof(msg),0);
                            close(c.fd);
                            found = true;
                            break;
                        }
                    }
                    if(!found){
                        cout<<"User not found\n";
                    }
                }
                cout<<control_panel;
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

                    // create client object
                    client new_client(client_addr, username, clientfd);
                    clients.push_back(new_client);

                    //adding to epoll
                    epoll_event ev;
                    ev.events = EPOLLIN | EPOLLRDHUP;
                    ev.data.fd = clientfd;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
                }
            }

            else if(fd == sockfd_udp){//video data receive

            }

            else{//user want to leave

            }
        }
    }

    

    return 0;
};      