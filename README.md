# Flow Slayer

A **3D action roguelite** built with **Unreal Engine 5.7** and **C++**, featuring a stylish combo-based combat system inspired by Hades, Devil May Cry, Dead Cells, and Furi.

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7.4-blue?logo=unrealengine)
![C++](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)
[![Dev Videos](https://img.shields.io/badge/Dev%20Videos-Google%20Drive-4285F4?logo=googledrive&logoColor=white)](https://drive.google.com/drive/folders/13BXUaIHeYjbny9D94_BoQ9VAFmhFtopz?usp=sharing)

---

## About

Flow Slayer is a skill-based action game where players fight through a series of arenas in short runs (30–40 min). The core mechanic revolves around **Flow/Momentum** — sustaining combos builds power and eventually grants damage immunity, while taking hits or dashing costs Flow.

- Fast-paced combat with 20+ chainable attacks — ground combos, dash attacks, launchers, aerial moves
- Air combat system with launchers, juggles, and aerial slams
- Lock-on targeting with dynamic camera behavior
- Run-based progression with XP, level-ups, and milestone upgrades
- Pure skill expression — the combat system rewards mastery, not stat grinding

---

## Technical Highlights

### Combat System
Built around a modular `FSCombatComponent` (~95% C++, Blueprints only for asset wiring):

- **Combo Chaining** — Animation-driven combo windows allow seamless attack transitions
- **Chorded Input** — Each attack has its own `UInputAction` with Chorded Actions configured in the editor via a strict 3-layer input pipeline — no manual key-state checks in code
- **Motion Warping** — Dynamic target tracking during attacks for satisfying hit connection
- **Hitstop** — Frame-perfect time dilation on hit for impactful feedback
- **Knockback Physics** — Directional knockback with horizontal and vertical force vectors

### Input Pipeline — strict 3-layer separation
```
[Enhanced Input] → InputManagerComponent (UInputAction*)
               → FlowSlayerCharacter    (TMap<UInputAction*, EAttackType>)
               → FSCombatComponent      (TMap<EAttackType, FCombo*>)
               → Animation Montage
```
`InputManagerComponent` has no dependency on combat types — `FlowSlayerCharacter` is the only bridge between input and combat domains.

### Flow / Momentum System
- Gains on hit landed, loses on dash or damage received
- Four tiers with escalating effects — max tier grants full damage immunity
- Drives all resource decisions (heal cost, dash cost, upgrade eligibility)

### Air Combat
- Launcher attacks send enemies airborne with trajectory peak detection
- Reduced gravity during aerial combos via animation notifies
- Enemies freeze briefly at apex, creating juggle windows

### Game Loop
- **RunManager** — singleton orchestrator for arena transitions and run completion
- **FSArenaManager** — per-arena encounter: spawn budget, max-alive cap, dynamic escalation, owns its exit portal
- **AArenaPortal** — placed in the level, hidden until arena clear; teleports player via a `DestinationActor` reference
- **ProgressionComponent** — 30 levels per run, XP curve `60 + (n-1)*5`, milestone events every 5 levels

---

## Architecture

```
Source/FlowSlayer/
├── FlowSlayerCharacter.h/cpp        // Player — orchestrates all components
├── Public/
│   ├── CombatData.h                 // EAttackType, FAttackData, FCombo (shared types)
│   ├── FSCombatComponent.h          // Combo state machine, attack execution
│   ├── FSFlowComponent.h            // Flow/Momentum resource
│   ├── FSLockOnComponent.h          // Lock-on acquisition and switching
│   ├── DashComponent.h              // Dash movement, cooldown, flow cost
│   ├── HealthComponent.h            // HP, death event
│   ├── ProgressionComponent.h       // XP, level-up, milestones
│   ├── InputManagerComponent.h      // Enhanced Input → UInputAction* delegates
│   ├── FSArenaManager.h             // Arena encounter + exit portal ownership
│   ├── RunManager.h                 // Run orchestration, arena transitions
│   ├── ArenaPortal.h                // Teleportation actor, hidden until arena clear
│   ├── AFSSpawnZone.h               // Enemy spawn zone
│   ├── FSEnemy.h / FSEnemy_*.h      // Enemy base + archetypes
│   └── AnimNotifyState_*.h          // Gameplay timing via animation notifies
└── Private/                         // Implementations
```

---

## Status

**In Progress:**
- Death screen + run reset
- Reward chest on arena clear
- Upgrade screen at milestone level-up

**Planned:**
- Modular weapon craft (Blade + Handle + Gem)
- More types of ennemies
- Boss / Elite ennemies

---

## Contact

**Andreas Chatti**

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Andreas--Chatti-0A66C2?logo=linkedin)](https://www.linkedin.com/in/andréas-chatti/)
[![Email](https://img.shields.io/badge/Email-andreaschatti%40hotmail.com-EA4335?logo=gmail)](mailto:andreaschatti@hotmail.com)
[![GitHub](https://img.shields.io/badge/GitHub-Andreas--Chatti-181717?logo=github)](https://github.com/Andreas-Chatti)
