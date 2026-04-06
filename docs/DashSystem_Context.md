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
WalkRun/Idle → InDashingStateAlias (bIsDashing AND bDashAnimCompleted) → DashingConduit (always true) → route by bLockOn + speed
```

### Exit Flow — DashToRun / DashToWalk
Transition vers WalkRun :
- `Automatic Rule Based on Sequence Player` + **`Automatic Rule Trigger Time = 0.26s`** (fire quand SafeMoveUpdated finit, 0.26s avant la fin de la séquence)
- **`Duration = 0.20s`** (blend court vers locomotion)
- `OnDashAnimEnded` bindé sur **Transition End** et **Transition Interrupt** → remet `bDashAnimCompleted = true`

### Exit Flow — Dashing8D
Transition vers WalkRun :
- Condition Blueprint : `NOT bIsDashing`
- **`Min Time Before Exit = 0.7s`** sur le state Dashing8D — empêche la transition de fire pendant 0.7s après l'entrée dans l'état
- **`Duration = 0.20s`**
- Pourquoi Min Time Before Exit : `ETriggerEvent::Triggered` (LShift tenu) re-fire `StartDash()` le frame d'après `EndDash()` → `bIsDashing = true` → condition `NOT bIsDashing` jamais stable sans ce guard. Solution native UE, zéro C++.

### bDashAnimCompleted — Double safety net
Variable ABP contrôlant la re-entrée dans les dash states (`bIsDashing AND bDashAnimCompleted` en entry condition).

**Chemin normal** : Transition End/Interrupt `DashToX → WalkRunAlias` fire `OnDashAnimEnded` → `bDashAnimCompleted = true` (immédiat dès que la transition finit).

**Chemin interrompu** (guard, mort, etc.) : fallback timer ABP (1.0s) fire `OnDashAnimEnded` → `bDashAnimCompleted = true`.

Setup du fallback timer :
- `OnDashStateEntry` (state entry function) → `bDashAnimCompleted = false`
- `AnimNotify_OnDashStateEntry` (state machine notification sur chaque dash state) → `Set Timer by Event (1.0s)` → callback = `OnDashAnimEnded`
- ⚠️ `AnimNotify_OnDashStateEntry` dans l'EventGraph est un **AnimNotify event** (déclenché par une notification de state machine), pas la même chose que la state entry function `OnDashStateEntry`. Les deux doivent exister pour que le système fonctionne.

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

---

## Bugs connus (diagnostiqués 2026-04-05, non encore fixés)

### Bug 1 — Dash locked définitivement (state stuck)
**Symptôme** : Le joueur ne peut plus dasher sans restart. Variable d'état bloquée.

**Cause** : `StopAllMontages()` appelé **en dehors** de `CombatComponent` (ex: `GameMode::HandleOnRunCompleted`, mort, écran pause) ne passe pas par `CancelAttack()` → `ResetComboState()` ne fire pas → `OnAttackingEnded` jamais broadcast → `DashComponent::bIsAttacking` reste `true` à jamais.

**Fix prévu** : Bind `AnimInstance->OnMontageEnded` dans `FSCombatComponent::BeginPlay()`. Si `bInterrupted=true` et `bIsAttacking=true`, appeler `ResetComboState()`. Le guard `bIsAttacking` évite le double reset quand c'est `CancelAttack` qui a stoppé la montage.

---

### Bug 2 — Dash cancel pas réactif (input lag + double press)
**Symptôme** : Dash cancel mid-attaque lent, peu réactif, feel mauvais. Nécessite deux pressions de Shift.

**Cause A — Double press** : `AnimCancelWindow` broadcast `OnAnimationCanceled` → `CancelAttack()` → état reset. Mais le dash n'est pas lancé. Le joueur doit re-presser Shift après le cancel. Pipeline actuelle = deux actions distinctes.

**Fix A prévu** : Input buffer dans `DashComponent`. Si `StartDash()` est appelé pendant `bIsAttacking` (seul bloqueur actif), stocker la direction dans `BufferedDashInput` + `bDashBuffered=true`. Dans `OnAttackingEnded()`, si `bDashBuffered`, consommer et lancer `StartDash(BufferedDashInput)` immédiatement. Vider le buffer dans `OnAttackingStarted()` (nouvelle attaque annule le buffer).

**Cause B — Blend-out trop long** : `CancelAttack(0.2f)` — la montage met 200ms à s'arrêter, l'animation est lente à transitionner.

**Fix B prévu** : Réduire le blend-out pour les dash cancels — passer `0.05f` au lieu de `0.2f`. Soit via un paramètre dans `HandleOnAnimationCanceled`, soit via une surcharge dédiée.

---

### Bug 3 — Transitions animation dash → locomotion lentes / absentes
**Symptôme** : Transition dash → sprint/marche trop lente ou absente. Animation parfois incorrecte.

**Cause** : Surtout côté ABP. La règle de sortie de `DashToRun`/`DashToWalk` requiert deux conditions simultanées (animation terminée ET `!bIsDashing`). Si le timing entre `EndDash()` et la fin d'animation est décalé d'un frame, la transition rate ou joue la mauvaise animation de stop. Le safety timer à `1.8s` (MaxDashDuration) peut aussi maintenir `bIsDashing=true` trop longtemps si `NotifyEnd` ne fire pas.

**Fix prévu** : ABP tuning (ajuster timing des conditions de sortie). Vérifier que `MaxDashDuration` est cohérent avec la durée réelle du montage dash. Inspecter si le blend-out de 0.2f post-cancel laisse l'ABP dans un état ambigu.
