# Combat System — Context
*Last updated: 2026-03-22*

## Key Files
| File | Role |
|------|------|
| `FSCombatComponent.h/.cpp` | Main combat logic, combo state machine, hit dispatch |
| `CombatData.h` | `EAttackType`, `FAttackData`, `FCombo` data structs |
| `HitboxComponent.h/.cpp` | Sphere/overlap sweep hit detection during active frames |
| `HitFeedbackComponent.h/.cpp` | Knockback, hitstop, camera shake on hit |
| `FSWeapon.h/.cpp` | Weapon actor spawned and attached to `WeaponSocket` |
| `AnimNotifyState_Hitbox` | Enables/disables the hitbox during animation |
| `AnimNotifyState_ComboWindow` | Opens/closes the combo input window |
| `AnimNotifyState_AnimCancelWindow` | Opens dash/jump cancel window mid-attack |

---

## Architecture

```
OnAttackInputReceived(EAttackType)
         │
         ├─ [Idle] ──────────────────────────────────────────────────────────┐
         │   GetComboFromContext()                                            │
         │   → TMap<EAttackType, FCombo*> ComboLookupTable lookup            │
         │   → context check (ground/air, cooldown)                          │
         │   ExecuteAttack(montage)                                           │
         │   bIsAttacking = true                                              │
         │                                                                    ▼
         │              [Anim Montage Playing]
         │                      │
         │         AnimNotifyState_ComboWindow
         │              │               │
         │         [Open]           [Close]
         │    bComboInputWindowOpen = true     bComboInputWindowOpen = false
         │              │                           │
         │    OnComboWindowInputReceived()       bContinueCombo ?
         │    → same combo: bContinueCombo=true      │
         │    → chain combo: bChainingToNewCombo=true ├─ YES → ContinueCombo()
         │                                            │         or ChainingToNextCombo()
         │                                            └─ NO  → ResetComboState()
         │
         └─ [Attacking + window closed] → blocked (input ignored)
```

---

## Data Types

### FAttackData (CombatData.h)
Stored in a `UDataTable` in the editor. Loaded via `AttackDataTable->FindRow<FAttackData>(rowName)`.

| Field | Type | Description |
|---|---|---|
| `Montage` | `UAnimMontage*` | Animation to play |
| `Damage` | `float` | Damage dealt on hit |
| `KnockbackForce` | `float` | Horizontal knockback impulse |
| `KnockbackUpForce` | `float` | Vertical knockback component |
| `FlowReward` | `float` | Flow granted on hit |
| `ComboWindowDuration` | `float` | Duration of the hit-streak timer reset |
| `AttackType` | `EAttackType` | Type identifier |
| `ChainableAttacks` | `TSet<EAttackType>` | Which attack types can follow as a chain |
| `AttackContext` | `EAttackDataContext` | Ground / Air / Any |
| `CooldownDelay` | `float` | Per-attack cooldown |
| `bOnCooldown` | `bool` | Runtime cooldown state |
| `OnAttackExecuted` | `FOnAttackExecuted` | Optional side-effect lambda (e.g., JumpSlam section switch) |

### FCombo (CombatData.h)
```cpp
TArray<FAttackData> Attacks;    // Ordered chain (e.g., StandingLight = 7 attacks)
GetAttackAt(int32 Index)        // Safe index access
GetMaxComboIndex()              // Attacks.Num() - 1
IsValid()                       // Attacks not empty AND Attacks[0].Montage != nullptr
```

### EAttackType (CombatData.h)
```
StandingLight (7), StandingHeavy (4), RunningLight (3), RunningHeavy (3)
AirCombo (3), AerialSlam (1)
DashPierce, DashSpinningSlash, DashDoubleSlash, DashBackSlash       (1 each)
JumpSlam, JumpForwardSlam, JumpUpperSlam                             (1 each)
Launcher, PowerLauncher                                              (1 each)
SpinAttack, HorizontalSweep                                          (1 each)
PowerSlash, PierceThrust, GroundSlam, DiagonalRetourne               (1 each)
```
Numbers in parentheses = attacks in the combo chain.

---

## Combo State Variables

| Variable | Role |
|---|---|
| `bIsAttacking` | True while any montage plays |
| `bComboInputWindowOpen` | True only inside AnimNotifyState_ComboWindow |
| `bContinueCombo` | Input was received during the window — continue on window close |
| `bChainingToNewCombo` | Next combo is a different type (uses PendingCombo) |
| `ComboIndex` | Current position in the FCombo chain |
| `OngoingCombo` | Pointer to the active FCombo |
| `PendingCombo` | Next combo when chaining to a different attack type |
| `bCanAirAttack` | Prevents spamming air attacks — reset on landing |

---

## Hit Flow

```
AnimNotifyState_Hitbox (active frames)
        │
        ▼
HitboxComponent::SweepTrace()
        │  overlap detected
        ▼
OnHitboxHitLanded delegate
        │
        ▼
UFSCombatComponent::HandleOnHitLanded(hitActor, hitLocation)
        │
        ├─ ++ComboHitCount  → OnComboCountChanged.Broadcast()
        ├─ ComboTimeRemaining = ComboWindowDuration  (streak timer reset)
        ├─ OnHitLanded.Broadcast()  → FlowSlayerCharacter → FSFlowComponent::AddFlow()
        ├─ HitFeedBackComponent::OnLandHit()  → hitstop + camera shake
        └─ hitActorDamageable->NotifyHitReceived()  → enemy receives damage
```

---

## Guard System

- Toggle via `ToggleGuard()`
- Blocked if `bIsAttacking` or airborne
- On activation: `RotatePlayerTowardControlRotation()` snaps yaw to camera
- Automatically deactivated when: movement input, dash, jump, or attack starts
- Damage is fully negated in `FlowSlayerCharacter::HandleOnHitReceived` when `IsGuarding() == true`

---

## Combo Hit Counter (Streak UI)

- `ComboHitCount` — increments each hit, displayed in UI
- `bComboCounterActive` — drives `TickComponent` decay countdown
- `ComboTimeRemaining` — decreases each frame, reset per hit via `ComboWindowDuration`
- `OnComboCounterStarted` — fired at 2nd hit (shows UI)
- `OnComboCountChanged(int32)` — fired on each hit
- `OnComboCounterEnded` — fired when timer reaches 0

---

## Important Constraints

- Attack data (montage refs) lives in a `UDataTable` — must be set in editor
- `FAttackData` is a value type (struct) copied into `FCombo::Attacks` — `OnAttackExecuted` lambda is bound after copy
- Jump slam attacks jump to different montage sections depending on ground/air state via `OnAttackExecuted` lambda
- `bCanAirAttack` is set to `false` on AirCombo's last attack, reset on `LandedDelegate`
- `ResetComboState()` sets `bCanAirAttack = false` if still airborne on reset (prevents re-triggering air attacks after a cancel)
