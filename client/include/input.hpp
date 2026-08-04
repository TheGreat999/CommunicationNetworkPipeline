#include "miniaudio.h"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <thread>

using namespace std;

void proccessInput(int sockfd, const sockaddr_in& serveraddr, const string& username, atomic<bool>& running);
