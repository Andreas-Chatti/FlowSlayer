# Flow / Momentum System — Context
*Last updated: 2026-03-22*

## Key Files
| File | Role |
|------|------|
| `FSFlowComponent.h/.cpp` | All flow logic: gain, decay, tiers, immunity |

---

## What Is Flow?

Flow is the central gameplay resource. It rewards aggressive play and punishes passivity or taking hits. Higher tiers directly boost player power.

---

## Tier System

```
CurrentFlow / MaxFlow ratio → EFlowTier

  0% - 24%  → None    (no bonus)
 25% - 49%  → Low     (movement speed boost — not yet implemented)
 50% - 74%  → Medium  (attack speed boost — not yet implemented)
 75% - 99%  → High    (damage boost — not yet implemented)
100%        → Max     (hit immunity window)
```

Tier transitions broadcast `FlowTierChanged(newTier, oldTier)` — listened to by UI and any gameplay system that cares about tier changes.

---

## Flow Gain / Loss Events

| Event | Effect | Source |
|---|---|---|
| Attack lands on enemy | `+FlowReward` (per `FAttackData`) | `HandleOnHitLanded` |
| Dash | `-flowCost` | `DashComponent::OnDashStarted → FlowComponent::RemoveFlow` |
| Hit received (non-Max tier) | `-damage / 2` | `HealthComponent::OnDamageReceived → OnPlayerHit` |
| Hit received (Max tier) | No immediate flow loss | Immunity window active |
| Passive decay | `-DecayRate * DeltaTime` per second | `TickComponent` |

---

## Passive Decay

```
Player lands a hit
        │
        └─ AddFlow() called
                │
                ├─ Stop current decay
                ├─ Clear DecayGraceTimer
                └─ Start DecayGraceTimer (5 sec)
                        │
                        └─ On expiry: bIsDecaying = true
                                │
                                └─ TickComponent: RemoveFlow(DecayRate * DeltaTime)
```

Flow decays at `8 units/sec` after `5 seconds` of no hits landed.

---

## Max Tier Immunity

```
Player hits Max tier (100% flow)
        │
        └─ OnFlowTierChanged → CurrentTier = Max
                │
                ├─ Stop decay (DecayGraceTimer cleared, bIsDecaying = false)
                └─ Start ImmunityTimer (5 sec)
                        │
                        ├─ Player takes a hit during window → OnPlayerHit()
                        │       └─ ImmunityTimer active → RemoveFlow() returns early (no loss)
                        │
                        ├─ Player lands a hit during window → AddFlow()
                        │       └─ ImmunityTimer still active → grace period reset deferred
                        │
                        └─ ImmunityTimer expires
                                └─ Start DecayGraceTimer (5 sec) → then decay resumes
```

`RemoveFlow()` is guarded: `if (ImmunityTimer active || bInfiniteFlow) return;`

---

## Key Values

| Constant | Value | Description |
|---|---|---|
| `MaxFlow` | 100 | Flow cap |
| `DecayRate` | 8 /sec | Passive drain rate |
| `DecayGracePeriod` | 5 sec | Idle time before decay starts |
| `ImmunityDuration` | 5 sec | Hit immunity window at Max tier |

---

## Public API

```cpp
void AddFlow(float amount);             // On hit landed
void RemoveFlow(float amount);          // On dash, decay tick
void ConsumeFlow(float amount);         // Voluntary spend (special attacks — future)
bool HasEnoughFlow(float cost) const;   // Used by DashComponent::CanAffordDash

float GetFlowRatio() const;             // 0.0 - 1.0 for UI bar
EFlowTier GetFlowTier() const;          // Current tier
```

---

## Delegate Wiring (set up in FlowSlayerCharacter constructor)

```cpp
// Flow gain on hit
CombatComponent->OnHitLanded → FlowSlayerCharacter::HandleOnHitLanded
                             → FlowComponent->HandleOnHitLanded(hitActor, hitLocation, damage, flowReward)

// Flow loss on dash
DashComponent->OnDashStarted → FlowComponent->RemoveFlow(flowCost)

// Flow loss on player hit
HealthComponent->OnDamageReceived → FlowComponent->OnPlayerHit(instigator, damage, hp, maxHp)

// Dash affordability check
DashComponent->CanAffordDash.BindUObject(FlowComponent, &UFSFlowComponent::HasEnoughFlow)
```

---

## Debug

`bInfiniteFlow` (EditAnywhere, Category="Debug") — sets flow to max at BeginPlay and bypasses all RemoveFlow calls. Useful for testing without flow constraints.

---

## Not Yet Implemented

- Tier gameplay bonuses (speed, attack speed, damage multipliers) — tiers exist and broadcast but no gameplay systems consume them yet
- `ConsumeFlow()` for special attacks (designed but no consumers)
