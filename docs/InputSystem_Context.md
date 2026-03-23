# Input System — Context
*Last updated: 2026-03-23 (dual mappings for all chorded attacks, IsInputActionTriggered replaces IsInputActionDown, JumpForwardSlam added to dash+guard disambiguation)*

## Key Files
| File | Role |
|------|------|
| `InputManagerComponent.h/.cpp` | Enhanced Input bindings, fires delegates with `UInputAction*` |
| `FlowSlayerCharacter.cpp` | Bridges input domain → combat domain via `TMap<UInputAction*, EAttackType>` |

---

## Architecture (3-layer separation)

```
[UE Enhanced Input System]
         │  UInputAction fires (Chorded Actions set in editor)
         ▼
[InputManagerComponent]
         │  knows only UInputAction*, has NO dependency on CombatData.h
         │  fires: OnAttackInputReceived(const UInputAction*)
         ▼
[FlowSlayerCharacter]
         │  TMap<UInputAction*, EAttackType> InputActionToAttackType
         │  resolves state variants (standing/running/air)
         │  calls: CombatComponent->OnAttackInputReceived(EAttackType)
         ▼
[FSCombatComponent]
         │  TMap<EAttackType, FCombo*> ComboLookupTable
         ▼
[AnimMontage]
```

**Core rule:** `InputManagerComponent` never imports `CombatData.h`. It is a pure input relay. All domain logic lives in `FlowSlayerCharacter`.

---

## Delegates (InputManagerComponent → FlowSlayerCharacter)

| Delegate | Trigger | Bound to |
|---|---|---|
| `OnAttackInputReceived(UInputAction*)` | Any attack input | `OnAttackInputActionReceived` |
| `OnMoveInput(FVector2D)` | Move axis active | `HandleMoveInput` |
| `OnLookInput(FVector2D)` | Look axis active | `HandleLookInput` |


| `OnLShiftKeyTriggered` | LShift alone (no chord) | `OnDashAction` |
| `OnSpaceKeyStarted` | Space pressed | `HandleOnSpaceKeyStarted` |
| `OnSpaceKeyCompleted` | Space released | `HandleOnSpaceKeyCompleted` |
| `OnMiddleMouseButtonClicked` | Middle mouse | `ToggleLockOn` |
| `OnGuardActionTriggered` | 'A' key | `HandleGuardInput` |

---

## InputAction Inventory

### Base actions
| Action | Key | Event |
|---|---|---|
| `JumpAction` | Space | Started / Completed |
| `MoveAction` | ZQSD | Triggered / Completed |
| `LookAction` | Mouse XY | Triggered |
| `MiddleMouseAction` | MMB | Started |
| `GuardAction` | A | Started |
| `LShiftAction` | LShift | Triggered |

### Attack actions (one `UInputAction` per attack, configured in editor)
| Action | Chord combo | Event |
|---|---|---|
| `LightAttackAction` | LMB | Triggered |
| `HeavyAttackAction` | RMB | Triggered |
| `DashPierceAction` | LMB + chord(LShift) + chord(Z) | Started |
| `DashSpinningSlashAction` | LMB + chord(LShift) + chord(Q or D) | Started |
| `DashDoubleSlashAction` | F + chord(LShift) **OR** LShift + chord(F) — ground only | Started |
| `DashBackSlashAction` | E + chord(LShift) **OR** LShift + chord(E) — ground only | Started |
| `JumpSlamAttackAction` | E + chord(Z) **OR** Z + chord(E) — ground + air | Started |
| `JumpForwardSlamAttackAction` | LShift + chord(A) **OR** A + chord(LShift) — ground + air | Started |
| `JumpUpperSlamAttackAction` | RMB + chord(Z) **OR** Z + chord(RMB) — ground + air | Started |
| `LauncherAttackAction` | LMB + chord(A) | Started |
| `PowerLauncherAttackAction` | RMB + chord(A) | Started |
| `SpinAttackAction` | E + chord(LMB) **OR** LMB + chord(E) | Started |
| `HorizontalSweepAttackAction` | E + chord(RMB) **OR** RMB + chord(E) | Started |
| `PowerSlashAttackAction` | F + chord(S) **OR** S + chord(F) | Started |
| `PierceThrustAttackAction` | F + chord(Z) **OR** Z + chord(F) | Started |
| `GroundSlamAttackAction` | S + chord(RMB) **OR** RMB + chord(S) | Started |
| `DiagonalRetourneAttackAction` | S + chord(LMB) **OR** LMB + chord(S) | Started |

---

## Handler Naming Convention (InputManagerComponent)

All handlers follow the `HandleOn` prefix:
- `HandleOnMoveTriggered` / `HandleOnMoveCompleted`
- `HandleOnLookTriggered`
- `HandleOnJumpStarted` / `HandleOnJumpCompleted`
- `HandleOnDashTriggered`, `HandleOnGuardTriggered` — kept separate (disambiguation conditions)
- `HandleOnMiddleMouseButtonStarted`

No named handler for LMB/RMB or standard attacks — all bound via inline lambdas (see below).

### Attack Bindings (inline lambdas, no named handlers)

All attack actions are bound via `BindActionValueLambda` in a loop, capturing the `UInputAction*` directly:

```cpp
// Standard attacks (ETriggerEvent::Started)
for (UInputAction* Action : AttackActions)
    EnhancedInputComponent->BindActionValueLambda(Action, ETriggerEvent::Started,
        [this, Action](const FInputActionValue&) { OnAttackInputReceived.ExecuteIfBound(Action); });

// LMB / RMB (ETriggerEvent::Triggered — fires every frame while held)
for (UInputAction* Action : { LightAttackAction, HeavyAttackAction })
    EnhancedInputComponent->BindActionValueLambda(Action, ETriggerEvent::Triggered,
        [this, Action](const FInputActionValue&) { OnAttackInputReceived.ExecuteIfBound(Action); });
```

---

## State-Based Attack Resolution (FlowSlayerCharacter)

`LightAttackAction` and `HeavyAttackAction` are state-determined:

```cpp
// LMB
if (IsFalling || IsFlying) → EAttackType::AirCombo
else if (Speed > RunSpeedThreshold) → EAttackType::RunningLight
else → EAttackType::StandingLight

// RMB
if (IsFalling || IsFlying) → EAttackType::AerialSlam
else if (Speed > RunSpeedThreshold) → EAttackType::RunningHeavy
else → EAttackType::StandingHeavy
```

`RunSpeedThreshold` = 600 (locked-on max speed). `SprintSpeedThreshold` = 900 (free movement max).

---

## Dash Disambiguation

Dash (LShift) and LShift-chorded attacks share the LShift key. Disambiguation logic:

```cpp
void HandleOnDashTriggered()
{
    // If any LShift-involved attack is active, don't fire the dash
    if (IsInputActionTriggered(DashPierceAction) || IsInputActionTriggered(DashSpinningSlashAction)
     || IsInputActionTriggered(DashDoubleSlashAction) || IsInputActionTriggered(DashBackSlashAction)
     || IsInputActionTriggered(JumpForwardSlamAttackAction))
        return;

    OnLShiftKeyTriggered.ExecuteIfBound();
}
```

`IsInputActionTriggered()` uses `FindActionInstanceData()->GetTriggerEvent()` — checks if the action is truly in `Started` or `Triggered` state (full chord satisfied), not just raw input value.

---

## Chorded Action Rules (Critical)

- **Never use ZQSD as primary trigger** — it consumes movement input and stops the character
- **Dual mappings**: every 2-key chord has TWO IMC entries (A/B reversed) so both press orders work. Example: `F + chord(LShift)` AND `LShift + chord(F)` for DashDoubleSlash
- **Disambiguation required**: any action whose key is also a solo-action (LShift=dash, A=guard) must be added to that solo-action's disambiguation guard
- **OR-chord** (e.g., Q or D for DashSpinningSlash): two separate key mappings on the same `UInputAction`
- `bConsumeInput` on the chorded action asset controls whether the primary key is consumed from other actions

---

## Helper Methods

```cpp
// Check if an InputAction is truly in Started or Triggered state (full chord satisfied)
// Uses FindActionInstanceData()->GetTriggerEvent() — NOT raw GetActionValue()
bool IsInputActionTriggered(const UInputAction* inputAction) const;

// Check if a raw key is pressed or was just pressed (used by AnimNotifies)
bool GetInputKeyState(FKey inputKey) const;

// Returns the current 2D move axis (used by DashComponent for dash direction)
FVector2D GetMoveInputAxis() const;

// Removes the MappingContext and calls DisableInput on the controller
void DisableAllInputs();
```

---

## Design Decisions

### LightAttack / HeavyAttack — `ETriggerEvent::Triggered` (intentionnel)
LMB et RMB utilisent `Triggered` (fire chaque frame tant que le bouton est tenu), pas `Started`.
**Raison :** Comportement voulu — maintenir LMB/RMB tenu spamme en continu les attaques standing/running, ce qui est le feel recherché pour le combat.
`CombatComponent` gère la fréquence via `bIsAttacking` et les fenêtres de combo.

### Dash disambiguation
`HandleOnDashTriggered` vérifie toutes les attaques utilisant LShift avant de fire le dash normal :
```cpp
if (IsInputActionTriggered(DashPierceAction) || IsInputActionTriggered(DashSpinningSlashAction)
 || IsInputActionTriggered(DashDoubleSlashAction) || IsInputActionTriggered(DashBackSlashAction)
 || IsInputActionTriggered(JumpForwardSlamAttackAction))
    return;
```

### Guard disambiguation
`HandleOnGuardTriggered` vérifie toutes les attaques utilisant A avant de fire la garde :
```cpp
if (IsInputActionTriggered(LauncherAttackAction) || IsInputActionTriggered(PowerLauncherAttackAction)
 || IsInputActionTriggered(JumpForwardSlamAttackAction))
    return;
```

---

## Future Architecture Note

If a second character is added, consider `UGreatSwordInputComponent : UInputManagerComponent` per character. The derived component still fires `UInputAction*` (not `EAttackType`) — the `TMap` stays in the character class to preserve domain separation.
