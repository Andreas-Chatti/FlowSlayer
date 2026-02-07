# Flow Slayer

A **3D action roguelite** built with **Unreal Engine 5.4** and **C++**, featuring a stylish combo-based combat system inspired by Hades, Devil May Cry, Dead Cells, and Furi.

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.4.4-blue?logo=unrealengine)
![C++](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus)
![Status](https://img.shields.io/badge/Status-In%20Development-yellow)
<a href="https://drive.google.com/drive/folders/13BXUaIHeYjbny9D94_BoQ9VAFmhFtopz?usp=sharing" target="_blank">
  <img src="https://img.shields.io/badge/Dev%20Videos-Google%20Drive-4285F4?logo=googledrive&logoColor=white" alt="Dev Videos">
</a>

---

## About The Project

Flow Slayer is a skill-based action game where players fight through hordes of enemies using a deep combo system. The core gameplay revolves around maintaining **Flow/Momentum** — the longer you keep your combo going, the more powerful you become.

**Key Design Goals:**
- Fast-paced, stylish combat with 17+ chainable combo attacks
- Air combat system with launchers, juggles, and aerial slams
- Lock-on targeting with dynamic camera behavior
- Pure skill-based progression — no stat grinding, just player mastery

---

## Technical Highlights

### Combat System
The combat system is built around a modular `FSCombatComponent` that handles:

- **Combo Chaining** — Animation-driven combo windows allow seamless attack transitions
- **Motion Warping** — Dynamic target tracking during attacks for satisfying hit connection
- **Hitstop** — Frame-perfect time dilation on hit for impactful feedback
- **Knockback Physics** — Directional knockback with horizontal and vertical force vectors
- **17+ Attack Types** — Ground combos, dash attacks, launchers, and aerial moves

### Air Combat
- **Launcher Attacks** — Send enemies airborne with trajectory peak detection
- **Air Stall System** — Reduced gravity during aerial combos via animation notifies
- **Mid-Air Freezing** — Enemies freeze briefly at apex, creating combo windows
- **Aerial Slams** — Multiple slam variations with ground impact

### Weapon System
- Sweep-based collision detection prevents fast attacks from phasing through targets
- Per-attack hit tracking to prevent duplicate damage
- Niagara-based sword trails and hit VFX

### Lock-On System
- Sphere-based target detection with nearest-priority targeting
- Distance-aware camera offset (pitch/yaw adjustments based on target range)
- Smooth camera interpolation with dead-target auto-disengage
- Input-based target switching

### Enemy AI
Two enemy archetypes implemented:
- **Grunt** — Melee combatant with sweep-based hitbox detection
- **Runner** — Ranged enemy with projectile attacks

### Spawn System
- Zone-based enemy spawning with ground detection via line traces
- Cooldown management and random transform generation

---

## Architecture

```
Source/FlowSlayer/
├── Public/
│   ├── FSCombatComponent.h      // Core combat logic & combo system
│   ├── FSWeapon.h               // Weapon hitbox & trail management
│   ├── FSLockOnComponent.h      // Target lock-on system
│   ├── FSEnemy.h                // Base enemy class
│   ├── FSEnemy_Grunt.h          // Melee enemy type
│   ├── FSEnemy_Runner.h         // Ranged enemy type
│   ├── FSEnemyAIController.h    // Enemy AI behavior
│   ├── FSProjectile.h           // Projectile system
│   ├── AFSSpawnZone.h           // Enemy spawn management
│   ├── FSDamageable.h           // Damage interface
│   ├── FSFocusable.h            // Lock-on target interface
│   └── AnimNotifyState_*.h      // Animation notify classes
├── Private/
│   └── [Implementation files]
└── FlowSlayerCharacter.h/cpp    // Player character
```

### Key Interfaces
- **IFSDamageable** — Health, damage reception, death state
- **IFSFocusable** — Lock-on indicator and health bar widget control

### Animation Notify System
Gameplay timing is controlled through custom `UAnimNotifyState` classes:
- `ComboWindow` — Input buffering for combo chains
- `Hitbox` — Weapon collision activation
- `AirStall` — Gravity modification during air attacks
- `AnimCancelWindow` — Action canceling opportunities

---

## Tech Stack

- **Engine:** Unreal Engine 5.4.4
- **Language:** C++ (gameplay systems) + Blueprints (UI/VFX)
- **Input:** Enhanced Input System
- **Animation:** Motion Warping, Animation Notifies, Montages
- **VFX:** Niagara Particle System
- **AI:** AIController with pathfinding

---

## Code Standards

This project follows Unreal Engine coding conventions:
- Bracket initialization for all variables
- PascalCase for functions and member variables
- `b` prefix for boolean members
- Comprehensive documentation comments
- Interface-based design for extensibility

---

## Current Status

**Implemented:**
- Full combo system with 17+ attack types
- Air combat with launchers and aerial combos
- Lock-on targeting system
- Two enemy types (melee & ranged)
- Projectile system
- Spawn zone system
- Player HUD

**In Progress:**
- Flow/Momentum meter system
- Additional enemy types
- Procedural dungeon generation

---

## Contact

**Andreas Chatti**

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Andreas--Chatti-0A66C2?logo=linkedin)](https://www.linkedin.com/in/andréas-chatti/)
[![Email](https://img.shields.io/badge/Email-andreaschatti%40hotmail.com-EA4335?logo=gmail)](mailto:andreaschatti@hotmail.com)
[![GitHub](https://img.shields.io/badge/GitHub-Andreas--Chatti-181717?logo=github)](https://github.com/Andreas-Chatti)
