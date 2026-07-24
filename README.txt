Only works on linux based distros running kernel version 2.5 and above due to use to epoll
Requirements : miniaudio library(provided with the source code)
Command to compile : 
    g++ -Wall -Wextra -std=c++17 server.cpp -o server
    g++ -Wall -Wextra -std=c++17 client.cpp input.cpp output.cpp -pthread -ldl -lm -o client