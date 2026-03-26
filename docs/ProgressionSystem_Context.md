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
    → OnXPGained.Broadcast(Amount, CurrentXP)
    → [while XP >= threshold]
        → CurrentLevel++
        → OnLevelUp.Broadcast(CurrentLevel)
        → [if milestone] OnMilestoneLevelUp.Broadcast(CurrentLevel)
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

FOnXPGained        OnXPGained;        // (Amount, NewTotal)
FOnLevelUp         OnLevelUp;         // (NewLevel) — futur entry point upgrade screen
FOnMilestoneLevelUp OnMilestoneLevelUp; // (NewLevel) — levels 5, 10, 15, 20, 25, 30
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

| Ennemi | XP (défaut) |
|---|---|
| Grunt | 10 |
| Runner | 10 (à ajuster dans BP_Runner) |

---

## État actuel (MVP)

- [x] Level et XP trackés
- [x] Multi-levelup en un seul gain géré (loop while)
- [x] Delegates `OnXPGained`, `OnLevelUp`, `OnMilestoneLevelUp` prêts pour UI
- [x] Pipeline validée en session (30 kills, level 9 atteint, milestone level 5 déclenché)
- [ ] UI barre XP / level
- [ ] Écran de choix d'upgrade au level up
- [ ] Récompenses spéciales aux milestones
