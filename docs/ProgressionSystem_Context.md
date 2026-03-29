# ProgressionSystem — Context Bundle

## Vue d'ensemble

Système de level et d'XP du joueur. Conçu comme MVP extensible vers un écran de choix d'upgrades à chaque level up (vision long terme : Option C — choix entre 3 options, récompenses spéciales aux milestones).

---

## Composants impliqués

| Classe | Rôle |
|---|---|
| `UProgressionComponent` | Owné par `AFlowSlayerCharacter`. Stocke level, XP, fire les delegates. |
| `AFSArenaManager` | Médiateur — écoute `OnEnemyDeath` de chaque ennemi spawné, appelle `AddXP`. |
| `AFSEnemy` | Possède `XPReward` (défaut : 10, `EditDefaultsOnly`). Exposé via `GetXPReward()`. |

---

## Pipeline complet

```
AFSArenaManager::TrySpawnEnemy()
    → spawnedEnemy->OnEnemyDeath.AddUniqueDynamic(this, HandleOnEnemyDeath)

AFSEnemy::HandleOnDeath()
    → OnEnemyDeath.Broadcast(this)

AFSArenaManager::HandleOnEnemyDeath(Enemy)
    → PlayerCharacter->GetProgressionComponent()->AddXP(Enemy->GetXPReward())

UProgressionComponent::AddXP(Amount)
    → CurrentXP += Amount
    → [while XP >= threshold]        ← level up traité EN PREMIER
        → CurrentLevel++
        → OnLevelUp.Broadcast(CurrentLevel)
        → [if milestone] OnMilestoneLevelUp.Broadcast(CurrentLevel)
    → OnXPGained.Broadcast(Amount, CurrentXP)  ← APRÈS le level up
```

---

## Décision d'architecture — Médiateur ArenaManager

`FSEnemy` ne connaît pas le joueur. `FlowSlayerCharacter` ne connaît pas la mort des ennemis.
`FSArenaManager` est le médiateur naturel car il spawne tous les ennemis et bind déjà `OnEnemyDeath`.

**Avantage clé :** fonctionne quelle que soit la cause de la mort (DoT, AoE, environnement) — pas seulement les hits directs du joueur.

---

## UProgressionComponent — Interface publique

```cpp
void AddXP(int32 Amount);

int32 GetCurrentLevel() const;
int32 GetCurrentXP() const;
int32 GetXPToNextLevel() const;
float GetXPRatio() const;        // [0, 1] — pour la barre XP UI
bool  IsMilestoneLevel(int32 Level) const;

TArray<FUpgradeData> DrawUpgrades(int32 Count = 3); // Fisher-Yates shuffle, tier-gated
void  SelectUpgrade(const FUpgradeData& Upgrade);   // Replacement system — annule le tier précédent avant d'appliquer
bool  HasUpgrade(FName UpgradeID) const;

FOnXPGained         OnXPGained;          // (Amount, NewTotal)
FOnLevelUp          OnLevelUp;           // (NewLevel)
FOnMilestoneLevelUp OnMilestoneLevelUp;  // (NewLevel) — levels 5, 10, 15, 20, 25, 30
FOnUpgradeSelected  OnUpgradeSelected;   // (Upgrade) — 5 handlers bindés dans BeginPlay
```

---

## Courbe XP

```cpp
int32 CalculateXPThreshold(int32 Level) const
{
    return 60 + (Level - 1) * 5;
}
```

| Level | XP requis | XP cumulé |
|---|---|---|
| 1 → 2 | 60 | 60 |
| 5 → 6 | 80 | 340 |
| 10 → 11 | 105 | 812 |
| 20 → 21 | 155 | 2162 |
| 29 → 30 | 200 | 3770 |

**Cible :** ~3700 XP total pour un run de 30 levels (~150-200 kills à ~15-20 XP moyen). Tunable via la formule.

---

## Milestones

- Tous les 5 levels : 5, 10, 15, 20, 25, 30
- Configurable via `MilestoneInterval` (`EditDefaultsOnly`, défaut : 5)
- `OnMilestoneLevelUp` est le futur point d'entrée pour récompenses spéciales (weapon upgrade, nouveau skill passif)
- `OnLevelUp` est le futur point d'entrée pour l'écran de choix d'upgrade (3 options)

---

## XP par ennemi

Configuré dans les BPs ennemis (`EditDefaultsOnly` → `Stats`).

| Ennemi | XP |
|---|---|
| Grunt | 10 |
| Runner | 30 |

---

## UI — WBP_PlayerXpBarUi

Widget Blueprint qui affiche la progression XP du joueur.

**Composants :**
- `XPProgress Bar` — barre de progression XP
- `Level Text` — texte affichant le level courant ("Level X")

**Logique (EventGraph) :**
- `Event Construct` : cast du pawn → `Player Ref`, bind `HandleOnXpGained` sur `OnXPGained` et `HandleOnLevelUp` sur `OnLevelUp` via le `ProgressionComponent`. Init de la barre via `GetXPRatio()`.
- `HandleOnXpGained` : appelle `GetXPRatio()` → `Set Percent` sur la barre
- `HandleOnLevelUp` : `New Level` → `To String` → `Append` → `Set Text` sur le level text

**Règle importante :** utiliser `GetXPRatio()` (jamais calculer manuellement) — la valeur est toujours dans [0, 1] grâce à l'ordre de broadcast dans `AddXP`.

---

## Bug corrigé — ordre de broadcast dans AddXP

`OnXPGained` était broadcasté AVANT la boucle de level up → `GetXPRatio()` retournait > 1.0 quand un kill déclenchait un level up → barre XP débordait.

**Fix :** `OnXPGained.Broadcast` déplacé APRÈS la boucle `while` → le ratio est toujours dans [0, 1] quand le BP le lit.

---

## Système d'upgrades

### FUpgradeData (`UpgradeData.h`)

Struct/DataTable row. Champs clés :

| Champ | Type | Rôle |
|---|---|---|
| `UpgradeID` | `FName` | Clé unique — utilisée dans `ActiveUpgradeIDs` |
| `Stat` | `EUpgradeStat` | Stat ciblée (Damage, MaxHealth, FlowDecayRate, DashFlowCost, HealCooldown, HealFlowCost, MoveSpeed) |
| `ValueType` | `EUpgradeValueType` | Additive ou Multiplicative |
| `Value` | `float` | Valeur totale souhaitée au tier (pas un delta) |
| `PrerequisiteUpgradeID` | `FName` | UpgradeID requis pour apparaître dans le pool — `NAME_None` pour T1 |

### Système de tiers (remplacement)

Les upgrades sont organisées en chaînes linéaires via `PrerequisiteUpgradeID` :
```
T1 (prereq: None) → T2 (prereq: T1) → T3 (prereq: T2)
```

**Règle clé : les valeurs sont des totaux, pas des deltas.**
Quand T2 est sélectionné, T1 est annulé puis T2 est appliqué → effet final = valeur de T2 uniquement.

`DrawUpgrades` filtre via `IsUpgradeEligible` : un upgrade n'apparaît que si son prérequis est dans `ActiveUpgradeIDs`.
T1 reste dans `ActiveUpgradeIDs` après remplacement — empêche le re-draw et trace l'historique.

### Pipeline SelectUpgrade (remplacement)

```
SelectUpgrade(T2)
    → UndoPreviousTier(T2)
        → FindRow(T2.PrerequisiteUpgradeID)  ← récupère les données de T1
        → BuildReverseUpgrade(T1)             ← negate additive / invert multiplicative
        → OnUpgradeSelected.Broadcast(reverseT1)  ← tous les handlers défont T1
    → ActiveUpgradeIDs.Add(T2.UpgradeID)
    → OnUpgradeSelected.Broadcast(T2)         ← tous les handlers appliquent T2
```

### Helpers privés

| Fonction | Rôle |
|---|---|
| `IsUpgradeEligible(Upgrade*)` | Filtre actif + tier gate |
| `BuildReverseUpgrade(Upgrade&)` | Pure function — retourne l'upgrade avec valeur inversée |
| `UndoPreviousTier(Upgrade&)` | Orchestre FindRow + BuildReverse + Broadcast |

### Handlers OnUpgradeSelected (bindés dans FlowSlayerCharacter::BeginPlay)

| Handler | Stats gérées |
|---|---|
| `FSCombatComponent::HandleOnUpgradeSelected` | `DamageMultiplier` |
| `DashComponent::HandleOnUpgradeSelected` | `FlowCost` |
| `HealthComponent::HandleOnUpgradeSelected` | `MaxHealth`, `HealCooldown`, `HealFlowCost` |
| `FSFlowComponent::HandleOnUpgradeSelected` | `DecayRate` |
| `FlowSlayerCharacter::HandleOnUpgradeSelected` | `SprintSpeedThreshold`, `RunSpeedThreshold` |

### DT_Upgrades — Chaînes disponibles

| Chaîne | T1 | T2 | T3 |
|---|---|---|---|
| Damage (×) | ×1.2 | ×1.4 | ×1.6 |
| MaxHealth (+) | +25 | +50 | +100 |
| FlowDecayRate (+) | -2/s | -4/s | -7/s |
| DashFlowCost (+) | -3 | -6 | -10 |
| MoveSpeed (+) | +75 | +150 | +250 |
| HealCooldown (+) | -1.5s | -3s | — |
| HealFlowCost (+) | -15 | -30 | — |

---

## État actuel

- [x] Level et XP trackés
- [x] Multi-levelup en un seul gain géré (loop while)
- [x] Delegates `OnXPGained`, `OnLevelUp`, `OnMilestoneLevelUp`
- [x] Pipeline validée (30 kills, level 9, milestone level 5 ✓)
- [x] WBP_PlayerXpBarUi — barre XP + level text, bindé sur les delegates
- [x] Bug ratio > 1.0 corrigé (ordre broadcast)
- [x] `DrawUpgrades` — Fisher-Yates, filtre actifs + tier gate
- [x] `SelectUpgrade` — système de remplacement (UndoPreviousTier + BuildReverseUpgrade)
- [x] 5 handlers `OnUpgradeSelected` — Damage, MaxHealth, FlowDecay, DashCost, MoveSpeed, HealCooldown, HealFlowCost
- [x] `DT_Upgrades.json` — 19 upgrades, 7 chaînes, T3 sur Damage/MaxHealth/FlowDecay/Dash/MoveSpeed
- [x] `WBP_UpgradeScreen` — 3 cartes (icône + nom + description), dark fantasy style, validé en runtime
- [ ] Déclenchement au milestone : pause + affichage WBP_UpgradeScreen
- [ ] Récompenses spéciales aux milestones
