#include "mod/NoVanillaMessages.h"

#include <string_view>

#include "ll/api/Config.h"
#include "ll/api/Versions.h"
#include "ll/api/mod/RegisterHelper.h"

#include "mod/Config.h"
#include "packet/MessageFilter.h"

namespace no_vanilla_messages {

static Config config;

namespace {

bool isLoaderVersionCompatible(ll::io::Logger& logger) {
#if defined(NVM_LL_MAJOR) && defined(NVM_LL_MINOR)
    auto const loader = ll::getLoaderVersion();
    if (static_cast<int>(loader.major) == NVM_LL_MAJOR && static_cast<int>(loader.minor) == NVM_LL_MINOR) {
        return true;
    }

    logger.error(
        "Built for LeviLamina {}.{} but the server runs {}.{}.{}: the hooks would patch wrong addresses, refusing to "
        "enable. Set LL_VERSION = \"{}.{}.*\" in xmake.lua and rebuild",
        NVM_LL_MAJOR,
        NVM_LL_MINOR,
        loader.major,
        loader.minor,
        loader.patch,
        NVM_LL_MAJOR,
        NVM_LL_MINOR
    );
    return false;
#else
    (void)logger;
    return true;
#endif
}

} // namespace

NoVanillaMessages& NoVanillaMessages::getInstance() {
    static NoVanillaMessages instance;
    return instance;
}

bool NoVanillaMessages::load() {
    auto& logger = getSelf().getLogger();

    logger.debug("Loading...");

    auto const& configFilePath = getSelf().getConfigDir() / "config.json";
    if (!ll::config::loadConfig(config, configFilePath)) {
        logger.warn("Cannot load configurations from {}", configFilePath);
        logger.info("Saving default configurations");
        if (!ll::config::saveConfig(config, configFilePath)) {
            logger.error("Cannot save default configurations to {}", configFilePath);
        }
    }

    return true;
}

bool NoVanillaMessages::enable() {
    auto& logger = getSelf().getLogger();

    logger.debug("Enabling...");

    if (!isLoaderVersionCompatible(logger)) {
        return false;
    }

    setHideJoinMessage(config.hideJoinMessage);
    setHideLeaveMessage(config.hideLeaveMessage);
    setHideSleepMessage(config.hideSleepMessage);
    setHideSkipNightMessage(config.hideSkipNightMessage);
    setHideDeathMessage(config.hideDeathMessage);
    setDebugLogging(config.debug);

    if (!installHooks()) {
        logger.error("Cannot install the packet filter, messages stay visible");
        return false;
    }

    if (config.debug) {
        logHookTargets();
        logger.info(
            "Vanilla messages hidden (join: {}, leave: {}, sleep: {}, skip night: {}, death: {})",
            config.hideJoinMessage,
            config.hideLeaveMessage,
            config.hideSleepMessage,
            config.hideSkipNightMessage,
            config.hideDeathMessage
        );
    }

    return true;
}

bool NoVanillaMessages::disable() {
    getSelf().getLogger().debug("Disabling...");

    uninstallHooks();

    return true;
}

bool NoVanillaMessages::unload() {
    getSelf().getLogger().debug("Unloading...");

    uninstallHooks();

    return true;
}

} // namespace no_vanilla_messages

LL_REGISTER_MOD(
    no_vanilla_messages::NoVanillaMessages,
    no_vanilla_messages::NoVanillaMessages::getInstance()
);
