# mod-talent-points-quests

An [AzerothCore](https://www.azerothcore.org) module: players earn talent points by completing quests instead of
by levelling.

## How it works

Eligible quests reward talent points:

| Quests | Points |
|---|---|
| Class quests, group quests, dungeon and heroic dungeon quests | 1 |
| Raid quests | 2 |

Only non-repeatable quests count: no daily, weekly or repeatable quest rewards a point.

Points are split in one tier per expansion:

| Tier | Character levels | Max points | Quests that count |
|---|---|---|---|
| Classic | 10–60 | 51 | any eligible quest |
| The Burning Crusade | 61–70 | 10 | eligible Outland quests |
| Wrath of the Lich King | 71–80 | 10 | eligible Northrend quests |

- **Quest expansion.** A quest belongs to the expansion of its zone, and its level must be at least 58 for The
  Burning Crusade or 68 for Wrath of the Lich King. Class quests have no zone, so only their level counts. The Blood Elf
  and Draenei starting zones are on the Outland map but stay in the Classic tier because of their level.
- **Highest tier first.** A quest point goes to the highest tier the quest belongs to. When that tier is full, the
  point spills into the tier below. A level 80 character who only did Outland quests can reach 51 + 10 = 61 points.
- **Levelling cap.** In each tier, a player can be up to 5 points ahead of the standard level-based amount, and never
  above the tier maximum. A level 30 character can have up to 26 points (21 + 5).
- **Server level cap.** A tier above `MaxPlayerLevel` stays empty. With `MaxPlayerLevel = 60`, only the Classic tier
  applies: at most 51 points.

Bonus talent points granted by the core (`Player::AddBonusTalent`) and `Rate.Talent` still apply on top.

## Install

1. Clone the module into the `modules/` folder of your AzerothCore source:

   ```sh
   cd modules
   git clone https://github.com/Miorey/mod-talent-points-quests.git
   ```

2. Re-run CMake and rebuild the worldserver.
3. Copy `mod_talent_points_quests.conf.dist` to `mod_talent_points_quests.conf` in your modules config folder and
   adjust it if needed.
4. Start the worldserver. The database updater applies `data/sql/db-world/talent_points_quests.sql`, which sets
   `quest_template.RewardTalents` on the eligible quests.

Characters are recalculated at login: anyone who has spent more points than the rule allows gets a talent reset.

## Configuration

| Option | Default | Description |
|---|---|---|
| `TalentPointsQuests.Enable` | `1` | Enable the quest-based talent points. |
| `TalentPointsQuests.PointsAhead` | `5` | Points a player can have in a tier above the standard level-based amount. |
| `TalentPointsQuests.Classic.LastLevel` | `60` | Last level of the Classic tier, which starts at level 10. |
| `TalentPointsQuests.TBC.LastLevel` | `70` | Last level of The Burning Crusade tier. |
| `TalentPointsQuests.WotLK.LastLevel` | `80` | Last level of the Wrath of the Lich King tier. |
| `TalentPointsQuests.TBC.QuestMinLevel` | `58` | Minimum quest level for The Burning Crusade tier. |
| `TalentPointsQuests.WotLK.QuestMinLevel` | `68` | Minimum quest level for the Wrath of the Lich King tier. |

Each tier starts right after the previous one and holds one point per level.

## Uninstall

Set `TalentPointsQuests.Enable = 0`, or remove the module and rebuild. The standard level-based talent points apply
again.

The `RewardTalents` values set by the SQL stay in the database. The core ignores them for every class except Death
Knights in Ebon Hold, but the quest reward window still shows the talent point. To remove them, restore
`quest_template` from your world database source.

## Credits

- [Jackhein](https://github.com/Jackhein): original rule, written for the
  [Murloc Village](https://wotlk.murlocvillage.com) server
- [Miorey](https://github.com/Miorey): module rewrite and expansion tiers

## License

[MIT](LICENSE)
