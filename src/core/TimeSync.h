#pragma once

#include <Arduino.h>

namespace TimeSync {

void updateFromRemote(uint32_t remoteSeconds);
uint32_t now();

}

