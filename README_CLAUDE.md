# FlowSlayer — Context for AI Assistants

> For code style and conventions, see `CLAUDE.md`.

---

## Project Overview

3D hack'n'slash roguelite (Hades / Devil May Cry / Dead Cells / Furi).
- **Engine:** Unreal Engine 5.4.4
- **Language:** ~95% C++ for gameplay, Blueprints only for asset wiring (montages, VFX, datatable refs)
- **Platform:** PC (KB+M), third-person 3D
- **Main branch:** `main`

---

## File Structure

```
Source/FlowSlayer/
│
├── FlowSlayerCharacter.cpp/.h       # Main player character — orchestrates all components
├── FlowSlayerGameMode.cpp/.h        # Game mode (minimal)
│
├── Public/
│   ├── CombatData.h                 # EAttackType enum, FAttackData, FCombo structs (shared combat types)
│   │
│   ├── FSCombatComponent.h          # Combat logic: combos, attack execution, combo state machine
│   ├── FSFlowComponent.h            # Flow/Momentum resource system
│   ├── FSLockOnComponent.h          # Lock-on target acquisition and switching
│   ├── DashComponent.h              # Dash movement + cooldown + flow cost
│   ├── HealthComponent.h            # HP, damage reception, death event
│   ├── InputManagerComponent.h      # Enhanced Input → fires delegates (UInputAction*)
│   ├── HitboxComponent.h            # Sweep/overlap hit detection during attacks
│   ├── HitFeedbackComponent.h       # Knockback, hitstop, camera shake on hit
│   │
│   ├── FSWeapon.h                   # Weapon actor (attached to character socket)
│   ├── FSDamageable.h               # Interface: NotifyHitReceived()
│   ├── FSFocusable.h                # Interface: for lock-on eligible actors
│   │
│   ├── FSEnemy.h                    # Base enemy class
│   ├── FSEnemy_Grunt.h              # Grunt enemy variant
│   ├── FSEnemy_Runner.h             # Runner enemy variant
│   ├── FSEnemyAIController.h        # AI controller (BehaviorTree-driven)
│   │
│   ├── FSArenaManager.h             # Arena/room manager
│   ├── AFSSpawnZone.h               # Enemy spawn zone
│   ├── FSProjectile.h               # Projectile actor
│   │
│   └── AnimNotify*/
│       ├── AnimNotifyState_Hitbox.h           # Activates hitbox during attack window
│       ├── AnimNotifyState_ComboWindow.h      # Opens/closes combo input window
│       ├── AnimNotifyState_AnimCancelWindow.h # Opens dash/jump cancel window
│       ├── AnimNotifyState_FSMotionWarping.h  # Sets motion warp target
│       ├── AnimNotifyState_MovementSpeed.h    # Overrides MaxWalkSpeed during anim
│       ├── AnimNotifyState_RotateToTarget.h   # Snaps character yaw toward target
│       ├── AnimNotifyState_AirStall.h         # Sets GravityScale=0 during aerial attacks
│       ├── AnimNotifyState_WeaponTrail.h      # Triggers weapon trail VFX
│       └── AnimNotify_Launch.h                # Applies launch impulse to hit target
│
└── Private/                         # Implementations (mirrors Public/)
```

---

## System Architecture

### Input Pipeline (3-layer, strict separation)

```
[UE Enhanced Input]
        ↓  UInputAction fires (Chorded Actions configured in editor)
[InputManagerComponent]
        ↓  OnAttackInputReceived(const UInputAction*)
[FlowSlayerCharacter]
        ↓  TMap<UInputAction*, EAttackType> lookup + state resolution
[FSCombatComponent]
        ↓  TMap<EAttackType, FCombo*> ComboLookupTable
[Animation Montage]
```

**Key constraint:** `InputManagerComponent` has **no dependency on `CombatData.h`**. It only speaks `UInputAction*`. `FlowSlayerCharacter` is the bridge between input domain and combat domain.

#### Chorded Attack Disambiguation
- Each attack has its own `UInputAction` asset with Chorded Action triggers set in editor
- **Never use ZQSD as primary trigger** — it consumes movement input and stops the character
- Example: DashPierce (LShift+Z+LMB) → LMB is main trigger, Z and LShift are chord conditions
- Dash vs DashAttack conflict: `HandleOnDashTriggered` calls `IsInputActionDown(DashPierceAction)` before firing regular dash
- OR-chord (e.g., DashSpinningSlash on Q or D): two mappings on the same `UInputAction`, different chord key each

---

### Combat State Machine (FSCombatComponent)

```
Idle
 └─ OnAttackInputReceived(EAttackType)
     └─ GetComboFromContext() → selects FCombo*
         └─ ExecuteAttack(montage)
             ├─ ComboWindow opens  (AnimNotifyState_ComboWindow)
             │   └─ input received → ContinueCombo() or ChainingToNextCombo()
             └─ ComboWindow closes → ResetComboState()
```

Key state variables:
- `bIsAttacking` — true while any montage is running
- `bComboInputWindowOpen` — true only during the combo window notify
- `bContinueCombo` — an input was received during the window
- `ComboIndex` — current position in the FCombo chain
- `PendingCombo` — next combo when player chains to a different attack type

---

### Flow/Momentum System (FSFlowComponent)

- **Gains** flow on hits landed → `HandleOnHitLanded`
- **Loses** flow on dash → `DashComponent::OnDashStarted`
- **Loses** flow on hit received → `HealthComponent::OnDamageReceived`
- At max tier: damage received is negated (hit immunity)

---

## Key Data Types (CombatData.h)

**`EAttackType`**
```
StandingLight, StandingHeavy, RunningLight, RunningHeavy,
AirCombo, AerialSlam,
DashPierce, DashSpinningSlash, DashDoubleSlash, DashBackSlash,
JumpSlam, JumpForwardSlam, JumpUpperSlam,
Launcher, PowerLauncher,
SpinAttack, HorizontalSweep,
PowerSlash, PierceThrust, GroundSlam, DiagonalRetourne
```

**`FAttackData`** — Damage, KnockbackForce, KnockbackUpForce, FlowReward, montage ref, hitbox params.

**`FCombo`** — Ordered list of `FAttackData` entries (one full combo chain).

---

## Implemented Features

- [x] Flow/Momentum system (hit immunity at max tier)
- [x] 20+ attacks with chorded input disambiguation
- [x] Combo system (chain attacks, combo window, state machine)
- [x] Combo hit counter + streak UI (timed reset)
- [x] Guard system (toggle, cancelled by dash/jump/move)
- [x] Lock-on with target switching (mouse X axis threshold)
- [x] Double jump
- [x] Dash (flow cost, cooldown, cancel window from attacks)
- [x] Weapon trail VFX
- [x] Enemy stun + death animations (Grunt, Runner)
- [x] Arena manager + spawn zones
- [ ] Procedural dungeon generation
- [ ] Modular weapon craft (Blade + Handle + Gem)
- [ ] Full gameloop / run structure
