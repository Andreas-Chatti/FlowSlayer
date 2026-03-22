# AnimNotify System — Context
*Last updated: 2026-03-22*

All custom notifies and notify states are in `Source/FlowSlayer/Public/` and `Private/`.
They are placed directly on animation montage timelines in the editor.

---

## Quick Reference

| Notify State | Display Name | Role |
|---|---|---|
| `AnimNotifyState_Hitbox` | "Weapon active frame" | Enables hit detection |
| `AnimNotifyState_ComboWindow` | "Combo Input Window" | Opens combo input buffer |
| `AnimNotifyState_AnimCancelWindow` | "Animation Cancel Window" | Allows dash/move to cancel attack |
| `AnimNotifyState_FSMotionWarping` | "FSMotionWarping" | Warps player toward target |
| `AnimNotifyState_MovementSpeed` | "MaxWalkSpeedModifier" | Overrides MaxWalkSpeed |
| `AnimNotifyState_AirStall` | "Air stall window" | Zeroes gravity during air attack |
| `AnimNotifyState_RotateToTarget` | "RotateToTarget" | Snaps/interpolates yaw toward target |
| `AnimNotifyState_WeaponTrail` | "WeaponTrail" | Spawns weapon trail VFX |
| `AnimNotify_Launch` | — | Fires launch impulse on hit target |

---

## AnimNotifyState_Hitbox — "Weapon active frame"

**Purpose:** Activates the hitbox sweep during the attack's active frames.

```
NotifyBegin → HitboxComponent::StartActiveFrame(HitboxProfile)
NotifyTick  → HitboxComponent::TickActiveFrame() — runs sweep traces each frame
NotifyEnd   → HitboxComponent::StopActiveFrame()
             → OnActiveFrameStopped delegate fires (clears already-hit actors set)
```

**HitboxProfile** — configured per-notify in the montage editor:
- Shape (sphere / capsule / box), size, socket to trace from, etc.

**Used on:** Player attacks and enemy attacks (same component, shared by both).

**Key detail:** Each `AnimNotifyState_Hitbox` instance caches `HitboxComp` from the owning actor at NotifyBegin. Enemies get their hitbox component, the player gets theirs.

---

## AnimNotifyState_ComboWindow — "Combo Input Window"

**Purpose:** Defines the window where the player can input the next attack to continue the combo.

```
NotifyBegin → CombatComponent->OnComboInputWindowOpened.Execute()
              → bComboInputWindowOpen = true
NotifyTick  → Checks bComboInputWindowOpen — if already closed early (chain instant),
              skips further processing (bHasClosedEarly = true)
NotifyEnd   → CombatComponent->OnComboInputWindowClosed.Execute()
              → bComboInputWindowOpen = false
              → if bContinueCombo → ContinueCombo() or ChainingToNextCombo()
              → if not → ResetComboState()
```

**Early close:** When the player chains instantly to a different combo type, `FSCombatComponent::OnComboWindowInputReceived` sets `bComboInputWindowOpen = false` directly. The notify state detects this via `bHasClosedEarly` to avoid double-firing `NotifyEnd`.

**Placement rule:** Start = when the player should be allowed to input. End = last moment to catch the input before the attack finishes.

---

## AnimNotifyState_AnimCancelWindow — "Animation Cancel Window"

**Purpose:** Defines a window where the player can cancel the current attack via dash or movement.

**Cancel action types** (`EAnimCancelWindowActionType`):
- `Dash` — cancels only if LShift is held AND movement input is active
- `Move` — cancels only if movement input is active (no LShift required)
- `Any` — cancels on LShift OR movement input

```
NotifyBegin → caches FSCharacter reference
NotifyTick  → checks input state each frame:
              bDashCancel = LShift held + HasMovementInput + type is Dash
              bMoveCancel = HasMovementInput + type is Move
              bAnyCancel  = (LShift OR HasMovementInput) + type is Any
              → if any true and not already triggered:
                  bAnimCancelTrigger = true
                  FSCharacter->OnAnimationCanceled.Broadcast()
                      → FlowSlayerCharacter::HandleOnAnimationCanceled()
                          → CombatComponent->CancelAttack()
NotifyEnd   → bAnimCancelTrigger = false (reset for next window)
```

**Key detail:** Uses `GetInputKeyState(EKeys::LeftShift)` (raw key state) and `HasMovementInput()` (cached axis from `InputManagerComponent`). `bAnimCancelTrigger` prevents the cancel from firing more than once per window.

---

## AnimNotifyState_FSMotionWarping — "FSMotionWarping"

**Purpose:** Automatically resolves the warp target and configures `UMotionWarpingComponent` so the attack animation root motion snaps the player toward the nearest enemy.

**Attack types** (`EFSMotionWarpingAttackType`):
- `Ground` — ignores Z axis, stays in `MOVE_Walking`
- `Launcher` — uses Z axis, switches to `MOVE_Flying` at Begin, back to `MOVE_Falling` at End
- `Air` — uses Z axis, does NOT change movement mode (already airborne)

```
NotifyBegin → GetTargetForMotionWarp():
              1. If locked-on target within SearchRadius → use it
              2. Else → GetNearestEnemyFromPlayer(SearchRadius)
            → SetupGroundAttackMotionWarp() or SetupAirAttackMotionWarp()
              → Applies ForwardOffset (land in front, not on top of enemy)
              → Applies ZOffset (air attacks only)
              → Sets warp target on UMotionWarpingComponent

NotifyEnd   → Clears warp target
            → Launcher: restores MOVE_Falling
```

**Configurable per-notify in the montage:**
- `SearchRadius` (default 300) — how far to look for a target
- `ForwardOffset` — avoids overshooting into the enemy
- `ZOffset` — vertical correction for air attacks
- `attackType` — Ground / Launcher / Air
- `bDebugLines` — draws a debug sphere at warp target

---

## AnimNotifyState_MovementSpeed — "MaxWalkSpeedModifier"

**Purpose:** Overrides `MaxWalkSpeed` during an animation window (e.g., slow lunge during a heavy attack).

```
NotifyBegin → set MaxWalkSpeed = TargetSpeed (if bSnapSpeed)
NotifyTick  → FMath::FInterpTo(current, TargetSpeed, dt, InterpolationSpeed)
NotifyEnd   → restore correct speed:
              LockOn active → RunSpeedThreshold (600)
              LockOn off    → SprintSpeedThreshold (900)
```

Reads lock-on state from the cached `AFlowSlayerCharacter` reference to determine which speed to restore.

---

## AnimNotifyState_AirStall — "Air stall window"

**Purpose:** Zeroes gravity during an aerial attack so the player hangs in the air during the active frames.

```
NotifyBegin → cache GravityScale from CharacterMovementComponent
            → GravityScale = 0.f
            → MovementMode = MOVE_Flying
NotifyEnd   → restore cached GravityScale
            → MovementMode = MOVE_Falling
```

Also exists on enemies (`FSEnemy::StartAirStall`) as a code-driven version triggered by hit reactions.

---

## AnimNotifyState_RotateToTarget — "RotateToTarget"

**Purpose:** Snaps or smoothly rotates the character toward a target during the notify window.

| Mode | Target |
|---|---|
| Player (`bIsEnemy = false`) | `GetControlRotation()` (camera forward) |
| Enemy (`bIsEnemy = true`) | Player pawn location |

```
NotifyBegin → if bSnapRotation: SetActorRotation instantly
NotifyTick  → if !bSnapRotation: RInterpTo(current, target, dt, RotationSpeed)
```

---

## AnimNotifyState_WeaponTrail — "WeaponTrail"

**Purpose:** Spawns and drives the Niagara sword trail VFX during the attack swing.

```
NotifyBegin → get CombatComponent → get equippedWeapon
            → SpawnSystemAttached(NiagaraTrailSystem, WeaponBase socket) → TrailNiagaraBaseCompRef
            → SpawnSystemAttached(NiagaraTrailSystem, WeaponTip socket)  → TrailNiagaraTipCompRef
            → Activate both components
NotifyEnd   → Deactivate both components
```

Two Niagara components (base + tip sockets) define the trail ribbon's start and end points.

---

## AnimNotify_Launch

**Purpose:** Fires a launch impulse on the hit target (used for launcher attacks).

- Point-in-time notify (not a state)
- Reads the current hit target from `HitboxComponent` and applies `LaunchCharacter()` with configurable force
- Used by Launcher and PowerLauncher attacks to send enemies airborne

---

## How to Add a New Notify to a Montage

1. Open the montage in the animation editor
2. Add a notify track if needed
3. Right-click on the timeline → "Add Notify State" → find by display name (e.g., "Weapon active frame")
4. Drag to set the window duration
5. Select the notify state bar → configure properties in the Details panel (e.g., `HitboxProfile`, `TargetSpeed`, `attackType`)

**Typical attack montage notify layout:**
```
[RotateToTarget    |-------|                                          ]
[FSMotionWarping       |----------|                                   ]
[MovementSpeed    |-----|                                             ]
[WeaponTrail                |---------|                               ]
[Weapon active frame          |-----|                                 ]
[Combo Input Window                    |---------|                    ]
[Anim Cancel Window                                |---------|        ]
```
