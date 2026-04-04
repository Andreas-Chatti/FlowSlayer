# Weapon Part System — Context Bundle

## Vue d'ensemble

Système de progression d'arme par collection de pièces. Pas de crafting UI — chaque pièce trouvée s'équipe automatiquement ou monte le tier si le slot est déjà occupé. 3 slots (Blade, Handle, Gem), chacun avec 3 tiers (T1→T2→T3).

---

## Fichiers clés

| Fichier | Rôle |
|---|---|
| `Public/WeaponPartData.h` | Tous les types : `EWeaponPartType`, `FWeaponPartData`, `ERewardType`, `FRewardCard` |
| `Public/FSWeapon.h/.cpp` | Propriétaire de l'état des pièces et de `WeaponDamageMultiplier` |
| `Public/ProgressionComponent.h/.cpp` | Pool de récompenses mixte, drop logic, delegate |
| `Private/FSCombatComponent.cpp` | Lit `GetDamageMultiplier()` dans `HandleOnHitLanded()` |
| `Public/FSArenaManager.h/.cpp` | Mécanique d'arène + broadcast `OnEnemySpawned` |
| `FlowSlayerGameMode.h/.cpp` | Bind `OnEnemySpawned` → bind `OnEnemyDeath` → XP + drop |
| `FlowSlayerCharacter.h/.cpp` | Pont `OnWeaponPartSelected` → `weapon->EquipPart()` |

---

## Data

### FWeaponPartData (FTableRowBase)
Stocké dans `DT_WeaponParts`. Champs clés :
- `PartID` — clé unique (ex: `Blade_T1`)
- `PartType` — `EWeaponPartType` (Blade/Handle/Gem)
- `Tier` — 1, 2 ou 3
- `Stat` — réutilise `EUpgradeStat` (ex: `Damage`) ; `None` pour Gem jusqu'à définition
- `ValueType` + `Value` — même sémantique que `FUpgradeData`
- `PrerequisitePartID` — `None` pour T1, `Blade_T1` pour T2, etc.

### FRewardCard (USTRUCT BlueprintType)
Union retournée par `DrawMixedRewards()` :
- `RewardType` (`ERewardType::Upgrade` ou `ERewardType::WeaponPart`)
- `UpgradeData` — valide si Upgrade
- `WeaponPartData` — valide si WeaponPart

---

## Pipelines

### Drop ennemi (automatique)
```
AFSArenaManager spawne un ennemi → OnEnemySpawned.Broadcast(Enemy)
    → GameMode::HandleOnEnemySpawned(Enemy)
        → Enemy->OnEnemyDeath.AddUniqueDynamic(HandleOnEnemyDeath)

Enemy meurt → OnEnemyDeath.Broadcast(this)
    → GameMode::HandleOnEnemyDeath(Enemy)
        → ProgressionComponent::AddXP(Enemy->GetXPReward())
        → FMath::FRand() < Enemy->GetWeaponPartDropChance()
            → ProgressionComponent::ApplyRandomWeaponPartDrop()
                → FindNextPartForSlot() — cherche le tier suivant dans DT_WeaponParts
                → AFSWeapon::EquipPart() — undo tier précédent + apply nouveau

AFSArenaManager::HandleOnEnemyDeath() — mécanique arène uniquement
    (AliveEnemyCount--, CheckCapEscalation, CheckArenaCompletion, ScheduleNextSpawn)
```

### Reward screen (choix joueur)
```
GameMode → WBP_UpgradeScreen
  → ProgressionComponent::DrawMixedRewards(3)
      → pool upgrades éligibles + parts éligibles (Tier == currentTier + 1)
      → shuffle → TArray<FRewardCard>[3]
  → Joueur confirme
      si Upgrade   → SelectUpgrade() → OnUpgradeSelected.Broadcast()
      si WeaponPart → SelectWeaponPart() → OnWeaponPartSelected.Broadcast()
          → FlowSlayerCharacter::HandleOnWeaponPartSelected()
          → AFSWeapon::EquipPart()
```

### Impact en combat
```
FSCombatComponent::HandleOnHitLanded()
  scaledAttack.Damage *= DamageMultiplier          // upgrade multiplier
  scaledAttack.Damage *= equippedWeapon->GetDamageMultiplier()  // part multiplier
```

---

## AFSWeapon — état interne

```cpp
TMap<EWeaponPartType, int32> EquippedPartTiers      // 0 = vide, 1-3 = tier
TMap<EWeaponPartType, FWeaponPartData> EquippedPartDataCache  // pour le undo
float WeaponDamageMultiplier{ 1.f }
```

`EquipPart()` :
1. Si slot occupé → `BuildReversePart()` + `ApplyPartStat(reverse)` → undo
2. `ApplyPartStat(newPart)` → apply
3. Met à jour TierMap et cache

`ApplyPartStat()` switch sur `PartData.Stat` :
- `EUpgradeStat::Damage` → modifie `WeaponDamageMultiplier`
- `default: break` → extension point pour Handle/Gem

---

## Points d'extension

- **Handle stats** : ajouter un `WeaponAttackCooldownMultiplier` dans `AFSWeapon`, case dans `ApplyPartStat`, getter lu par `FSCombatComponent`
- **Gem stats** : étendre `EUpgradeStat` ou créer un enum dédié, ajouter case dans `ApplyPartStat`
- **Notification HUD sur drop** : ajouter `OnWeaponPartDropped` delegate dans `ProgressionComponent`, broadcaster depuis `ApplyRandomWeaponPartDrop()`
- **Nouvelle source de récompense** : étendre `ERewardType` + `FRewardCard`, étendre `DrawMixedRewards()`

---

## Setup éditeur requis

1. Créer `DT_WeaponParts` (row struct `FWeaponPartData`) — 9 rows : Blade/Handle/Gem × T1/T2/T3
2. Assigner sur `BP_FlowSlayerCharacter` → `ProgressionComponent` → `WeaponPartDataTable`
3. `WBP_UpgradeScreen` / `BP_ThirdPersonGameMode` :
   - Remplacer `DrawUpgrades()` → `DrawMixedRewards()`
   - Branch sur `RewardType` pour l'affichage
   - Sur confirmation : si WeaponPart → `SelectWeaponPart(card.WeaponPartData)`
