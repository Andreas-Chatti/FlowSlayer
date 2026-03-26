# Dash System — Context
*Last updated: 2026-03-25*

## Key Files

| File | Role |
|------|------|
| `DashComponent.h/.cpp` | State machine, validation, direction snap, cooldown, safety timer, delegates |
| `AnimNotifyState_SafeMoveUpdated.h/.cpp` | Mouvement physique (SafeMove A→B, curve-driven) — authoring dans le montage |

---

## Architecture

Le dash est divisé en deux responsabilités distinctes :

| Responsabilité | Propriétaire |
|---|---|
| État logique (CanDash, bIsDashing, cooldown, direction, distance) | `DashComponent` |
| Mouvement physique (SafeMove A→B, timing, courbe) | `AnimNotifyState_SafeMoveUpdated` |

**Couplage directionnel :** `AnimNotifyState` dépend de `DashComponent` (lit SnappedInput2D + distance, appelle `EndDash()`). `DashComponent` ne sait pas que le notify existe.

---

## Dash Flow

```
Player presses LShift (no chord active)
        │
        └─ FlowSlayerCharacter::OnDashAction()
                └─ DashComponent->StartDash(InputManagerComponent->GetMoveInputAxis())
                        │
                        ├─ CanDash() check (voir section dédiée)
                        ├─ Snap input to 8 directions (45° increments) → SnappedInput2D
                        ├─ Compute world direction (camera-relative) from snapped input
                        ├─ Capture DashForward + DashLateral (dot products, frozen)
                        ├─ bIsDashing = true
                        ├─ Start safety timer (MaxDashDuration) → EndDash() if NotifyEnd never fires
                        ├─ bIsOnCooldown = true + start cooldown timer (CooldownDuration)
                        └─ OnDashStarted.Broadcast(FlowCost)
                                ├─ FSFlowComponent::RemoveFlow(FlowCost)
                                └─ FlowSlayerCharacter::HandleOnDashStarted()
                                        └─ if guarding → CombatComponent->ToggleGuard()

                                 ↓ (~15 frames later — montage joue → AnimNotifyState fire)

        AnimNotifyState_SafeMoveUpdated::NotifyBegin()
                ├─ Recompute world direction from SnappedInput2D + CURRENT camera yaw
                │   (not the stale Frame 1 direction — camera may have moved in ~15 frames)
                ├─ Start = character location
                └─ End = Start + dashDirection * GetDashDistance()

        AnimNotifyState_SafeMoveUpdated::NotifyTick() [each frame]
                ├─ TimeElapsed += FrameDeltaTime
                ├─ alpha = TimeElapsed / NotifyDuration
                ├─ curveValue = MoveCurve->GetFloatValue(alpha)
                ├─ deltaMove = (End - Start) * (curveValue - LastCurveValue)
                └─ SafeMoveUpdatedComponent(deltaMove)

        AnimNotifyState_SafeMoveUpdated::NotifyEnd()
                ├─ Reset internal state
                └─ DashComp->EndDash()
                        ├─ bIsDashing = false
                        └─ OnDashEnded.Broadcast()
```

**Cooldown design decision:** Le cooldown démarre dans `StartDash()`, pas dans `EndDash()`. Le joueur voit son cooldown partir immédiatement à l'appui — plus responsive, plus lisible pour un action game.

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

`bIsAttacking` est set par `CombatComponent::OnAttackingStarted/Ended` delegates (bindés dans le constructeur de `FlowSlayerCharacter`).

---

## 8-Direction Snapping

Input angle rounded to nearest 45° increment :
```cpp
float inputAngle   = Atan2(inputDirection.Y, inputDirection.X);
float snappedAngle = Round(inputAngle / HALF_PI * 2.f) * HALF_PI / 2.f;
FVector2D snappedInput{ FMath::Cos(snappedAngle), FMath::Sin(snappedAngle) };
SnappedInput2D = snappedInput;
```
Produit exactement 8 directions : 0°, 45°, 90°, 135°, 180°, -135°, -90°, -45°.
Cohérent avec le BlendSpace Dashing8D qui a 8 slots.

---

## Direction Handling (SnappedInput2D vs DashForward/DashLateral)

### Problème : direction stale
`StartDash()` est appelé au frame 1. `NotifyBegin()` fire ~15 frames plus tard (après anticipation du montage). Si la direction monde était calculée au frame 1 et stockée, la caméra peut avoir tourné entre temps → direction stale.

### Solution : deux séparations de responsabilité

| Donnée | Capturée où | Utilisée par | Pourquoi |
|---|---|---|---|
| `SnappedInput2D` (FVector2D) | `StartDash()` | `AnimNotifyState::NotifyBegin()` | Intention du joueur (stable) ; direction monde recalculée au moment où le mouvement start avec le camera yaw courant |
| `DashForward` / `DashLateral` (float) | `StartDash()` | ABP EventGraph Update (blend space) | Gelés une fois pour éviter le drift si le perso tourne pendant l'anim |

**`SnappedInput2D` a 15 frames de retard — c'est intentionnel.** Ce sont les coordonnées d'intention du joueur (2D espace input), pas la direction monde. La direction monde est recalculée dans `NotifyBegin()` à partir de `SnappedInput2D` + `GetControlRotation().Yaw` courant.

### ABP — lecture des blend weights
Dans l'EventGraph Update de l'ABP, lire directement :
```
DashComponent->GetDashForward()  → DashForward blend axis
DashComponent->GetDashLateral()  → DashLateral blend axis
```
Ces valeurs sont gelées à `StartDash()` — les lire chaque frame en Update est safe (pas de drift).
La variable ABP `DashDir` (FVector) est obsolète et doit être supprimée.

---

## AnimNotifyState_SafeMoveUpdated

### Propriétés (authoring dans le montage)
| Propriété | Description |
|---|---|
| `MoveCurve` | Float curve, X = temps [0,1], Y = déplacement [0,1]. Profil ease-out agressif recommandé |

### Durée
La durée du mouvement = la durée de la **barre notify dans le montage**. Pas de propriété séparée.
⚠️ La barre doit couvrir uniquement les frames de mouvement (pas l'anticipation ni la recovery).

### Distance
Lue depuis `DashComponent::GetDashDistance()`. Centralisée dans le composant, pas dans le notify.

### Robustesse
- `NotifyEnd` a un guard `if (DashComp)` — pas de crash si `NotifyBegin` n'a pas eu le temps de fire.
- Si la transition ABP se produit avant que `NotifyBegin` fire, `EndDash()` ne sera pas appelé → protégé par le **safety timer** dans `StartDash()`.

---

## Safety Timer

```cpp
// Dans StartDash() — fallback si NotifyEnd ne fire jamais (montage interrompu avant NotifyBegin)
GetWorld()->GetTimerManager().SetTimer(
    SafetyTimer,
    [this]() { if (bIsDashing) EndDash(); },
    MaxDashDuration,
    false
);
```

`MaxDashDuration` (default 1.8s, `EditDefaultsOnly`) doit être ≥ durée max d'un dash animation. Pattern professionnel standard pour tout état piloté par animation.

---

## Attack Interaction

`OnAttackingStarted()` force-end le dash si le joueur est en train de dasher :
```cpp
void UDashComponent::OnAttackingStarted()
{
    bIsAttacking = true;
    if (bIsDashing)
        EndDash();
}
```
Evite l'état `bIsDashing = true` bloqué si une attaque interrompt le montage de dash avant que `NotifyEnd` fire.

---

## Configurable Properties (DashComponent — set in Blueprint defaults)

| Property | Default | Description |
|---|---|---|
| `Distance` | 300 | Unités UE parcourues par dash |
| `CooldownDuration` | 0.5s | Délai avant prochain dash (démarre au press) |
| `MaxDashDuration` | 1.8s | Safety timer — EndDash() force-fired si NotifyEnd ne fire pas |
| `FlowCost` | 10 | Flow cost par dash |

### Propriétés supprimées
- ~~`DashDuration`~~ → durée = barre notify dans le montage
- ~~`DashCurve`~~ → `MoveCurve` dans l'AnimNotifyState
- ~~`DashDirectionWorld`~~ → remplacé par `SnappedInput2D` + recalcul dans `NotifyBegin()`

---

## DashCurve

Ease-out agressive : ~80% de la distance couverte dans les 30 premiers % du temps.
- Point gauche `(0, 0)` : tangente droite très pentue (quasi-verticale)
- Point droit `(1, 1)` : tangente gauche plate (horizontale)
- Aucun point intermédiaire

Résultat : le joueur "claque" à sa destination plutôt que de glisser.

---

## Delegate Wiring (FlowSlayerCharacter constructor)

```cpp
DashComponent->OnDashStarted.AddUObject(FlowComponent, &UFSFlowComponent::RemoveFlow);
DashComponent->OnDashStarted.AddUObject(this, &AFlowSlayerCharacter::HandleOnDashStarted);
DashComponent->CanAffordDash.BindUObject(FlowComponent, &UFSFlowComponent::HasEnoughFlow);
CombatComponent->OnAttackingStarted.AddUniqueDynamic(DashComponent, &UDashComponent::OnAttackingStarted);
CombatComponent->OnAttackingEnded.AddUniqueDynamic(DashComponent, &UDashComponent::OnAttackingEnded);
```

---

## Animation Blueprint — Dash Transitions (ABP_GreatSwordChar)

### Dash States in Locomotion

| State | Type | Condition | Use case |
|---|---|---|---|
| **Dashing8D** | BlendSpacePlayer (DashForward, DashLateral) | `bLockOn` | Lock-on: 8-dir blend space |
| **DashToRun** | SequencePlayer | `NOT bLockOn AND speed > RunSpeedThreshold` | Free: sprint-speed dash |
| **DashToWalk** | SequencePlayer | `NOT bLockOn AND speed <= RunSpeedThreshold` | Free: walk/run-speed dash |

### Entry Flow
```
WalkRun/Idle → InDashingStateAlias (bIsDashing) → DashingConduit (always true) → route by bLockOn + speed
```

### Exit Flow — DashToRun / DashToWalk
Transition vers WalkRun :
- `Automatic Rule Based on Sequence Player` + `Automatic Rule Trigger Time = 0.0s`
- Condition Blueprint supplémentaire : `NOT bIsDashing`
- Les deux doivent être vrais simultanément : animation terminée **ET** `EndDash()` appelé

### bDashAnimCompleted Variable
Variable ABP dérivée de `bIsDashing` (poll chaque frame dans EventGraph) :
```
EventGraph Update → bDashAnimCompleted = NOT bIsDashing
```
**Ne plus utiliser** `On State Fully Blended Out` / `On State Entry` pour setter cette variable — trop fragile sur les chaînes de transitions rapides.

Utilisée par le EventGraph pour contrôler DashForward/DashLateral (zéroïsé quand dash terminé).

### Stop Conduit — Dual Transition Rules
Deux transition rules par stop animation (SprintToIdle, RunToIdle, WalkToIdle) :

1. **Dash-specific route** (priorité haute) : `NOT bDashAnimCompleted AND [speed routing]`
   - Fire pendant le blend-out du dash (bDashAnimCompleted encore false)
   - Assure le bon stop même si WalkRun est entré mi-blend depuis un dash state

2. **Normal route** (priorité basse) : `[speed routing]` uniquement

### FrozenLastSpeed
- Set dans EventGraph : `if (bHasMovementInput) → FrozenLastSpeed = speed`
- Utilisé uniquement dans les routes du conduit Stop
- Pas utilisé dans WalkRun state

---

## Interactions with Other Systems

| System | Interaction |
|---|---|
| **Flow** | `OnDashStarted` → `RemoveFlow(FlowCost)`. Dash bloqué si `HasEnoughFlow == false` |
| **Combat** | Dash bloqué si `bIsAttacking`. Guard annulé au démarrage du dash. `OnAttackingStarted` force-end le dash si en cours |
| **Input** | `AnimNotifyState_AnimCancelWindow` type `Dash` peut trigger un dash cancel mid-attack |
| **Movement** | Ground-only — `IsFalling()` bloque le dash |

---

## Not Yet Implemented

- Aerial dash variant
- Dash I-frames (invincibility during dash)
- Directional dash VFX variants
