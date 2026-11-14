#include "packet/MessageFilter.h"

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "ll/api/io/Logger.h"
#include "ll/api/io/LoggerRegistry.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/memory/Symbol.h"
#include "ll/api/utils/SystemUtils.h"

#include "mc/common/SubClientId.h"
#include "mc/network/LoopbackPacketSender.h"
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/NetworkIdentifier.h"
#include "mc/network/NetworkIdentifierWithSubId.h"
#include "mc/network/Packet.h"
#include "mc/network/packet/TextPacket.h"
#include "mc/network/packet/TextPacketPayload.h"

using namespace ll::memory_literals;

namespace no_vanilla_messages {
namespace detail {

std::atomic_bool gHideJoinMessage{true};
std::atomic_bool gHideLeaveMessage{true};
std::atomic_bool gHideSleepMessage{true};
std::atomic_bool gHideSkipNightMessage{true};
std::atomic_bool gHideDeathMessage{true};
std::atomic_bool gDebugLogging{false};

constexpr std::string_view JoinedMessageKey{"multiplayer.player.joined"};
constexpr std::string_view LeftMessageKey{"multiplayer.player.left"};
constexpr std::string_view SleepingMessageKey{"chat.type.sleeping"};
constexpr std::string_view SkipNightMessageKey{"multiplayer.playersSkippingNight"};
constexpr std::string_view DeathMessageKeyPrefix{"death."};

enum class MessageKind : unsigned char { None, Joined, Left, Sleeping, SkippingNight, Died };

[[nodiscard]] std::shared_ptr<ll::io::Logger> logger() {
    static auto instance = ll::io::LoggerRegistry::getInstance().getOrCreate("NoVanillaMessages");
    return instance;
}

[[nodiscard]] std::string_view stripColorCodes(std::string_view text) {
    while (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xC2
           && static_cast<unsigned char>(text[1]) == 0xA7) {
        text.remove_prefix(3);
    }
    return text;
}

[[nodiscard]] bool isChatLike(TextPacketType type) {
    return type == TextPacketType::Chat || type == TextPacketType::Whisper;
}

[[nodiscard]] bool containsJsonTranslate(std::string_view message, std::string_view keyPrefix) {
    constexpr std::string_view Marker{"\"translate\":\""};
    auto const                 pos = message.find(Marker);
    if (pos == std::string_view::npos) {
        return false;
    }
    return message.substr(pos + Marker.size()).starts_with(keyPrefix);
}

[[nodiscard]] MessageKind classify(Packet const& packet) {
    if (packet.getId() != MinecraftPacketIds::Text) {
        return MessageKind::None;
    }

    auto const& text = static_cast<TextPacket const&>(packet);

    auto const [type, message] = std::visit(
        [](auto const& body) {
            return std::pair{static_cast<TextPacketType>(body.mType), std::string_view{body.mMessage.get()}};
        },
        text.mBody.get()
    );

    if (gDebugLogging.load(std::memory_order_relaxed)) {
        logger()->info(
            "[diag] outbound text packet, type={} text: {}",
            static_cast<int>(type),
            message.substr(0, 96)
        );
    }

    if (isChatLike(type)) {
        return MessageKind::None;
    }

    std::string_view head = stripColorCodes(message);
    if (head.starts_with('%')) {
        head.remove_prefix(1);
    }

    if (head.starts_with(JoinedMessageKey)) {
        return MessageKind::Joined;
    }
    if (head.starts_with(LeftMessageKey)) {
        return MessageKind::Left;
    }
    if (head.starts_with(SleepingMessageKey) || containsJsonTranslate(message, SleepingMessageKey)) {
        return MessageKind::Sleeping;
    }
    if (head.starts_with(SkipNightMessageKey) || containsJsonTranslate(message, SkipNightMessageKey)) {
        return MessageKind::SkippingNight;
    }
    if (head.starts_with(DeathMessageKeyPrefix) || containsJsonTranslate(message, DeathMessageKeyPrefix)) {
        return MessageKind::Died;
    }
    return MessageKind::None;
}

[[nodiscard]] std::string_view kindName(MessageKind kind) {
    switch (kind) {
    case MessageKind::Joined:
        return "join";
    case MessageKind::Left:
        return "leave";
    case MessageKind::Sleeping:
        return "sleep";
    case MessageKind::SkippingNight:
        return "skip night";
    case MessageKind::Died:
        return "death";
    default:
        return "unknown";
    }
}

[[nodiscard]] bool isEnabled(MessageKind kind) {
    switch (kind) {
    case MessageKind::Joined:
        return gHideJoinMessage.load(std::memory_order_relaxed);
    case MessageKind::Left:
        return gHideLeaveMessage.load(std::memory_order_relaxed);
    case MessageKind::Sleeping:
        return gHideSleepMessage.load(std::memory_order_relaxed);
    case MessageKind::SkippingNight:
        return gHideSkipNightMessage.load(std::memory_order_relaxed);
    case MessageKind::Died:
        return gHideDeathMessage.load(std::memory_order_relaxed);
    default:
        return false;
    }
}

[[nodiscard]] bool shouldDrop(Packet const& packet) {
    MessageKind const kind = classify(packet);
    if (kind == MessageKind::None || !isEnabled(kind)) {
        return false;
    }

    if (gDebugLogging.load(std::memory_order_relaxed)) {
        logger()->info("Hid the vanilla {} announcement", kindName(kind));
    }
    return true;
}

} // namespace detail

LL_TYPE_INSTANCE_HOOK(
    SendBroadcastHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    "?$sendBroadcast@LoopbackPacketSender@@QEAAXAEBVPacket@@@Z"_sym,
    void,
    Packet const& packet
) {
    if (detail::shouldDrop(packet)) {
        return;
    }
    origin(packet);
}

LL_TYPE_INSTANCE_HOOK(
    SendToClientsHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    "?$sendToClients@LoopbackPacketSender@@QEAAXAEBV?$vector@UNetworkIdentifierWithSubId@@V?$allocator@"
    "UNetworkIdentifierWithSubId@@@std@@@std@@AEBVPacket@@@Z"_sym,
    void,
    std::vector<NetworkIdentifierWithSubId> const& ids,
    Packet const&                                 packet
) {
    if (detail::shouldDrop(packet)) {
        return;
    }
    origin(ids, packet);
}

using SendToClientByIdentifierFn =
    void (LoopbackPacketSender::*)(NetworkIdentifier const&, Packet const&, SubClientId);

LL_TYPE_INSTANCE_HOOK(
    SendToClientHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    "?$sendToClient@LoopbackPacketSender@@QEAAXAEBVNetworkIdentifier@@AEBVPacket@@W4SubClientId@@@Z"_sym,
    void,
    NetworkIdentifier const& identifier,
    Packet const&            packet,
    SubClientId              recipientSubId
) {
    if (detail::shouldDrop(packet)) {
        return;
    }
    origin(identifier, packet, recipientSubId);
}

using SendToClientByUserFn = void (LoopbackPacketSender::*)(UserEntityIdentifierComponent const*, Packet const&);

LL_TYPE_INSTANCE_HOOK(
    SendToClientByUserHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    "?$sendToClient@LoopbackPacketSender@@QEAAXPEBVUserEntityIdentifierComponent@@AEBVPacket@@@Z"_sym,
    void,
    UserEntityIdentifierComponent const* userIdentifier,
    Packet const&                        packet
) {
    if (detail::shouldDrop(packet)) {
        return;
    }
    origin(userIdentifier, packet);
}

namespace {

bool installedHooks = false;

void unhookAll() noexcept {
    SendBroadcastHook::unhook();
    SendToClientsHook::unhook();
    SendToClientHook::unhook();
    SendToClientByUserHook::unhook();
}

void logSymbol(std::string_view name, std::string_view symbol) noexcept {
    void* const target = ll::memory::SymbolView{symbol}.resolve(true);
    if (target == nullptr) {
        detail::logger()->info("[diag] {} -> NOT FOUND ({})", name, symbol);
        return;
    }
    auto const module = ll::utils::sys_utils::getModuleFileName(ll::utils::sys_utils::getModuleHandle(target));
    detail::logger()->info("[diag] {} -> {} ({})", name, reinterpret_cast<uintptr_t>(target), module);
}

} // namespace

void logHookTargets() noexcept {
    logSymbol("sendBroadcast", "?$sendBroadcast@LoopbackPacketSender@@QEAAXAEBVPacket@@@Z");
    logSymbol(
        "sendToClients",
        "?$sendToClients@LoopbackPacketSender@@QEAAXAEBV?$vector@UNetworkIdentifierWithSubId@@V?$allocator@"
        "UNetworkIdentifierWithSubId@@@std@@@std@@AEBVPacket@@@Z"
    );
    logSymbol("sendToClient(id)", "?$sendToClient@LoopbackPacketSender@@QEAAXAEBVNetworkIdentifier@@AEBVPacket@@W4SubClientId@@@Z");
    logSymbol(
        "sendToClient(user)",
        "?$sendToClient@LoopbackPacketSender@@QEAAXPEBVUserEntityIdentifierComponent@@AEBVPacket@@@Z"
    );
}

bool installHooks() noexcept {
    if (installedHooks) {
        return true;
    }

    bool ok = SendBroadcastHook::hook() >= 0;
    ok      = SendToClientsHook::hook() >= 0 && ok;
    ok      = SendToClientHook::hook() >= 0 && ok;
    ok      = SendToClientByUserHook::hook() >= 0 && ok;

    if (!ok) {
        unhookAll();
        return false;
    }

    installedHooks = true;
    return true;
}

void uninstallHooks() noexcept {
    if (!installedHooks) {
        return;
    }
    unhookAll();
    installedHooks = false;
}

void setHideJoinMessage(bool value) noexcept {
    detail::gHideJoinMessage.store(value, std::memory_order_relaxed);
}

void setHideLeaveMessage(bool value) noexcept {
    detail::gHideLeaveMessage.store(value, std::memory_order_relaxed);
}

void setHideSleepMessage(bool value) noexcept {
    detail::gHideSleepMessage.store(value, std::memory_order_relaxed);
}

void setHideSkipNightMessage(bool value) noexcept {
    detail::gHideSkipNightMessage.store(value, std::memory_order_relaxed);
}

void setHideDeathMessage(bool value) noexcept {
    detail::gHideDeathMessage.store(value, std::memory_order_relaxed);
}

void setDebugLogging(bool value) noexcept { detail::gDebugLogging.store(value, std::memory_order_relaxed); }

} // namespace no_vanilla_messages
