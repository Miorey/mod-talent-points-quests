/*
 * This file is part of mod-talent-points-quests, an AzerothCore module.
 * Original rule by Jackhein <https://github.com/Jackhein>.
 */

#include "Config.h"
#include "DBCStores.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "World.h"
#include <algorithm>
#include <array>

namespace
{
    // One tier per expansion: 0 = Classic, 1 = The Burning Crusade, 2 = Wrath of the Lich King
    constexpr std::size_t TIER_COUNT = 3;

    struct Tier
    {
        uint32 FirstLevel;    // first character level that earns a point in this tier
        uint32 LastLevel;     // last character level that earns a point in this tier
        uint32 QuestMinLevel; // quests below this level never count for this tier
    };

    struct TalentPointsQuestsConfig
    {
        bool Enabled = true;
        uint32 PointsAhead = 5;
        std::array<Tier, TIER_COUNT> Tiers = { {
            { 10, 60, 0 },
            { 61, 70, 58 },
            { 71, 80, 68 }
        } };
    };

    TalentPointsQuestsConfig sTalentConfig;

    // Points a player of this level earns in the tier with the standard level-based rule
    uint32 GetStandardPoints(Tier const& tier, uint32 level)
    {
        if (level < tier.FirstLevel)
            return 0;

        return std::min(level, tier.LastLevel) - tier.FirstLevel + 1;
    }

    // Most points the tier can hold for a player of this level
    uint32 GetTierCap(Tier const& tier, uint32 level)
    {
        if (level < tier.FirstLevel)
            return 0;

        uint32 const maxPlayerLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
        return std::min(GetStandardPoints(tier, level) + sTalentConfig.PointsAhead,
            GetStandardPoints(tier, maxPlayerLevel));
    }

    // Highest tier a quest can count for: the lower of its zone expansion and its level
    std::size_t GetQuestTier(Quest const* quest)
    {
        std::size_t zoneTier = TIER_COUNT - 1;
        if (quest->GetZoneOrSort() > 0)
            if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(quest->GetZoneOrSort()))
                if (MapEntry const* map = sMapStore.LookupEntry(area->mapid))
                    zoneTier = std::min<std::size_t>(map->Expansion(), TIER_COUNT - 1);

        int32 const questLevel = quest->GetQuestLevel() > 0 ? quest->GetQuestLevel() : quest->GetMinLevel();
        std::size_t levelTier = 0;
        for (std::size_t i = 1; i < TIER_COUNT; ++i)
            if (questLevel >= int32(sTalentConfig.Tiers[i].QuestMinLevel))
                levelTier = i;

        return std::min(zoneTier, levelTier);
    }

    uint32 CalculateQuestTalentPoints(Player const* player)
    {
        std::array<uint32, TIER_COUNT> questPoints = { };
        for (uint32 questId : player->getRewardedQuests())
            if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
                if (quest->GetBonusTalents())
                    questPoints[GetQuestTier(quest)] += quest->GetBonusTalents();

        // Fill the highest tier first, what does not fit spills into the tier below
        uint32 const level = player->GetLevel();
        uint32 total = 0;
        uint32 spill = 0;
        for (std::size_t i = TIER_COUNT; i-- > 0;)
        {
            uint32 const available = questPoints[i] + spill;
            uint32 const granted = std::min(available, GetTierCap(sTalentConfig.Tiers[i], level));
            total += granted;
            spill = available - granted;
        }

        return total;
    }
}

class TalentPointsQuestsPlayerScript : public PlayerScript
{
public:
    TalentPointsQuestsPlayerScript() : PlayerScript("TalentPointsQuestsPlayerScript", {
        PLAYERHOOK_ON_CALCULATE_TALENTS_POINTS,
        PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST
    }) { }

    void OnPlayerCalculateTalentsPoints(Player const* player, uint32& talentPointsForLevel) override
    {
        if (!sTalentConfig.Enabled)
            return;

        // GetBonusTalentCount() only reads, it is just missing its const qualifier
        talentPointsForLevel = CalculateQuestTalentPoints(player) + const_cast<Player*>(player)->GetBonusTalentCount();
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        // The core recalculates talents before it marks the quest as rewarded, so do it again now
        if (sTalentConfig.Enabled && quest->GetBonusTalents())
            player->InitTalentForLevel();
    }
};

class TalentPointsQuestsWorldScript : public WorldScript
{
public:
    TalentPointsQuestsWorldScript() : WorldScript("TalentPointsQuestsWorldScript", {
        WORLDHOOK_ON_AFTER_CONFIG_LOAD
    }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        TalentPointsQuestsConfig config;
        config.Enabled = sConfigMgr->GetOption<bool>("TalentPointsQuests.Enable", true);
        config.PointsAhead = sConfigMgr->GetOption<uint32>("TalentPointsQuests.PointsAhead", 5);

        config.Tiers[0].LastLevel = sConfigMgr->GetOption<uint32>("TalentPointsQuests.Classic.LastLevel", 60);
        config.Tiers[1].LastLevel = sConfigMgr->GetOption<uint32>("TalentPointsQuests.TBC.LastLevel", 70);
        config.Tiers[2].LastLevel = sConfigMgr->GetOption<uint32>("TalentPointsQuests.WotLK.LastLevel", 80);
        config.Tiers[1].QuestMinLevel = sConfigMgr->GetOption<uint32>("TalentPointsQuests.TBC.QuestMinLevel", 58);
        config.Tiers[2].QuestMinLevel = sConfigMgr->GetOption<uint32>("TalentPointsQuests.WotLK.QuestMinLevel", 68);

        for (std::size_t i = 0; i < TIER_COUNT; ++i)
        {
            if (i > 0)
                config.Tiers[i].FirstLevel = config.Tiers[i - 1].LastLevel + 1;

            if (config.Tiers[i].LastLevel < config.Tiers[i].FirstLevel)
            {
                LOG_ERROR("module", "mod-talent-points-quests: tier last levels must increase from level 10, "
                    "using the defaults.");
                config.Tiers = TalentPointsQuestsConfig().Tiers;
                break;
            }
        }

        sTalentConfig = config;
    }
};

void AddSC_talent_points_quests()
{
    new TalentPointsQuestsPlayerScript();
    new TalentPointsQuestsWorldScript();
}
