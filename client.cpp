#include <bits/stdc++.h>
#include <unistd.h>
#include <arpa/inet.h>


using namespace std;

#define JOINPORT 8000
#define VIDEOPORT 8001
#define BUFFSIZE 2048

struct sockaddr_in servertcp;
struct sockaddr_in serverudp;
socklen_t serverlentcp;
socklen_t serverlenudp;
int sockfdtcp;
int sockfdudp;

void inittcp(char server_ip[]){
    sockfdtcp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdtcp < 0) {
        perror(" socket");
        exit(EXIT_FAILURE);
    }

    memset(&servertcp, 0, sizeof(servertcp));
    servertcp.sin_family = AF_INET;
    servertcp.sin_addr.s_addr = INADDR_ANY;
    servertcp.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, server_ip, &servertcp.sin_addr);

    if (bind(sockfdtcp, (struct sockaddr *)&servertcp, sizeof(servertcp)) < 0) {
        perror("bind");
        close(sockfdtcp);
        exit(EXIT_FAILURE);
    }
}

void initudp(char server_ip[]){
    sockfdudp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfdtcp < 0) {
        perror(" socket");
        exit(EXIT_FAILURE);
    }

    memset(&serverudp, 0, sizeof(serverudp));
    serverudp.sin_family = AF_INET;
    serverudp.sin_addr.s_addr = INADDR_ANY;
    serverudp.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, server_ip, &serverudp.sin_addr);

    if (bind(sockfdudp, (struct sockaddr *)&serverudp, sizeof(serverudp)) < 0) {
        perror("bind");
        close(sockfdudp);
        exit(EXIT_FAILURE);
    }
}

//roomid username
int main(int argc,char* argv[]){
    // if(argc!=3){
    //     cout<<"wrong input";
    //     return -1;
    // }
    


    
    // string username =argv[2];
    // if(username.size()>20){
    //     cout<<"max username len <= 20\n";
    //     return -1;
    // }
    // string room_id= argv[1];

    string username = "nayan";
    string room_id = "abcdef";
    char server_ip[] = "127.0.0.0";

    inittcp(server_ip);
    initudp(server_ip);
   
    // int buff[BUFFSIZE]={1};
    // sendto(sockfd,buff,sizeof(buff),0,(struct sockaddr * )&server,serverlen);
    // int n=recvfrom(sockfd,buff,BUFFSIZE,0,(struct sockaddr *)&udp,&udplen);
    // while(n<0){
    //     sendto(sockfd,buff,sizeof(buff),0,(struct sockaddr * )&server,serverlen);
    //     buff[0]=1;
    //     n=recvfrom(sockfd,buff,BUFFSIZE,0,(struct sockaddr *)&udp,&udplen);
    // }
    return 0;
};