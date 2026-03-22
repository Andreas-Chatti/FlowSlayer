# Lock-On System — Context
*Last updated: 2026-03-22*

## Key Files
| File | Role |
|------|------|
| `FSLockOnComponent.h/.cpp` | All lock-on logic: detection, switching, camera, validation |
| `FSFocusable.h` | Interface required for any lock-on candidate |
| `FSDamageable.h` | Used to check if target is dead (death-triggered disengage) |

---

## Architecture

```
Player (MMB pressed)
        │
        └─ FlowSlayerCharacter::ToggleLockOn()
                │
                ├─ Not locked → LockOnComponent->EngageLockOn()
                │       │
                │       ├─ FindTargetsInRadius() — sphere trace for pawns
                │       ├─ CollectValidTargets() — filter by IFSFocusable + IFSDamageable + alive
                │       ├─ FindBestScoredTarget() — rank by camera alignment dot product
                │       └─ SetCurrentTarget(best)
                │               ├─ OnLockOnStarted.Execute(target) → FlowSlayerCharacter
                │               │       ├─ CombatComponent->SetLockedOnTargetRef(target)
                │               │       └─ MaxWalkSpeed = RunSpeedThreshold (600)
                │               └─ SetPlayerLockOnMovementMode(true)
                │                       └─ bOrientRotationToMovement = false
                │
                └─ Already locked → LockOnComponent->DisengageLockOn()
                        └─ OnLockOnStopped.Broadcast() → FlowSlayerCharacter
                                ├─ CombatComponent->SetLockedOnTargetRef(nullptr)
                                └─ MaxWalkSpeed = SprintSpeedThreshold (900)
```

---

## Tick Behaviour

Each tick while locked on:
1. `LockOnValidCheck()` — verifies target is still alive and in range
   - On death → attempt `SwitchToNearestTarget()`, if none → disengage
   - Out of range → disengage
2. `UpdateLockOnCamera(deltaTime)`
   - `UpdatePlayerFacingTarget()` — smooth yaw rotation toward target
   - `UpdateCameraFacingTarget()` — interpolates controller rotation toward target with distance-based pitch/yaw offsets

---

## Target Switching

```
Player moves mouse X strongly (|lookAxis.X| > 1.0)
        │
        └─ FlowSlayerCharacter::HandleLookInput()
                └─ LockOnComponent->SwitchLockOnTarget(axisValueX)
                        │
                        ├─ FindTargetsInRadius() — re-scan for current targets
                        ├─ FindBestTargetInDirection() — filter by left/right relative to camera
                        └─ SetCurrentTarget(newTarget)
```

Switch is gated by `targetSwitchDelay` (0.65s timer) to prevent rapid flickering.

---

## Camera Offsets (distance-based)

| Situation | Pitch | Yaw |
|---|---|---|
| Far from target | -10° | 0° |
| Close to target | -5° | +30° |

Interpolated at `CameraRotationInterpSpeed = 8.0`.

---

## Movement Mode Changes

| Lock-On State | `bOrientRotationToMovement` | `MaxWalkSpeed` |
|---|---|---|
| Engaged | `false` | 600 (RunSpeedThreshold) |
| Disengaged | `true` | 900 (SprintSpeedThreshold) |

While locked on, `HandleMoveInput` uses `GetActorForwardVector` / `GetActorRightVector` instead of camera-relative directions.

---

## Target Validity Requirements

A target must:
1. Implement `IFSFocusable`
2. Implement `IFSDamageable`
3. Have `HealthComponent` not dead (`IsDead() == false`)
4. Be within `LockOnDetectionRadius` (default 2000 units)

---

## Delegates

| Delegate | Type | Fired when |
|---|---|---|
| `OnLockOnStarted(AActor*)` | `TDelegate` | Lock-on engages on a valid target |
| `OnLockOnStopped` | `FMulticastDelegate` | Lock-on disengages for any reason |

Both are bound in `FlowSlayerCharacter::BeginPlay`.

---

## Cached References

The component caches both interface pointers to avoid repeated casts per tick:
```cpp
IFSDamageable* CachedDamageableLockOnTarget;
IFSFocusable*  CachedFocusableTarget;
```
Both are updated in `SetCurrentTarget()`.
