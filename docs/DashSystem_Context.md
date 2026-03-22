# Dash System — Context
*Last updated: 2026-03-22*

## Key Files
| File | Role |
|------|------|
| `DashComponent.h/.cpp` | All dash logic: movement, cooldown, VFX, flow cost |

---

## What the Dash Does

Moves the character along a **curve-driven trajectory** over a fixed duration (`DashDuration = 0.2s`).
- Input direction is **snapped to 8 directions** before conversion to world space
- Movement is driven by a `UCurveFloat` (X = normalized time [0,1], Y = normalized displacement [0,1])
- Uses `SafeMoveUpdatedComponent` per-frame delta — proper collision handling, no teleporting

---

## Dash Flow

```
Player presses LShift (no chord active)
        │
        └─ FlowSlayerCharacter::OnDashAction()
                └─ DashComponent->StartDash(InputManagerComponent->GetMoveInputAxis())
                        │
                        ├─ CanDash() check:
                        │   - Not already dashing
                        │   - Not on cooldown
                        │   - Not airborne (IsFalling == false)
                        │   - Not attacking (bIsAttacking == false)
                        │   - Has enough flow (CanAffordDash.Execute(FlowCost))
                        │   - inputDirection not zero
                        │
                        ├─ Snap input to 8 directions
                        ├─ Convert to world direction (camera-relative)
                        ├─ Compute dashStart / dashEnd
                        ├─ OnDashStarted.Broadcast(FlowCost)
                        │       ├─ FSFlowComponent::RemoveFlow(flowCost)
                        │       └─ FlowSlayerCharacter::HandleOnDashStarted()
                        │               └─ if guarding → CombatComponent->ToggleGuard() (cancel guard)
                        ├─ Activate AfterImages Niagara VFX
                        ├─ Play DashSound
                        └─ Enable Tick
                                │
                                └─ TickComponent (each frame):
                                        alpha = dashElapsed / DashDuration
                                        curveValue = DashCurve->GetFloatValue(alpha)
                                        deltaCurve = curveValue - lastCurveValue  ← per-frame delta
                                        deltaMove = (dashEnd - dashStart) * deltaCurve
                                        SafeMoveUpdatedComponent(deltaMove)
                                        │
                                        └─ alpha >= 1.0 → endDash()
                                                ├─ bIsDashing = false
                                                ├─ Disable Tick
                                                ├─ OnDashEnded.Broadcast()
                                                ├─ Deactivate AfterImages VFX
                                                └─ Start cooldown timer (DashCooldown = 0.5s)
```

---

## CanDash() Guard Conditions

```cpp
bool CanDash() const
{
    if (CanAffordDash.IsBound() && !CanAffordDash.Execute(FlowCost))
        return false;                          // Not enough flow

    return !bIsDashing                         // Not already dashing
        && !bIsOnCooldown                      // Cooldown expired
        && !IsFalling()                        // On the ground
        && !bIsAttacking;                      // Not mid-attack
}
```

`bIsAttacking` is set by `CombatComponent::OnAttackingStarted/Ended` delegates (bound in `FlowSlayerCharacter` constructor).

---

## Delegate Wiring (FlowSlayerCharacter constructor)

```cpp
// Flow cost deducted on dash
DashComponent->OnDashStarted.AddUObject(FlowComponent, &UFSFlowComponent::RemoveFlow);

// Guard cancelled on dash
DashComponent->OnDashStarted.AddUObject(this, &AFlowSlayerCharacter::HandleOnDashStarted);

// Affordability check delegated to FlowComponent
DashComponent->CanAffordDash.BindUObject(FlowComponent, &UFSFlowComponent::HasEnoughFlow);

// Sync attacking state from CombatComponent
CombatComponent->OnAttackingStarted.AddUniqueDynamic(DashComponent, &UDashComponent::OnAttackingStarted);
CombatComponent->OnAttackingEnded.AddUniqueDynamic(DashComponent, &UDashComponent::OnAttackingEnded);
```

---

## 8-Direction Snapping

Input angle is rounded to the nearest 45° increment:
```cpp
float inputAngle  = Atan2(inputDirection.Y, inputDirection.X);
float snappedAngle = Round(inputAngle / (PI/2) * 4) * (PI/2) / 4;
```
This ensures clean directional dashes (forward, back, left, right, and 4 diagonals) regardless of analog stick drift or partial WASD presses.

---

## Configurable Properties (set in Blueprint defaults)

| Property | Default | Description |
|---|---|---|
| `DashCurve` | — | Float curve driving movement profile (required) |
| `DashDistance` | 600 | Total units covered |
| `DashDuration` | 0.2s | Total time for one dash |
| `DashCooldown` | 0.5s | Delay before next dash allowed |
| `FlowCost` | 10 | Flow units consumed per dash |
| `DashMontage` | — | Anim montage played during dash |
| `DashSound` | — | Sound played at dash start |
| `DashVFX` | — | Niagara after-image trail system |

---

## VFX

The after-image trail (`AfterImagesVFXComp`) is a `UNiagaraComponent` attached to the mesh at BeginPlay (`InitDashVFXComp`). It is:
- **Activated** at `StartDash()`
- **Deactivated** at `endDash()`

The Niagara system asset is assigned in Blueprint defaults via `DashVFX`.

---

## Interactions with Other Systems

| System | Interaction |
|---|---|
| **Flow** | `OnDashStarted` → `RemoveFlow(FlowCost)`. Dash blocked if `HasEnoughFlow(FlowCost) == false` |
| **Combat** | Dash blocked while `bIsAttacking`. Guard is cancelled on dash start |
| **Input** | `AnimNotifyState_AnimCancelWindow` with type `Dash` can trigger a dash cancel mid-attack (fires `OnAnimationCanceled`) |
| **Movement** | Dash is **ground-only** — `IsFalling()` blocks it. No aerial dash |

---

## Not Yet Implemented

- Aerial dash variant
- Dash I-frames (invincibility during dash)
- Directional dash VFX variants (different trail per direction)
