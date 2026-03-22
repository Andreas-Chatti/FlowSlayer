# Enemy System — Context
*Last updated: 2026-03-22*

## Key Files
| File | Role |
|------|------|
| `FSEnemy.h/.cpp` | Abstract base enemy — shared logic for all enemy types |
| `FSEnemy_Grunt.h/.cpp` | Close-range melee variant |
| `FSEnemy_Runner.h/.cpp` | Fast, aggressive variant |
| `FSEnemyAIController.h/.cpp` | BehaviorTree-driven AI controller |
| `HitboxComponent` | Shared with player — sweep hit detection |
| `HitFeedbackComponent` | Knockback, hitstop on hit |
| `HealthComponent` | HP, damage reception, death event |

---

## Class Hierarchy

```
ACharacter
    └─ AFSEnemy  (Abstract, implements IFSDamageable + IFSFocusable)
            ├─ AFSEnemy_Grunt
            └─ AFSEnemy_Runner
```

---

## Interfaces

### IFSDamageable
- `NotifyHitReceived(AActor* instigator, FAttackData)` — called by player's CombatComponent on hit
- `GetHealthComponent()` — required for death check before applying damage
- Implemented by: `AFSEnemy`, `AFlowSlayerCharacter`

### IFSFocusable
- `DisplayLockedOnWidget(bool)` — shows/hides the lock-on indicator widget
- `DisplayHealthBarWidget(bool)` — shows/hides the health bar widget
- `DisplayAllWidgets(bool)` — convenience wrapper
- Implemented by: `AFSEnemy`
- Required by: `FSLockOnComponent` (target must implement this interface)

---

## Components (per enemy)

| Component | Role |
|---|---|
| `HealthComponent` | HP + `OnDeath` delegate + `OnDamageReceived` delegate |
| `HitboxComponent` | Sweep traces during attack animations |
| `HitFeedbackComponent` | Applies knockback impulse + hitstop on hit |
| `LockOnWidget` | UI indicator shown when this enemy is locked-on |

---

## Hit Reception Flow

```
Player CombatComponent::HandleOnHitLanded()
        │
        └─ hitActorDamageable->NotifyHitReceived(player, attackData)
                │
                └─ AFSEnemy::NotifyHitReceived()
                        └─ OnHitReceived.Broadcast(instigator, attackData)
                                │
                                └─ AFSEnemy::HandleOnHitReceived()
                                        ├─ HitFeedbackComponent->OnReceiveHit(location, knockback, upForce)
                                        └─ HealthComponent->ReceiveDamage(damage, instigator)
                                                │
                                                └─ [if HP <= 0] → HandleOnDeath()
```

---

## Death

- `HandleOnDeath()` — called by `HealthComponent::OnDeath`
- Broadcasts `OnEnemyDeath(this)` — listened to by `FSArenaManager` for wave tracking
- Enemy actor is destroyed after `destroyDelay` (5 seconds, const)
- Death animation played in Blueprint (bound to `OnEnemyDeath`)

---

## Attack System

Each enemy has a `FAttackData MainAttack` (set in Blueprint defaults).
- `Attack()` is a `BlueprintNativeEvent` — implemented in Blueprint per variant
- `bIsAttacking` / `bCanAttack` state managed by AI and anim notifies
- `SetIsAttacking(bool)` called by anim notifies to track attack state

---

## AI (FSEnemyAIController)

- BehaviorTree-driven (BT asset assigned in Blueprint)
- Each enemy variant has its own BT with different behavior (Grunt: charge and melee, Runner: fast repositioning)
- AI is responsible for calling `Attack()` when in range (`AttackRange = 150.f` default)

---

## AirStall

When hit by a launcher attack (`KnockbackUpForce > 0`), enemies can be air-stalled:
```cpp
StartAirStall(float airStallDuration)
    → SetMovementMode(MOVE_Flying)       // zeroes gravity
    → Timer expires → SetMovementMode(MOVE_Falling)
```
Also triggered by `AnimNotifyState_AirStall` on the player side.

---

## Variants

### Grunt
- Close-range melee, high HP
- Slower, predictable attack pattern

### Runner
- Fast movement, lower HP
- More aggressive repositioning behavior

---

## Not Yet Implemented

- Ranged enemy type (`FSProjectile` exists but no ranged enemy uses it yet in a complete BT)
- XPReward system (field exists: `int32 XPReward{10}` but no consumer)
- Stun system (previous implementation removed — rework pending)
