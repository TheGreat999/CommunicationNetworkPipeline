#include <bits/stdc++.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iomanip>
#include <sys/epoll.h>

#include "input.hpp"
#include "output.hpp"

using namespace std;

#define JOINPORT 8000
#define VIDEOPORT 8001
#define BroadCASTPORT 8001
#define BUFFSIZE 4096
#define eventqueuesize 128

int epollfd = epoll_create1(0);
struct epoll_event events[eventqueuesize];
struct sockaddr_in servertcp;
struct sockaddr_in serverudp;
struct sockaddr_in localudp;
struct sockaddr_in broadcastaddr;
socklen_t serverlentcp = sizeof(servertcp);
socklen_t serverlenudp = sizeof(serverudp);
socklen_t localudp_len = sizeof(localudp);
socklen_t broadcastlen = sizeof(broadcastaddr);
int sockfdtcp;
int sockfdudp;
int broadcastfd;

void inittcp(string server_ip){
    sockfdtcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdtcp < 0) {
        perror("TCP socket");
        exit(EXIT_FAILURE);
    }

    memset(&servertcp, 0, sizeof(servertcp));
    servertcp.sin_family = AF_INET;
    servertcp.sin_addr.s_addr = INADDR_ANY;
    servertcp.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, server_ip.c_str(), &servertcp.sin_addr);

    if (connect(sockfdtcp, (sockaddr* )&servertcp,serverlentcp) < 0) {
        perror("TCP Connection failed");
        exit(EXIT_FAILURE);
    }
}

void initudp(string server_ip){
    sockfdudp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfdudp < 0) {
        perror("UDP socket");
        exit(EXIT_FAILURE);
    }

    memset(&localudp, 0, sizeof(localudp));
    localudp.sin_family = AF_INET;
    localudp.sin_addr.s_addr = INADDR_ANY;
    localudp.sin_port = htons(0);

    if (bind(sockfdudp, (struct sockaddr *)&localudp, sizeof(localudp)) < 0) {
        perror("UDP bind");
        close(sockfdudp);
        exit(EXIT_FAILURE);
    }

    memset(&serverudp, 0, sizeof(serverudp));
    serverudp.sin_family = AF_INET;
    serverudp.sin_port = htons(VIDEOPORT);
    inet_pton(AF_INET, server_ip.c_str(), &serverudp.sin_addr);
}

void initbroadcast(){
    broadcastfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastfd < 0) {
        perror("Broadcast socket");
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    setsockopt(broadcastfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    memset(&broadcastaddr, 0, sizeof(broadcastaddr));
    broadcastaddr.sin_family = AF_INET;
    broadcastaddr.sin_addr.s_addr = INADDR_ANY;
    broadcastaddr.sin_port = htons(BroadCASTPORT);
    inet_pton(AF_INET, "255.255.255.255", &broadcastaddr.sin_addr);

    broadcastlen = sizeof(broadcastaddr);
}

void initEpoll(){
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfdtcp;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfdtcp, &ev);

    ev.events = EPOLLIN;
    ev.data.fd = sockfdudp;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfdudp, &ev);
}

int main(int argc,char* argv[]){
    if(argc!=2){
        cout<<"wrong input";
        return -1;
    }
    string username =argv[1];
    if(username.size()>20){
        cout<<"max username len <= 20\n";
        return -1;
    }

    // string username = "nayan";
    // string room_id = "abcdef";
    // string server_ip = "127.0.0.0";

    
    initbroadcast();
    timeval tv{};
    tv.tv_sec = 1;
    setsockopt(broadcastfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    vector<string> roomids;
    vector<string> server_ips;
    while(true){

        char broad[] = "1";
        sendto(broadcastfd,broad,1,0, (sockaddr *)& broadcastaddr, broadcastlen);
        
        while(true){
            char roomid[7];
            int n = recvfrom(broadcastfd, roomid, 7, 0,(sockaddr* ) &serverudp, &serverlenudp);
            if(n < 0) break;
            roomids.push_back(string(roomid + 1,n - 1));
            server_ips.push_back(string(inet_ntoa(serverudp.sin_addr)));

        }

        if(!roomids.empty())break;
    
        cout<<"No room found. Type \"yes\" to rescan again ";
        string r;
        getline(cin,r);
        if(r.empty() || r != "yes"){
            close(broadcastfd);
            return 0;
        }

    }

    cout << left << setw(5)  << "SN" << setw(10) << "ROOM" << setw(25) << "SERVER" << '\n';
    cout << string(40, '-') << '\n';
    for(size_t i = 0; i < roomids.size(); i++){
        string socket = server_ips[i] + ":" + to_string(VIDEOPORT);
        cout << left << setw(5)  << i + 1 << setw(10) << roomids[i] << setw(25) << socket << '\n';
    }
    cout<<"Type the SN of the room you want to join\n";
    int i;
    while(true){
        cin>>i;
        i--;
        if(i >= 0 && static_cast<size_t>(i) < roomids.size())break;

        cout<<"invalid SN please type a correct SN\n";

    }


    close(broadcastfd);

    char tcpbuffer[BUFFSIZE];
    char udpbuffer[BUFFSIZE];

    string room_id = roomids[i];
    string server_ip = server_ips[i];

    inittcp(server_ip);
    initudp(server_ip);
    string joinmsg = room_id + username;
    sendto(sockfdtcp, joinmsg.c_str(), joinmsg.size(), 0, (sockaddr* )& servertcp, serverlentcp);
    int n = recvfrom(sockfdtcp, tcpbuffer, BUFFSIZE, 0, (sockaddr* )& servertcp, &serverlentcp);
    cout<<string(tcpbuffer,n);

    initEpoll();

    atomic<bool> running(true);

    startMicStream(sockfdudp, serverudp, username, running);

    while(running){
        int n = epoll_wait(epollfd,events,eventqueuesize,-1);
        for(int i=0; i<n; i++){
            int fd = events[i].data.fd;
            
            if(fd == sockfdtcp){// Message from server
                int n = recvfrom(sockfdtcp, tcpbuffer, BUFFSIZE, 0, (sockaddr* )& servertcp, &serverlentcp);
                if(n <= 0){
                    running = false;
                    break;
                }

                if(tcpbuffer[0] == '0'){
                    cout<<"You are removed from the room by the admin. \n";
                    running = false;
                    close(sockfdtcp);
                    close(sockfdudp);
                    return 0;
                }
                cout<<"Message from server : ";
                cout<<string(tcpbuffer, n)<<'\n';
            }
            else{// Raw stream data
                sockaddr_in from_addr;
                socklen_t from_len = sizeof(from_addr);
                int n = recvfrom(sockfdudp, udpbuffer, BUFFSIZE, 0, (sockaddr* )&from_addr, &from_len);
                if(n <= 0){
                    running = false;
                    close(sockfdtcp);
                    close(sockfdudp);
                    return 0;
                }

                if(udpbuffer[0] == 0){
                    if(n < 2) continue;
                    uint8_t sz = udpbuffer[1];
                    if(sz > n - 2) continue;

                    string sender(udpbuffer + 2, udpbuffer + 2 + sz);
                    uint8_t* data = (uint8_t*)(udpbuffer + 2 + sz);
                    int datasize = n - 2 - sz;

                    processOutput(sender, data, datasize);
                }
            }
        }
    }
    close(sockfdtcp);
    close(sockfdudp);
    return 0;   
};
