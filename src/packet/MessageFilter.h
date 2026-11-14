#pragma once

namespace no_vanilla_messages {

void setHideJoinMessage(bool value) noexcept;
void setHideLeaveMessage(bool value) noexcept;
void setHideSleepMessage(bool value) noexcept;
void setHideSkipNightMessage(bool value) noexcept;
void setHideDeathMessage(bool value) noexcept;
void setDebugLogging(bool value) noexcept;

void logHookTargets() noexcept;

bool installHooks() noexcept;
void uninstallHooks() noexcept;

} // namespace no_vanilla_messages
