# Input System — Context
*Last updated: 2026-03-25*

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
| `OnHealActionTriggered` | LShift + Space | `HandleHealInput` |

---

## InputAction Inventory

### Base actions
| Action | Key | IMC Trigger | C++ Binding | Notes |
|---|---|---|---|---|
| `JumpAction` | Space | *(none — Down behavior)* | Started / Completed | No explicit trigger → Completed fires only on real key release |
| `MoveAction` | ZQSD | *(none — Down behavior)* | Started / Triggered / Completed | — |
| `LookAction` | Mouse XY | *(none)* | Triggered | — |
| `MiddleMouseAction` | MMB | *(none)* | Started | — |
| `GuardAction` | A | *(none)* | Started | — |
| `LShiftAction` | LShift | *(none)* | Triggered | — |
| `HealAction` | LShift + Space **OR** Space + LShift | *(none — Down behavior)* | Started | Down behavior required — see Heal/Jump conflict section |

### Attack actions (one `UInputAction` per attack, configured in editor)
| Action | Chord combo | IMC Trigger | Notes |
|---|---|---|---|
| `LightAttackAction` | LMB | Pulse (TriggerOnStart=true, interval) | Re-fires while held |
| `HeavyAttackAction` | RMB | Pulse (TriggerOnStart=true, interval) | Re-fires while held |
| `DashPierceAction` | LMB + chord(LShift) + chord(Z) | Pulse (TriggerOnStart=true) | — |
| `DashSpinningSlashAction` | LMB + chord(LShift) + chord(Q or D) | Pulse (TriggerOnStart=true) | — |
| `DashDoubleSlashAction` | F + chord(LShift) **OR** LShift + chord(F) — ground only | Pulse (TriggerOnStart=true) | — |
| `DashBackSlashAction` | E + chord(LShift) **OR** LShift + chord(E) — ground only | Pulse (TriggerOnStart=true) | — |
| `JumpSlamAttackAction` | E + chord(Z) **OR** Z + chord(E) — ground + air | Pulse (TriggerOnStart=true) | — |
| `JumpForwardSlamAttackAction` | LShift + chord(A) **OR** A + chord(LShift) — ground + air | Pulse (TriggerOnStart=true) | — |
| `JumpUpperSlamAttackAction` | RMB + chord(Z) **OR** Z + chord(RMB) — ground + air | Pulse (TriggerOnStart=true) | — |
| `LauncherAttackAction` | LMB + chord(A) | Pulse (TriggerOnStart=true) | — |
| `PowerLauncherAttackAction` | RMB + chord(A) | Pulse (TriggerOnStart=true) | — |
| `SpinAttackAction` | E + chord(LMB) **OR** LMB + chord(E) | Pulse (TriggerOnStart=true) | — |
| `HorizontalSweepAttackAction` | E + chord(RMB) **OR** RMB + chord(E) | Pulse (TriggerOnStart=true) | — |
| `PowerSlashAttackAction` | F + chord(S) **OR** S + chord(F) | Pulse (TriggerOnStart=true) | — |
| `PierceThrustAttackAction` | F + chord(Z) **OR** Z + chord(F) | Pulse (TriggerOnStart=true) | — |
| `GroundSlamAttackAction` | S + chord(RMB) **OR** RMB + chord(S) | Pulse (TriggerOnStart=true) | — |
| `DiagonalRetourneAttackAction` | S + chord(LMB) **OR** LMB + chord(S) | Pulse (TriggerOnStart=true) | — |

**Pulse TriggerOnStart=true :** toutes les attaques utilisent ce trigger intentionnellement. TriggerOnStart fire au press immédiatement, puis re-fire à intervalle régulier si le joueur maintient la touche. `CombatComponent` gère la fréquence effective via `bIsAttacking` et les fenêtres de combo.

---

## C++ Bindings (SetupInputBindings)

```cpp
// Jump — no explicit trigger in IMC (Down behavior)
EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, ...HandleOnJumpStarted);
EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, ...HandleOnJumpCompleted);

// Heal — Started only (one-shot per press)
EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started, ...HandleOnHealTriggered);

// All attack actions — ETriggerEvent::Triggered (matches Pulse TriggerOnStart behavior)
for (UInputAction* Action : AttackActions)
    EnhancedInputComponent->BindActionValueLambda(Action, ETriggerEvent::Triggered,
        [this, Action](const FInputActionValue&) { OnAttackInputReceived.ExecuteIfBound(Action); });
```

---

## Handler Naming Convention (InputManagerComponent)

All handlers follow the `HandleOn` prefix:
- `HandleOnMoveTriggered` / `HandleOnMoveCompleted`
- `HandleOnLookTriggered`
- `HandleOnJumpStarted` / `HandleOnJumpCompleted`
- `HandleOnDashTriggered`, `HandleOnGuardTriggered`, `HandleOnHealTriggered` — kept separate (disambiguation conditions)
- `HandleOnMiddleMouseButtonStarted`

No named handler for attack actions — all bound via inline lambdas in a loop.

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

Dash (LShift) et les attaques LShift-chordées partagent la même touche. Logique de garde :

```cpp
void HandleOnDashTriggered()
{
    if (IsInputActionTriggered(DashPierceAction) || IsInputActionTriggered(DashSpinningSlashAction)
     || IsInputActionTriggered(DashDoubleSlashAction) || IsInputActionTriggered(DashBackSlashAction)
     || IsInputActionTriggered(JumpForwardSlamAttackAction))
        return;

    OnLShiftKeyTriggered.ExecuteIfBound();
}
```

`IsInputActionTriggered()` utilise `FindActionInstanceData()->GetTriggerEvent()` — vérifie que l'action est vraiment en état `Started` ou `Triggered` (chord complet satisfait).

Note : `HealAction` n'a plus besoin d'être vérifié ici — le ChordBlocker UE gère le conflit LShift+Dash via la priorité IMC (voir section suivante).

---

## Heal / Jump Conflict — Solution via IMC Priority + ChordBlocker

### Problème
`IA_Heal` = LShift + Space. Si le joueur appuie LShift+Space pour healer avec peu de flow :
1. `IA_Dash` pourrait fire (LShift seul) et consommer du flow avant le heal
2. `IA_Jump` pourrait fire après le heal quand Space est relâché

### Solution : priorité IMC + comportement Down

`IA_Heal` est à une priorité IMC **plus haute** que `IA_Jump` (index plus bas = plus haute priorité) et que `IA_LShift` (dash).

`IA_Heal` n'a **aucun trigger explicite** dans l'IMC (comportement Down implicite) → reste en état `Triggered` tant que LShift+Space sont maintenus → UE injecte un **ChordBlocker** dans `IA_Jump` et `IA_LShift` pour toute la durée → ils ne peuvent pas atteindre l'état `Triggered` → leurs delegates ne fire jamais pendant le heal.

### Pourquoi pas Pressed sur IA_Heal
Si `IA_Heal` utilisait `Pressed`, le ChordBlocker ne resterait actif qu'**un seul frame** (Pressed = Completed au frame suivant même si touche maintenue). `IA_Jump` pourrait re-trigger dès le frame 2. Le comportement Down (aucun trigger explicite) maintient le ChordBlocker actif toute la durée.

### Pourquoi pas Pressed sur IA_Jump
`Pressed` fire `Completed` au frame 2 même si Space est encore maintenu → `StopJumping()` appelé prématurément. Sans trigger explicite, `Completed` ne fire que sur le vrai release de la touche.

### ChordBlocker — mécanisme UE
UE injecte automatiquement un `ChordBlocker` dans les actions de priorité basse qui partagent la même touche primaire qu'une action chord de priorité haute qui fire. Le ChordBlocker reste actif tant que l'action parente est en état `Triggered` (comportement Down). Il est supprimé dès que l'action parente sort de `Triggered`.

---

## Guard Disambiguation

```cpp
void HandleOnGuardTriggered()
{
    if (IsInputActionTriggered(LauncherAttackAction) || IsInputActionTriggered(PowerLauncherAttackAction)
     || IsInputActionTriggered(JumpForwardSlamAttackAction))
        return;

    OnGuardActionTriggered.ExecuteIfBound();
}
```

---

## Chorded Action Rules (Critical)

- **Never use ZQSD as primary trigger** — consomme le mouvement et stoppe le perso
- **Dual mappings** : chaque chord 2-touches a DEUX entrées IMC (A/B inversé) pour les deux ordres d'appui. Exemple : `F + chord(LShift)` ET `LShift + chord(F)` pour DashDoubleSlash
- **Disambiguation required** : toute action dont la touche est aussi une action solo (LShift=dash, A=guard) doit être ajoutée au guard de cette action solo
- **OR-chord** (ex. Q ou D pour DashSpinningSlash) : deux mappings séparés sur le même `UInputAction`
- `bConsumeInput` sur l'asset chorded action contrôle si la touche primaire est consommée pour les autres actions

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

### LightAttack / HeavyAttack — Pulse (TriggerOnStart)
Pulse avec TriggerOnStart fire immédiatement au press, puis à intervalle si maintenu. Combiné avec `ETriggerEvent::Triggered` en C++, ce pattern permet de spammer les attaques en maintenant LMB/RMB, tout en laissant `CombatComponent` gérer la cadence réelle via `bIsAttacking` et les fenêtres de combo.

### Cooldown LShift/Dash → démarre au press, pas à la fin
Voir `DashSystem_Context.md` — décision de design dans `DashComponent`.

### Heal IMC priority > Jump/Dash
Seul moyen de garantir que le ChordBlocker reste actif pendant toute la durée LShift+Space. Toute solution basée sur un flag (ex. `bHealWasTriggered`) est plus fragile car elle dépend de l'ordre d'exécution des delegates dans un même frame.

---

## Future Architecture Note

If a second character is added, consider `UGreatSwordInputComponent : UInputManagerComponent` per character. The derived component still fires `UInputAction*` (not `EAttackType`) — the `TMap` stays in the character class to preserve domain separation.
