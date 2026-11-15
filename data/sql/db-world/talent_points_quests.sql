-- mod-talent-points-quests: quests that reward talent points.
-- Only non-repeatable, non-daily, non-weekly, enabled quests that do not reward talents yet.

-- 1 point: class quests, group quests, dungeon and heroic dungeon quests
UPDATE `quest_template` AS `qt`
LEFT JOIN `quest_template_addon` AS `qta` ON `qta`.`ID` = `qt`.`ID`
SET `qt`.`RewardTalents` = 1
WHERE `qt`.`RewardTalents` = 0
AND `qt`.`QuestType` != 1
AND (`qt`.`Flags` & 0x9000) = 0
AND (COALESCE(`qta`.`SpecialFlags`, 0) & 1) = 0
AND (`qt`.`QuestSortID` IN (-61, -81, -82, -141, -161, -162, -261, -262, -263, -372)
OR `qt`.`QuestInfoID` IN (1, 81, 85));

-- 2 points: raid quests
UPDATE `quest_template` AS `qt`
LEFT JOIN `quest_template_addon` AS `qta` ON `qta`.`ID` = `qt`.`ID`
SET `qt`.`RewardTalents` = 2
WHERE `qt`.`RewardTalents` = 0
AND `qt`.`QuestType` != 1
AND (`qt`.`Flags` & 0x9000) = 0
AND (COALESCE(`qta`.`SpecialFlags`, 0) & 1) = 0
AND `qt`.`QuestInfoID` IN (62, 88, 89);
