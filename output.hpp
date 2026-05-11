#include "miniaudio.h"

#include <vector>
#include <queue>
#include <mutex>
#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>

using namespace std;

void processOutput(const string& username, const uint8_t* data, int datalen);