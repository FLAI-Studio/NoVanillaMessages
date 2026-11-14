#pragma once

namespace no_vanilla_messages {

struct Config {
    int  version              = 4;
    bool hideJoinMessage      = true;
    bool hideLeaveMessage     = true;
    bool hideSleepMessage     = true;
    bool hideSkipNightMessage = true;
    bool hideDeathMessage     = true;
    bool debug                = false;
};

} // namespace no_vanilla_messages
