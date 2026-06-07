#pragma once
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/player/PlayerUseItemEvent.h"
#include "ll/api/event/player/PlayerChatEvent.h"
#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/event/entity/MobDieEvent.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/item/ItemStack.h"
#include "mc/math/Vec3.h"
#include "mc/world/phys/AABB.h"
#include <memory>
#include <string>
#include <atomic>
#include <unordered_set>

namespace bow_nether {

static std::atomic<bool> gGodMode{false};

// Set berisi unique ID entity yang sedang diprotect
static std::unordered_set<uint64_t> gProtectedIds;

static constexpr float NETHER_X   =  0.0f;
static constexpr float NETHER_Y   = 64.0f;
static constexpr float NETHER_Z   =  0.0f;
static constexpr int   NETHER_DIM = 1;

class BowNetherPlugin {

    std::shared_ptr<ll::event::Listener<ll::event::PlayerUseItemEvent>> mUseItemListener;
    std::shared_ptr<ll::event::Listener<ll::event::PlayerChatEvent>>    mChatListener;
    std::shared_ptr<ll::event::Listener<ll::event::ActorHurtEvent>>     mHurtListener;
    std::shared_ptr<ll::event::Listener<ll::event::MobDieEvent>>        mDieListener;

    // Cek apakah actor ini diprotect
    static bool isProtected(Actor& actor) {
        if (!gGodMode.load()) return false;
        if (actor.isPlayer()) return false;
        return true;
    }

    void onUseItem(ll::event::PlayerUseItemEvent& ev) {
        if (ev.item().getTypeName() != "minecraft:bow") return;
        auto& player = ev.self();
        auto& level  = player.getLevel();

        // /reload
        level.runCommand(
            "reload",
            player.getDimensionConst(),
            CommandPermissionLevel::GameDirectors
        );

        // Tp semua entity bukan player ke nether
        Vec3 dest{NETHER_X, NETHER_Y, NETHER_Z};
        AutomaticID<Dimension, int> netherId{NETHER_DIM};
        level.forEachDimension([&](Dimension& dim) -> bool {
            dim.forEachActor([&](Actor& actor) {
                if (!actor.isPlayer()) actor.teleport(dest, netherId);
            });
            return true;
        });
    }

    void onChat(ll::event::PlayerChatEvent& ev) {
        if (ev.message() != "u_def") return;
        bool newState = !gGodMode.load();
        gGodMode.store(newState);
        ev.cancel();

        auto& level = ev.self().getLevel();

        if (newState) {
            // Daftarkan semua entity non-player ke protected set
            gProtectedIds.clear();
            level.forEachDimension([&](Dimension& dim) -> bool {
                dim.forEachActor([&](Actor& actor) {
                    if (!actor.isPlayer()) {
                        // Set HP max
                        auto* health = actor.getAttribute(SharedAttributes::HEALTH);
                        if (health) health->setCurrentValue(health->getMaxValue());

                        // Set invulnerable flag
                        actor.setInvulnerable(true);

                        // Simpan ID
                        gProtectedIds.insert(actor.getRuntimeID().rawID);
                    }
                });
                return true;
            });
            ev.self().sendMessage("§aEntity GodMode: ON — semua entity kebal total");
        } else {
            // Lepas proteksi
            level.forEachDimension([&](Dimension& dim) -> bool {
                dim.forEachActor([&](Actor& actor) {
                    if (!actor.isPlayer()) {
                        actor.setInvulnerable(false);
                    }
                });
                return true;
            });
            gProtectedIds.clear();
            ev.self().sendMessage("§cEntity GodMode: OFF");
        }
    }

    void onHurt(ll::event::ActorHurtEvent& ev) {
        if (!isProtected(ev.self())) return;
        // Nol-kan damage dan cancel event
        ev.damage() = 0.0f;
        ev.cancel();
        // Restore HP setiap kena damage supaya tidak turun sama sekali
        auto* health = ev.self().getAttribute(SharedAttributes::HEALTH);
        if (health) health->setCurrentValue(health->getMaxValue());
    }

    void onDie(ll::event::MobDieEvent& ev) {
        if (!isProtected(ev.self())) return;
        // Paksa restore HP — cegah kematian
        auto& mob    = ev.self();
        auto* health = mob.getAttribute(SharedAttributes::HEALTH);
        if (health) health->setCurrentValue(health->getMaxValue());
        // Pastikan invulnerable tetap aktif
        mob.setInvulnerable(true);
    }

public:
    bool load() { return true; }

    bool enable() {
        auto& bus = ll::event::EventBus::getInstance();

        mUseItemListener = bus.emplaceListener<ll::event::PlayerUseItemEvent>(
            [this](ll::event::PlayerUseItemEvent& ev) { onUseItem(ev); },
            ll::event::EventPriority::Normal
        );
        mChatListener = bus.emplaceListener<ll::event::PlayerChatEvent>(
            [this](ll::event::PlayerChatEvent& ev) { onChat(ev); },
            ll::event::EventPriority::Normal
        );
        // Priority::Monitor = jalan PALING AKHIR, setelah semua handler lain
        // Ini penting supaya cancel-nya benar-benar final
        mHurtListener = bus.emplaceListener<ll::event::ActorHurtEvent>(
            [this](ll::event::ActorHurtEvent& ev) { onHurt(ev); },
            ll::event::EventPriority::Monitor
        );
        mDieListener = bus.emplaceListener<ll::event::MobDieEvent>(
            [this](ll::event::MobDieEvent& ev) { onDie(ev); },
            ll::event::EventPriority::Monitor
        );
        return true;
    }

    bool disable() {
        auto& bus = ll::event::EventBus::getInstance();
        bus.removeListener(mUseItemListener);
        bus.removeListener(mChatListener);
        bus.removeListener(mHurtListener);
        bus.removeListener(mDieListener);
        gGodMode.store(false);
        gProtectedIds.clear();
        return true;
    }
};

static std::unique_ptr<BowNetherPlugin> gPlugin;

} // namespace bow_nether

LL_REGISTER_MOD(bow_nether::BowNetherPlugin, bow_nether::gPlugin);
