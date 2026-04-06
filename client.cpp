#include <bits/stdc++.h>
#include <unistd.h>
#include <arpa/inet.h>


using namespace std;

#define JOINPORT 8000
#define VIDEOPORT 8001
#define BUFFSIZE 2048


//roomid username
int main(int argc,char* argv[]){
    // if(argc!=3){
    //     cout<<"wrong input";
    //     return -1;
    // }
    unordered_map<string,sockaddr_in> users;
    struct sockaddr_in discover;
    struct sockaddr_in server;
    socklen_t disclen,serverlen;
    int sockfd;

    
    // string username =argv[2];
    // if(username.size()>20){
    //     cout<<"max username len <= 20\n";
    //     return -1;
    // }
    // string room_id= argv[1];

    string username = "nayan";
    string room_id = "abcd";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&discover, 0, sizeof(discover));
    discover.sin_family = AF_INET;
    discover.sin_addr.s_addr = INADDR_ANY;
    discover.sin_port = htons(JOINPORT);
    inet_pton(AF_INET, "255.255.255.255", &discover.sin_addr);

    if (bind(sockfd, (struct sockaddr *)&discover, sizeof(discover)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int buff[BUFFSIZE]={1};
    sendto(sockfd,buff,sizeof(buff),0,(struct sockaddr * )&discover,disclen);
    int n=recvfrom(sockfd,buff,BUFFSIZE,0,(struct sockaddr *)&server,&serverlen);
    while(n<0){
        sendto(sockfd,buff,sizeof(buff),0,(struct sockaddr * )&discover,disclen);
        buff[0]=1;
        n=recvfrom(sockfd,buff,BUFFSIZE,0,(struct sockaddr *)&server,&serverlen);
    }
    return 0;
};