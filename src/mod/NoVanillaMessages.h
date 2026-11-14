#pragma once

#include "ll/api/mod/NativeMod.h"

namespace no_vanilla_messages {

class NoVanillaMessages {
public:
    static NoVanillaMessages& getInstance();

    NoVanillaMessages() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    /// @return True if the mod is unloaded successfully.
    bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace no_vanilla_messages
