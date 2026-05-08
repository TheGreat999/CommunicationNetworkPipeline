#include <bits/stdc++.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iomanip>

using namespace std;

#define JOINPORT 8000
#define VIDEOPORT 8001
#define BUFFSIZE 4096

struct sockaddr_in servertcp;
struct sockaddr_in serverudp;
struct sockaddr_in broadcastaddr;
socklen_t serverlentcp;
socklen_t serverlenudp;
socklen_t broadcastlen;
int sockfdtcp;
int sockfdudp;
int broadcastfd;

void inittcp(string server_ip){
    sockfdtcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdtcp < 0) {
        perror(" socket");
        exit(EXIT_FAILURE);
    }

    memset(&servertcp, 0, sizeof(servertcp));
    servertcp.sin_family = AF_INET;
    servertcp.sin_addr.s_addr = INADDR_ANY;
    servertcp.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, server_ip.c_str(), &servertcp.sin_addr);

    if (bind(sockfdtcp, (struct sockaddr *)&servertcp, sizeof(servertcp)) < 0) {
        perror("bind");
        close(sockfdtcp);
        exit(EXIT_FAILURE);
    }
}

void initudp(string server_ip){
    sockfdudp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfdtcp < 0) {
        perror(" socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverudp, 0, sizeof(serverudp));
    serverudp.sin_family = AF_INET;
    serverudp.sin_addr.s_addr = INADDR_ANY;
    serverudp.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, server_ip.c_str(), &serverudp.sin_addr);

    if (bind(sockfdudp, (struct sockaddr *)&serverudp, sizeof(serverudp)) < 0) {
        perror("bind");
        close(sockfdudp);
        exit(EXIT_FAILURE);
    }
}

void initbroadcast(){
    broadcastfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfdtcp < 0) {
        perror(" socket");
        exit(EXIT_FAILURE);
    }

    memset(&broadcastaddr, 0, sizeof(broadcastaddr));
    broadcastaddr.sin_family = AF_INET;
    broadcastaddr.sin_addr.s_addr = INADDR_ANY;
    broadcastaddr.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, "255.255.255.255", &broadcastaddr.sin_addr);

    if (bind(broadcastfd, (struct sockaddr *)&broadcastaddr, sizeof(broadcastaddr)) < 0) {
        perror("bind");
        close(broadcastfd);
        exit(EXIT_FAILURE);
    }
}


int main(int argc,char* argv[]){
    if(argc!=2){
        cout<<"wrong input";
        return -1;
    }
    string username =argv[2];
    if(username.size()>20){
        cout<<"max username len <= 20\n";
        return -1;
    }

    string username = "nayan";
    string room_id = "abcdef";
    string server_ip = "127.0.0.0";

    
    initbroadcast();

    char tcpbuffer[BUFFSIZE];
    char udpbuffer[BUFFSIZE];


    timeval tv{};
    tv.tv_sec = 2;
    setsockopt(broadcastfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    vector<string> roomids;
    vector<string> server_ips;
    while(true){\

        char broad[] = "1";
        sendto(broadcastfd,broad,1,0, (sockaddr *)& broadcastaddr, broadcastlen);
        
        while(true){
            char roomid[6];
            int n = recvfrom(broadcastfd, roomid, 6, 0, (sockaddr*) &broadcastaddr, &broadcastlen);
            if(n < 0) break;
            roomids.push_back(string(roomid,n));
            server_ips.push_back(string(inet_ntoa(broadcastaddr.sin_addr)));

        }

        if(!roomids.empty())break;
    
        cout<<"No room found. Type \"yes\" to rescan again";
        string r;
        cin>>r;
        if(r != "yes")break;

    }

    cout << left << setw(5)  << "SN" << setw(10) << "ROOM" << setw(25) << "SERVER" << '\n';

    cout << string(40, '-') << '\n';

    for(int i = 0; i < roomids.size(); i++){
        cout << left << setw(5)  << i + 1 << setw(10) << roomids[i] << setw(25) << server_ips[i] << ':' << VIDEOPORT << '\n';
    }
    cout<<"Type the SN of the room you want to join\n";
    int i;
    while(true){
        cin>>i;
        i--;
        if(i >= 0 && i < roomids.size())break;

        cout<<"invalid SN please type a correct SN\n";

    }

    room_id = roomids[i];
    server_ip = server_ips[i];

    inittcp(server_ip);
    initudp(server_ip);

    
    return 0;
};