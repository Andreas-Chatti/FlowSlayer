# FLOW SLAYER - DOCUMENT DE CONCEPTION COMPLET 📋

---

## TABLE DES MATIÈRES

1. [Vision & Piliers du Jeu](#vision)
2. [Comparaisons & Inspirations](#inspirations)
3. [Structure du Jeu](#structure)
4. [Systèmes de Gameplay](#gameplay)
5. [Progression & Méta](#progression)
6. [Contenu](#contenu)
7. [Spécifications Techniques](#technique)
8. [Roadmap de Développement](#roadmap)
9. [Estimations & Scope](#estimations)

---

<a name="vision"></a>
# 1. VISION & PILIERS DU JEU

## Concept Core

**Flow Slayer** est un action roguelite en 3D où tu massacres des hordes d'ennemis dans des mini-donjons générés aléatoirement. Le système de **Flow/Momentum** est au cœur du gameplay : plus tu maintiens ton flow en attaquant sans arrêt, plus tu deviens rapide et puissant. Craft ton arme avec des parts trouvées, choisis ton personnage, et vise le run parfait.

---

## Les 10 Piliers Non-Négociables

1. **Petits mobs + Élites + Boss** (pas que des boss comme Furi)
2. **Dopamine constante** (XP, gold, loot fréquent)
3. **RNG + Builds variés** (système de parts pour crafter l'arme)
4. **Runs courts** (30-40 min MAX)
5. **Fun > Tryhard** (accessible, pas souls-like frustrant)
6. **Plusieurs persos** (4-6 avec playstyles différents)
7. **Combos stylés** mais accessibles
8. **Flow/Momentum system** (ressource principale unique)
9. **Zéro narrative** (pure gameplay, pas de dialogues/cinématiques)
10.**Pas de stats permanentes** (100% skill-based)

---

## Expérience Cible

**Le joueur doit ressentir :**
- Puissance immédiate (massacre des hordes)
- Progression constante (loot, level up, flow qui monte)
- Flow state hypnotique (quand tout s'enchaîne parfaitement)
- Variété (chaque run = build différent)
- Envie de rejouer ("Just one more run")

**Pas de :**
- Grind frustrant
- Downtime ennuyeux
- Complexity creep (trop de systèmes)
- Punition excessive (mort = restart rapide)

---

<a name="inspirations"></a>
# 2. COMPARAISONS & INSPIRATIONS

## Jeux de Référence

### Hades (Supergiant Games)

**Ce qu'on prend :**
- Structure roguelite (runs courts, rejouabilité)
- Choix d'upgrades entre étages
- Plusieurs persos avec playstyles différents
- Polish du combat

**Ce qu'on ne prend PAS :**
- Narrative complexe
- Rooms avec peu d'ennemis
- Durée de run (60+ min)

---

### Dead Cells (Motion Twin)

**Ce qu'on prend :**
- Combat fluide et rapide
- Système de parts/blueprints
- Scroll shops entre zones
- Skill-based difficulty

**Ce qu'on ne prend PAS :**
- 2D platforming
- Méta-progression via cells
- Backtracking

---

### Furi (The Game Bakers)

**Ce qu'on prend :**
- Caméra style (third-person, cinématique)
- Focus sur boss fights épiques
- Combos stylés
- Polish visuel/audio

**Ce qu'on ne prend PAS :**
- SEULEMENT des boss (on a des mobs)
- Linéaire pur (on a génération procédurale)
- Walk sequences entre boss

---

### Vampire Survivors (poncube)

**Ce qu'on prend :**
- Dopamine constante (kills, XP)
- Hordes d'ennemis
- Level up fréquents avec choix

**Ce qu'on ne prend PAS :**
- Auto-aim/auto-fire
- Vue top-down statique
- Pas de skill mécanique requis

---

### Devil May Cry (Capcom)

**Ce qu'on prend :**
- Combos stylés
- Rank system (D → SSS)
- Flow dans le combat
- Satisfaction des enchaînements

**Ce qu'on ne prend PAS :**
- Campagne linéaire 15h
- Exploration complexe
- Narrative heavy

---

## Notre Différenciation Unique

**Flow Slayer se distingue par :**

1. **Flow = Ressource + Gameplay Modifier**
   - Pas juste cosmétique
   - Affecte vitesse, bouclier, combos
   - Montée/descente constante = tension

2. **Hybride Arènes + Mini-Donjons**
   - Pas de maze complexe
   - Layout linéaire avec choix
   - Combat constant (pas de downtime)

3. **Craft modulaire d'arme**
   - Lame + Handle + Gems
   - Combinaisons infinies
   - RNG contrôlée (choix de 3)

4. **Sessions ultra-courtes**
   - 30-40 min (vs 60-90 min des autres roguelites)
   - Parfait pour "one quick run"

---

<a name="structure"></a>
# 3. STRUCTURE DU JEU

## Format de Run

### Durée Totale : 30-40 minutes

```
RUN COMPLET (4 ÉTAGES)

Étage 1 : Introduction (5-7 min)
├─ Salle 1 : Vague de mobs (2 min)
├─ Salle 2 : Coffre + Choix upgrade
├─ Salle 3 : Vague + Elite (3 min)
└─ Boss 1 (2-3 min)

Étage 2 : Escalade (6-8 min)
├─ Salle 1 : Vague
├─ Salle 2 : Choix (gauche = facile, droite = dur + reward)
├─ Salle 3 : Vague + Elite
├─ Salle 4 : Vague intense
└─ Boss 2 (3-4 min)

Étage 3 : Challenge (7-9 min)
├─ Plus de salles
├─ Plus d'élites
├─ Density maximale
└─ Boss 3 (4-5 min)

Étage 4 : Final (8-10 min)
├─ Gauntlet
├─ Élites multiples
└─ Boss Final (5-6 min, multi-phases)
```

---

## Layout des Étages (Génération Procédurale SIMPLE)

### Principe : Linéaire avec Embranchements

**Pas de maze complexe.** Chemins clairs avec choix risk/reward.

```
Exemple de layout :

START → [Combat] → [Choice] → [Combat] → [Elite] → [BOSS]
                     ↓
                  [Secret]
                  (Combat dur + Rare reward)

Choice Point :
├─ Gauche : Combat facile → Common/Rare loot
└─ Droite : Combat dur → Rare/Epic loot
```

### Règles de Génération

1. **Pas de pathfinding complexe** (juste A → B → C)
2. **1-2 choix par étage** (embranchements)
3. **Secrets optionnels** (salles cachées, reward élevé)
4. **Pas de backtracking** (portes se ferment)
5. **Toujours un chemin clair vers le boss**

**Difficulté Technique :** ⭐⭐ (Faible)

---

## Types de Salles

### Salle de Combat (80% des salles)
- Porte se ferme
- 3-5 vagues d'ennemis
- Durée : 2-3 minutes
- Loot : XP/Gold constant, parts occasionnelles

### Salle de Repos (10%)
- Pas d'ennemis
- Coffre garanti
- Choix d'upgrade (heal, new part, reroll)

### Salle Secrète (5%)
- Difficile à trouver (mur cassable, switch caché)
- Combat très dur
- Reward : Epic/Legendary garanti

### Boss Room (5%)
- Arène circulaire large
- Boss avec phases
- Reward : Legendary part + choix d'upgrade majeur

---

<a name="gameplay"></a>
# 4. SYSTÈMES DE GAMEPLAY

## A. Combat System

### Contrôles (PC Keyboard + Mouse)

```
WASD        : Mouvement
Mouse       : Orientation caméra (légère)
Left Click  : Attaque légère
Hold Click  : Attaque lourde (chargée)
Shift       : Dash (i-frames)
Space       : Capacité spéciale (selon perso)
E           : Interact (coffres, portes)
```

---

### Combos Basiques

**Enchaînement Light Attacks :**
```
Clic → Clic → Clic → Finisher (auto)
  ↓      ↓      ↓        ↓
 Hit1   Hit2   Hit3   Knockback + AOE

Durée totale : ~1.5 sec
Dégâts : 10 + 15 + 20 + 30 = 75
Flow gain : 5 + 5 + 7 + 10 = 27
```

**Heavy Attack (chargé) :**
```
Hold Click (1 sec) → Release
         ↓
    Gros hit unique

Dégâts : 100
Flow gain : 20
Knockback : Large
```

**Combos Avancés (Flow > 50) :**
```
Light × 3 → Heavy → Dash Cancel → Light × 3
    ↓         ↓          ↓            ↓
  Base      Finisher   Reset       Repeat

Infinite combo possible SI tu maintiens flow
```

---

### Animation Canceling (Advanced)

**Dash Cancel :**
- Pendant n'importe quelle attaque
- Press Shift = Cancel animation + dash
- **Permet :** Esquive urgente ou extend combo

**Flow Cancel (Flow > 75) :**
- Skip recovery frames automatiquement
- Attaques enchaînent ultra vite
- **Sensation :** "God mode"

---

## B. Flow System (MÉCANIQUE CENTRALE)

### Définition

**Flow = Momentum de combat** représenté par une barre (0-100)

Plus ton flow est élevé :
- Plus tu es rapide
- Plus tu as de défense (bouclier)
- Plus tu débloques de combos

---

### Mécaniques Détaillées

#### Flow Gain
```
Source                    | Flow +
--------------------------|--------
Hit léger                 | +5
Hit lourd                 | +10
Hit combo finisher        | +15
Kill ennemi basic         | +3
Kill élite                | +20
Perfect dodge (last frame)| +10
Parry (Knight uniquement) | +15
```

#### Flow Decay
```
Condition                | Flow -
-------------------------|----------
3 sec sans hit           | -10/sec
Se faire toucher (no shield) | -30
Se faire toucher (avec shield)| -15
Dash excessif (spam)     | -5/dash après 3e
```

---

### Flow Tiers & Effets

```
FLOW 0-25 : NORMAL
├─ Vitesse : 100% (base)
├─ Dégâts : 100%
├─ Bouclier : Aucun
└─ Combos : Basiques uniquement

FLOW 26-50 : WARMING UP
├─ Vitesse : 120%
├─ Dégâts : 110%
├─ Bouclier : Aucun
├─ VFX : Légères traînées sur arme
└─ Combos : Basiques

FLOW 51-75 : FLOW STATE
├─ Vitesse : 140%
├─ Dégâts : 125%
├─ Bouclier : Actif (absorbe 1 hit)
├─ VFX : Traînées prononcées, aura
├─ Combos : Avancés débloqués
└─ Son : Musique layer additionnel

FLOW 76-100 : ZEN MODE
├─ Vitesse : 160%
├─ Dégâts : 150%
├─ Bouclier : Actif (absorbe 2 hits)
├─ VFX : Aura intense, distorsion écran
├─ Combos : Dash cancels gratuits
├─ Son : Musique intense + heartbeat
└─ Effet : Légère slow-mo ennemis (95% vitesse)
```

---

## C. HP & Damage System (HYBRIDE)

### Ressources du Joueur

```
HP (Barre Rouge) : 100 base
├─ Prendre hit sans bouclier : -Dégâts
├─ Heal : Rare (coffres, level up)
└─ Mort si HP = 0

Flow (Barre Bleue) : 0-100
├─ Génère bouclier si > 50
├─ Reset à 0 si hit reçu
└─ Pas de mort directe

Bouclier (Overlay bleu sur HP) :
├─ Actif si Flow 51-75 : Absorbe 1 hit
├─ Actif si Flow 76-100 : Absorbe 2 hits
└─ Disparaît si flow < 50
```

### Exemple de Combat

```
Scenario :

1. Tu commences : HP 100, Flow 0
2. Tu attaques 10x : Flow monte à 60
   → Bouclier activé (1 hit absorption)
3. Ennemi te frappe : Bouclier absorbe
   → Flow reset à 0
   → HP reste 100
4. Ennemi te frappe ENCORE (pas de bouclier) :
   → HP = 100 - 20 = 80
   → Flow reste 0
5. Tu attaques 15x : Flow remonte à 80
   → Bouclier réactivé (2 hits)
6. Boss te hit : 1er hit absorbé
   → Bouclier passe à 1 hit
   → Flow = 60
7. Boss te hit ENCORE : 2e hit absorbé
   → Bouclier disparu
   → Flow = 40 (< 50, plus de bouclier)
8. Boss te hit UNE 3E FOIS :
   → HP = 80 - 25 = 55
   → Flow = 20

Strategy : MAINTENIR FLOW = SURVIE
```

---

## D. Level Up & XP System

### Gain d'XP

```
Source            | XP +
------------------|------
Grunt (basic)     | 10
Runner (fast)     | 15
Tank (tanky)      | 20
Elite             | 100
Boss              | 500
```

### Courbe de Level Up

```
Level | XP Required | XP Cumul
------|-------------|----------
1     | 100         | 100
2     | 150         | 250
3     | 200         | 450
4     | 300         | 750
5     | 400         | 1150
...
10    | 1000        | ~6000
```

**Estimation :** Level 8-10 par run complet

---

### Choix au Level Up (Toujours 3 options)

```
Categorie 1 : PARTS (50% chance)
├─ New Part (Common/Rare/Epic selon étage)
├─ Upgrade Part existante (Tier +1)
└─ Reroll (refresh les 3 choix, 1x par étage)

Categorie 2 : STATS (30% chance)
├─ +20 HP Max
├─ +10 Flow gain per hit
├─ +10% vitesse base
└─ +15% dégâts

Categorie 3 : ABILITIES (20% chance)
├─ +1 Dash charge
├─ Heal 50 HP
├─ Flow burst (consomme 50 flow = AOE explosion)
└─ Magnet (loot attire automatiquement)
```

---

## E. System de Parts (Craft d'Arme)

### Structure : 3 Slots

**1. LAME** (détermine moveset)
**2. HANDLE** (détermine stats secondaires)
**3. GEM** (effets spéciaux)

---

### LAMES (Movesets)

#### Straight Sword
```
Style       : Équilibré
Vitesse     : Moyenne (1.0x)
Dégâts/hit  : Moyen (10 base)
Portée      : Moyenne
Combo       : 4 hits + finisher
Special     : Thrust (dash forward + hit)
```

#### Great Axe
```
Style       : Lent mais puissant
Vitesse     : Lente (0.7x)
Dégâts/hit  : Élevé (18 base)
Portée      : Large (AOE)
Combo       : 3 hits lourds + spin finisher
Special     : Ground slam (AOE + stun)
```

#### Dual Daggers
```
Style       : Rapide, DPS
Vitesse     : Rapide (1.4x)
Dégâts/hit  : Faible (6 base)
Portée      : Courte
Combo       : 6 hits rapides + flurry finisher
Special     : Dash strike (multi-hit en ligne)
```

#### Spear
```
Style       : Portée, poke
Vitesse     : Moyenne (1.1x)
Dégâts/hit  : Moyen (11 base)
Portée      : Longue
Combo       : 3 pokes + sweep finisher
Special     : Charge (dash + pierce)
```

#### Gauntlets
```
Style       : Corps-à-corps, combos infinis
Vitesse     : Très rapide (1.5x)
Dégâts/hit  : Très faible (5 base)
Portée      : Très courte
Combo       : Infinite (tant que flow > 0)
Special     : Uppercut (lance ennemi)
```

---

### HANDLES (Stats Secondaires)

#### Wooden Handle
```
Dégâts   : ×1.0
Vitesse  : ×1.0
Special  : Aucun
Rarity   : Common
```

#### Iron Handle
```
Dégâts   : ×1.3
Vitesse  : ×0.9
Special  : Aucun
Rarity   : Common/Rare
```

#### Crystal Handle
```
Dégâts   : ×0.9
Vitesse  : ×1.3
Special  : Aucun
Rarity   : Rare
```

#### Bone Handle
```
Dégâts   : ×1.0
Vitesse  : ×1.0
Special  : Lifesteal 5% des dégâts
Rarity   : Rare/Epic
```

#### Ethereal Handle
```
Dégâts   : ×0.8
Vitesse  : ×1.2
Special  : Phase through ennemis (pas de collision)
Rarity   : Epic
```

---

### GEMS (Effets Spéciaux)

#### Fire Gem
```
Effect     : Burn (5 dégâts/sec × 3 sec)
Proc       : 100% on hit
Visuel     : Arme en feu, trails rouges
Rarity     : Common → Legendary
Upgrade    : Epic = Explosion on kill (AOE 5m)
             Legendary = Burn stacks (max 3 stacks)
```

#### Ice Gem
```
Effect     : Slow 30% vitesse × 2 sec
Proc       : 100% on hit
Visuel     : Arme givrée, trails bleus
Rarity     : Common → Legendary
Upgrade    : Epic = Freeze on 3rd hit (2 sec)
             Legendary = Shatter frozen = AOE damage
```

#### Lightning Gem
```
Effect     : Chain lightning (2 cibles, 50% dégâts)
Proc       : 30% on hit
Visuel     : Arcs électriques
Rarity     : Rare → Legendary
Upgrade    : Epic = Chain 4 cibles
             Legendary = Chain 6 cibles + stun
```

#### Poison Gem
```
Effect     : Poison stack (3 dégâts/sec)
Proc       : 100% on hit (stack max 5)
Visuel     : Nuage vert, trails verts
Rarity     : Rare → Legendary
Upgrade    : Epic = Stack max 10
             Legendary = Poison explosion on kill
```

#### Void Gem
```
Effect     : Drain 5 Flow from enemy → give to you
Proc       : 50% on hit (si ennemi a flow)
Visuel     : Traînées noires, absorption
Rarity     : Epic → Legendary
Upgrade    : Legendary = Heal 1 HP per 10 flow drained
```

#### Holy Gem
```
Effect     : AOE heal 5 HP (rayon 5m)
Proc       : On crit (10% base crit chance)
Visuel     : Éclat doré
Rarity     : Epic → Legendary
Upgrade    : Legendary = Revive 1x par run (50% HP)
```

---

### Système de Rareté

```
COMMON (Blanc)
├─ Stats : 100% base
├─ Effet : Basique
├─ Sockets : 1 gem max
└─ Drop rate : 50%

RARE (Bleu)
├─ Stats : 130% base
├─ Effet : +1 bonus mineur
├─ Sockets : 1 gem max
└─ Drop rate : 30%

EPIC (Violet)
├─ Stats : 160% base
├─ Effet : +1 bonus majeur
├─ Sockets : 2 gems
└─ Drop rate : 15%

LEGENDARY (Orange)
├─ Stats : 200% base
├─ Effet : +2 bonus majeurs + effet unique
├─ Sockets : 3 gems
└─ Drop rate : 5%
```

---

### Exemples de Builds

#### Build "Speed Demon"
```
Lame   : Dual Daggers (Epic)
Handle : Crystal (Legendary)
Gem 1  : Lightning (Epic)
Gem 2  : Void (Rare)

Stats resultantes :
├─ Vitesse : 1.4 × 1.3 × 1.6 (Epic) = 2.9x base
├─ Dégâts : 6 × 0.9 × 1.6 = 8.6 per hit
├─ Hits/sec : ~5 hits
└─ DPS : ~43 + lightning chains

Playstyle : Zerg rapide, jamais arrêter, flow infini
```

#### Build "Glass Cannon"
```
Lame   : Great Axe (Legendary)
Handle : Iron (Epic)
Gem 1  : Fire (Legendary)
Gem 2  : Poison (Epic)
Gem 3  : Holy (Legendary)

Stats :
├─ Vitesse : 0.7 × 0.9 × 1.6 = 1.0x (normal)
├─ Dégâts : 18 × 1.3 × 2.0 = 46.8 per hit
└─ Effet : Burn + Poison + Heal on crit

Playstyle : Slow methodical, gros hits, heal passive
```

#### Build "Tank"
```
Lame   : Sword + Shield (Epic)
Handle : Bone (Legendary)
Gem 1  : Holy (Epic)
Gem 2  : Ice (Rare)

Stats :
├─ Block : Actif (réduit 50% dégâts)
├─ Lifesteal : 5%
├─ Heal on crit
└─ Slow ennemis

Playstyle : Parry-focused, sustain, long fights
```

---

## F. Enemies & AI

### Tier 1 : Petits Mobs (80% des spawns)

#### Grunt
```
HP         : 30
Vitesse    : Lente (0.8x player)
Dégâts     : 10
AI         : Suit joueur, attaque mêlée
XP         : 10
Loot       : Occasional common part
Spawn rate : Très élevé (groups de 5-10)
```

#### Runner
```
HP         : 20
Vitesse    : Rapide (1.3x player)
Dégâts     : 8
AI         : Sprint vers joueur, attaque hit-and-run
XP         : 15
Loot       : Occasional common part
Spawn rate : Élevé (groups de 3-5)
```

#### Shooter
```
HP         : 15
Vitesse    : Moyenne
Dégâts     : 12 (projectile)
AI         : Garde distance, tire toutes les 2 sec
XP         : 15
Loot       : Occasional common part
Spawn rate : Moyen (2-3 par vague)
Special    : PRIORITÉ à kill (snipe player)
```

---

### Tier 2 : Ennemis Normaux (15% des spawns)

#### Tank
```
HP         : 100
Vitesse    : Très lente (0.6x)
Dégâts     : 25
AI         : Marche vers joueur, attaque lourde (wind-up 1 sec)
XP         : 20
Loot       : Common/Rare part
Spawn rate : Faible (1-2 par vague)
Special    : Super armor (pas de stagger)
```

#### Berserker
```
HP         : 50
Vitesse    : Rapide (1.2x)
Dégâts     : 15
AI         : Agressif, combo 3 hits
XP         : 20
Loot       : Common/Rare part
Spawn rate : Faible
Special    : Enrage si < 30% HP (vitesse × 1.5)
```

#### Shielder
```
HP         : 60
Vitesse    : Moyenne
Dégâts     : 12
AI         : Bloque attaques frontales (shield up)
XP         : 20
Loot       : Rare part garantie
Spawn rate : Rare (1 par vague)
Special    : Doit être flanked ou parried
```

---

### Tier 3 : Élites (4% des spawns)

**Définition :** Versions buffées d'ennemis normaux

```
Elite Grunt "Brute"
├─ HP : 150 (5x normal)
├─ Vitesse : Normale
├─ Dégâts : 30 (3x normal)
├─ AI : Suit + Ground slam AOE
├─ XP : 100
├─ Loot : Epic part garanti
└─ Aura : Buff nearby allies (+20% dégâts)

Elite Runner "Assassin"
├─ HP : 80
├─ Vitesse : 1.8x
├─ Dégâts : 20
├─ AI : Teleport + backstab combo
├─ XP : 100
├─ Loot : Epic part garanti
└─ Special : Invisibilité (3 sec cooldown)

Elite Tank "Juggernaut"
├─ HP : 300
├─ Vitesse : 0.8x
├─ Dégâts : 40
├─ AI : Charge + Slam combo
├─ XP : 150
├─ Loot : Epic/Legendary part
└─ Special : Regen 5 HP/sec si pas hit 5 sec

Elite Shooter "Sniper"
├─ HP : 60
├─ Vitesse : Moyenne
├─ Dégâts : 50 (one-shot si low HP)
├─ AI : Garde longue distance, laser aim
├─ XP : 100
├─ Loot : Epic part garanti
└─ Special : Charge shot (3 sec wind-up)
```

---

### Tier 4 : Boss (1% - Fin d'étage)

#### Boss 1 : "The Sentinel" (Étage 1)

**Role :** Tutoriel déguisé

**Stats :**
- HP : 500
- Phases : 2

**Phase 1 (100-50% HP) :**
```
Pattern 1 : Combo mêlée 3 hits (lent, télégraphié)
Pattern 2 : Ground slam AOE (dodge required)
Pattern 3 : Projectile lent (easy parry)

Loop : Pattern 1 → 2 → 1 → 3 → Repeat
Durée : ~90 sec
```

**Phase 2 (50-0% HP) :**
```
Pattern 1 : Combo 5 hits (plus rapide)
Pattern 2 : Double ground slam
Pattern 3 : Projectile barrage (3 projectiles)
Pattern 4 : Summon 3 Grunts

Loop : 1 → 2 → 4 → 3 → Repeat
Durée : ~90 sec
```

**Loot :** Legendary part garanti

---

#### Boss 2 : "The Berserker" (Étage 2)

**Role :** Tester le maintien de flow

**Stats :**
- HP : 800
- Phases : 2

**Mechanic Unique :** Plus le joueur a de flow, plus le boss est agressif

**Phase 1 :**
```
Si player flow < 50 :
├─ Vitesse normale
├─ Patterns basiques
└─ Pauses entre attaques

Si player flow > 50 :
├─ Vitesse +50%
├─ Patterns agressifs
└─ Pas de pauses

Strategy : Maintenir flow moyen (pas trop haut)
```

**Phase 2 :**
```
Boss entre en permanent rage
├─ Vitesse × 1.5
├─ Dégâts × 1.3
├─ Attaques non-stop
└─ Player DOIT avoir flow élevé pour suivre

Strategy : All-in, max flow, dodge parfait
```

---

#### Boss 3 : "The Phantom" (Étage 3)

**Role :** Tester précision et parries

**Stats :**
- HP : 1200
- Phases : 3

**Mechanic :** Téléportation constante

**Phase 1 :**
```
Teleport toutes les 3 sec
├─ Apparaît derrière joueur
├─ 1 hit rapide
└─ Disparaît

Counter : Perfect dodge (+10 flow)
```

**Phase 2 :**
```
Teleport toutes les 2 sec
├─ Combo 3 hits
├─ Certaines attaques MUST parry (sinon one-shot)
└─ Si parry 3x → Boss stun 5 sec

Counter : Learn parry timing
```

**Phase 3 :**
```
Summon 2 clones
├─ Real boss + 2 clones (1 HP each)
├─ Tous téléportent
├─ Trouver le vrai
└─ Kill clones d'abord = boss enragé

Strategy : Ignore clones, focus real boss
```

---

#### Boss 4 : "The Architect" (Final Boss)

**Role :** Synthèse de tous les skills

**Stats :**
- HP : 2000
- Phases : 4

**Phase 1 (100-75%) :**
```
Mix des patterns des 3 boss précédents
├─ Sentinel combos
├─ Berserker aggression
└─ Phantom teleports

Durée : 2 min
```

**Phase 2 (75-50%) :**
```
Arena change : Platformes mouvantes
├─ Chutes = dégâts
├─ Boss summon adds constants
└─ Projectiles from all sides

Durée : 2 min
```

**Phase 3 (50-25%) :**
```
Boss enrage permanent
├─ Vitesse × 2
├─ Dégâts × 1.5
├─ New pattern : Laser beam (must dodge)
└─ Gravity shift (screen rotates)

Durée : 2 min
```

**Phase 4 (25-0%) :**
```
DPS check
├─ Boss invincible 50% du temps
├─ Windows d'attaque courtes (3 sec)
├─ Player MUST have flow 100 pour DPS suffisant
└─ Sinon : Enrage wipe

Durée : 2-3 min
```

**Loot :** Choix de 3 Legendary parts + Currency bonus

---

<a name="progression"></a>
# 5. PROGRESSION & MÉTA

## Progression In-Run (Temporaire)

**Reset à chaque run.**

```
Level Ups (temporaires)
├─ Stats boosts
├─ Parts trouvées
└─ Abilities unlockées

Gold (temporaire)
├─ Acheter au shop (si implémenté)
└─ Reset après run
```

---

## Méta-Progression (Permanente)

**PAS de stats permanentes** (reste skill-based)

### Unlocks via Achievements

```
PERSONNAGES :
├─ The Slayer : Débloqué dès le début
├─ The Rogue : Finir 1 run (n'importe quel perso)
├─ The Berserker : Atteindre flow 100 pendant un run
├─ The Monk : Maintenir flow > 50 pendant 5 min
├─ The Knight : Parry 50 attaques (cumulatif)
└─ The Reaper : Finir un run sans se faire toucher

PARTS (ajoutées au pool de loot) :
├─ Void Gem : Drain 1000 flow total (cumulatif)
├─ Holy Gem : Heal 500 HP total (cumulatif)
├─ Ethereal Handle : Phase through 100 ennemis
└─ Legendary variants : Conditions difficiles

COSMÉTIQUES (optionnel) :
├─ Skins d'armes
├─ Auras de flow personnalisées
├─ Traînées de dash custom
└─ Victory poses
```

### Currency Cosmétique (Optionnel)

**"Shards"** (persistent currency)

```
Gain :
├─ Boss kill : 50 shards
├─ Run complete : 100 shards
├─ Achievements : 20-100 shards

Dépense :
└─ Cosmétiques uniquement (0 impact gameplay)
```

---

<a name="contenu"></a>
# 6. CONTENU - SCOPE MINIMAL

## Scope Confirmé (Version 1.0)

```
PERSONNAGES JOUABLES : 4
├─ The Slayer (balanced)
├─ The Rogue (speed)
├─ The Berserker (damage)
└─ The Monk (flow master)

(+2 à débloquer : Knight, Reaper)
```

```
TYPES D'ENNEMIS : 10
├─ Tier 1 (Petits mobs) :
│   ├─ Grunt
│   ├─ Runner
│   ├─ Shooter
│   └─ Exploder (nouveau)
├─ Tier 2 (Normaux) :
│   ├─ Tank
│   ├─ Berserker
│   ├─ Shielder
│   └─ Mage (nouveau)
└─ Tier 3 (Élites) :
    ├─ Elite Grunt (Brute)
    └─ Elite Runner (Assassin)

(+2 élites à ajouter : Elite Tank, Elite Shooter)
```

```
BOSS : 4
├─ Boss 1 : The Sentinel (Étage 1)
├─ Boss 2 : The Berserker (Étage 2)
├─ Boss 3 : The Phantom (Étage 3)
└─ Boss 4 : The Architect (Étage 4 - Final)
```

```
LAMES : 5
├─ Straight Sword
├─ Great Axe
├─ Dual Daggers
├─ Spear
└─ Gauntlets
```

```
HANDLES : 5
├─ Wooden
├─ Iron
├─ Crystal
├─ Bone
└─ Ethereal
```

```
GEMS : 6
├─ Fire
├─ Ice
├─ Lightning
├─ Poison
├─ Void
└─ Holy
```

**Total Combinaisons : 5 × 5 × 6 = 150 builds de base**
**Avec rarités : 150 × 4 = 600 variations**

---

## Extension Possible (Post-Launch)

```
DLC / Updates :
├─ +2 Personnages
├─ +5 Étages (Daily/Weekly dungeons)
├─ +10 Ennemis nouveaux
├─ +2 Boss
├─ +5 Lames
├─ +3 Gems
├─ New mechanics (ex: Co-op mode)
└─ Cosmetics shop
```

---

<a name="technique"></a>
# 7. SPÉCIFICATIONS TECHNIQUES

## Choix Techniques Validés

```
ENGINE        : Unreal Engine 5.3/5.4
LANGAGE       : C++ (gameplay) + Blueprints (UI, VFX)
DIMENSION     : 3D
VUE           : Third-Person (style Furi)
PLATEFORME    : PC (KB+M) [Potentiel console plus tard]
GRAPHIQUES    : Stylisé (pas photoréaliste)
FRAME RATE    : Target 60 FPS
RÉSOLUTION    : 1080p minimum, 4K supported
```

---

## Architecture du Code

### Hiérarchie des Classes Principales

```
ACharacter (Unreal)
├─ AFSCharacter (Player)
│   ├─ Combat logic
│   ├─ Flow system
│   ├─ Input handling
│   └─ Components :
│       ├─ UFSCombatComponent
│       ├─ UFSFlowComponent
│       └─ UFSInventoryComponent
│
└─ AFSEnemy (Base enemy)
    ├─ AI behavior
    ├─ HP/Damage
    └─ Subclasses :
        ├─ AGrunt
        ├─ ARunner
        ├─ ATank
        └─ AFSBoss
            ├─ ASentinelBoss
            ├─ ABerserkerBoss
            ├─ APhantomBoss
            └─ AArchitectBoss

AActor (Unreal)
├─ AFSWeapon
│   ├─ Hitbox logic
│   ├─ VFX spawning
│   └─ Weapon stats
│
├─ AFSProjectile
│   └─ Bullet/arrow logic
│
├─ AWaveManager
│   ├─ Spawn logic
│   └─ Wave progression
│
└─ AFSDungeonGenerator
    └─ Procedural layout

UActorComponent (Unreal)
├─ UFSCombatComponent
│   ├─ Hitstop
│   ├─ Camera shake
│   └─ VFX/SFX triggering
│
├─ UFSFlowComponent
│   ├─ Flow gain/decay
│   ├─ Tier calculation
│   └─ Buff application
│
└─ UFSInventoryComponent
    ├─ Parts storage
    ├─ Weapon crafting
    └─ Stat calculation
```

---

## Assets Nécessaires

### 3D Models & Animations

**Source : Mixamo (gratuit)**

```
PERSONNAGES (1 base + 3 variants) :
├─ 1 Modèle humanoid rigged
└─ Animations :
    ├─ Locomotion : Idle, Walk, Run, Jump
    ├─ Combat : 10-15 attaques variées
    ├─ Réactions : Hit, Death, Stagger
    └─ Special : Dash, Victory pose

ENNEMIS (4-5 modèles de base) :
├─ Modèles humanoid/creature rigged
└─ Animations :
    ├─ Locomotion : Idle, Walk, Run
    ├─ Combat : 2-3 attaques chacun
    └─ Réactions : Hit, Death

ARMES (5 modèles) :
├─ Static meshes simples
└─ Pas d'animations (attachés au perso)
```

---

### VFX (Effets Visuels)

**Source : Unreal Starter Content + Niagara**

```
COMBAT :
├─ Slash trails (arme)
├─ Hit sparks
├─ Blood/impact particles
└─ Shockwaves (ground slam)

FLOW :
├─ Aura body (tiers 51-100)
├─ Weapon glow (couleur selon gem)
├─ Screen distortion (Zen mode)
└─ Speed lines (dash)

GEMS :
├─ Fire : Flammes, burn DOT
├─ Ice : Cristaux, freeze
├─ Lightning : Arcs électriques
├─ Poison : Nuage vert
├─ Void : Absorption noire
└─ Holy : Éclat doré

ENVIRONNEMENT :
├─ Portails (entrée/sortie salle)
├─ Coffre sparkle
└─ Level up burst
```

---

### SFX (Sons)

**Source : Freesound.org + Asset packs**

```
COMBAT :
├─ Whoosh (swing arme)
├─ Impact (hit ennemi)
├─ Parry/block
├─ Critical hit
└─ Death scream

PLAYER :
├─ Footsteps
├─ Dash
├─ Level up chime
└─ Hurt grunt

FLOW :
├─ Flow gain (subtle chime)
├─ Flow tier up (layer sonore additionnel)
├─ Zen mode trigger (heartbeat + music change)
└─ Flow lost (negative sound)

UI :
├─ Menu navigation
├─ Button click
├─ Inventory open/close
└─ Part equip
```

---

### Musique

**Source : Royalty-free ou composer**

```
AMBIANCE :
├─ Menu theme (calm)
├─ Dungeon ambient (tense)
└─ Shop/Respite (soft)

COMBAT :
├─ Combat layer 1 (base)
├─ Combat layer 2 (flow 51+)
├─ Combat layer 3 (flow 76+)
└─ Boss themes (4 unique tracks)

VICTORY :
└─ Victory jingle (short)
```

**Note :** Musique adaptive (layers) = crucial pour le flow system

---

## Performance Targets

```
FRAME RATE  : 60 FPS stable
ENEMIES MAX : 30-40 simultanés à l'écran
PARTICLES   : Modéré (pas de spam fou)
DRAW CALLS  : Optimisé (batching, LODs)
LOAD TIMES  : < 5 sec entre salles
```

---

<a name="roadmap"></a>
# 8. ROADMAP DE DÉVELOPPEMENT

## Phase 0 : Setup (CE SOIR - 1-2h) ✅

```
✅ Création projet Unreal
✅ Download assets Mixamo
✅ Setup structure dossiers
✅ Import perso + animations
✅ (Optionnel) Init Git
```

---

## Phase 1 : PROTOTYPE CORE (Semaine 1-4)

### Semaine 1 : Combat Basique + Ennemis

```
Jour 1 : Classe Character + Mouvement ✅
├─ FSCharacter créé ✅
├─ Caméra style DMC5 (fait partiellement, manque système de lock-on ennemi)
├─ Mouvement WASD + dash ✅
└─ Deliverable : Perso bouge ✅

Jour 2 : Première attaque + Hitbox
├─ Classe Weapon ✅
├─ Animation Montage (1 attaque) ✅
├─ Hitbox detection ✅
└─ Deliverable : Attaque fonctionne ✅

Jour 3 : Premiers ennemis
├─ Classe Enemy ✅ 
├─ AI simple (follow player) ✅
├─ 2 types (Grunt, Runner)
└─ Deliverable : Ennemis killable ✅

Jour 4 : Hitstop + VFX basiques
├─ CombatComponent
├─ Hitstop implementation
├─ Particules + Sons
└─ Deliverable : Hits feel satisfaisants

Jour 5 : Flow System v1
├─ Flow meter (0-100)
├─ Flow gain sur hit
├─ Vitesse scaling
├─ UI basique
└─ Deliverable : Flow fonctionnel

TEST GO/NO-GO #1 : Flow feel bon ?
```

**Week-end 1 :** Pause ou polish

---

### Semaine 2 : Vagues + Boss + Game Feel

```
Jour 6 : Spawn de vagues
├─ WaveManager
├─ 3 vagues configurées
└─ Deliverable : Vagues spawns

Jour 7 : Premier boss
├─ Classe Boss
├─ 2 phases
├─ HP bar
└─ Deliverable : Boss fight complet

Jour 8-9 : Polish game feel
├─ Camera shake
├─ Plus de VFX
├─ Sons variés
├─ Tweaking valeurs
└─ Deliverable : Combat feel incroyable

Jour 10 : UI complète
├─ HUD (HP, Flow, XP)
├─ Damage numbers
├─ Level up popup
└─ Deliverable : UI lisible

TEST GO/NO-GO #2 : Boss fight fun ?
```

**Week-end 2 :** Playtest avec amis

---

### Semaine 3 : Parts System + 2e Perso

```
Jour 11-12 : Parts System v1
├─ Struct Part (Lame, Handle, Gem)
├─ Inventory component
├─ Craft basique (swap armes)
├─ 3 lames, 3 handles, 3 gems
└─ Deliverable : Craft fonctionne

Jour 13-14 : Rarity system
├─ Common/Rare/Epic/Legendary
├─ Loot tables
├─ Coffres spawns
├─ Drop logic
└─ Deliverable : Loot coloré

Jour 15 : 2e Personnage
├─ The Rogue (speed variant)
├─ Stats différentes
├─ Starting weapon différent
└─ Deliverable : 2 persos jouables
```

---

### Semaine 4 : Level Up + Étage Complet

```
Jour 16-17 : XP & Level Up
├─ XP gain sur kills
├─ Level up logic
├─ Choix de 3 options (UI)
└─ Deliverable : Level up fonctionne

Jour 18-20 : Premier étage complet
├─ 3 salles de combat
├─ 1 salle repos (coffre)
├─ 1 boss room
├─ Transitions entre salles
└─ Deliverable : 1 étage jouable de A à Z

TEST FINAL PROTOTYPE :
├─ Run complet (5-7 min)
├─ Fun constant ?
├─ Rejouabilité ?
└─ DÉCISION : Continue production ou pivot
```

---

## Phase 2 : VERTICAL SLICE (Semaine 5-8)

**Objectif :** 2 étages complets, polished

```
Semaine 5 :
├─ +5 types d'ennemis
├─ +2 élites
├─ +1 boss (Berserker)
└─ Étage 2 layout

Semaine 6 :
├─ +2 personnages (Berserker, Monk)
├─ +5 parts (lames, gems)
├─ Polish VFX/SFX
└─ Balance tweaking

Semaine 7 :
├─ HP system (hybride flow/HP)
├─ Bouclier implementation
├─ Screen effects (Zen mode)
└─ Musique adaptive (layers)

Semaine 8 :
├─ UI/UX polish complet
├─ Menu principal
├─ Pause menu
├─ Settings (audio, video)
└─ Playtesting externe (10+ personnes)

DELIVERABLE : 15-20 min de gameplay AAA-quality
```

---

## Phase 3 : PRODUCTION (Semaine 9-20)

**Objectif :** Jeu complet (4 étages)

```
Semaines 9-12 : Contenu
├─ Étages 3-4 (layouts + boss)
├─ Tous les ennemis (10 types)
├─ Tous les élites (4)
├─ Toutes les parts (5×5×6)
├─ 2 derniers persos (Knight, Reaper)
└─ Génération procédurale (si temps)

Semaines 13-16 : Polish & Balance
├─ Tous les VFX/SFX finaux
├─ Musique complète (boss themes)
├─ Balance global (difficulté curve)
├─ Achievements implementation
├─ Unlocks system
└─ Bug fixing massif

Semaines 17-20 : Final Polish
├─ Performance optimization
├─ UI/UX final pass
├─ Trailer creation
├─ Store page (Steam)
├─ Press kit
├─ Marketing materials
└─ Beta testing (50+ joueurs)

DELIVERABLE : Jeu complet, prêt à launch
```

---

## Phase 4 : LAUNCH & POST-LAUNCH (Semaine 21+)

```
Semaine 21 : Pre-Launch
├─ Build final
├─ Steam page live
├─ Wishlist campaign
├─ Devlog final (recap)
└─ Press outreach

Semaine 22 : LAUNCH 🚀
├─ Release day
├─ Community management
├─ Bug hotfixes
└─ Monitor feedback

Semaines 23+ : Support & Updates
├─ Patch bugs critiques
├─ Balance updates
├─ QoL improvements
├─ (Optionnel) DLC planning
└─ Marketing continu
```

---

<a name="estimations"></a>
# 9. ESTIMATIONS & SCOPE

## Temps de Développement

```
PROTOTYPE (Semaine 1-4)     : 1 mois
VERTICAL SLICE (Semaine 5-8): 1 mois
PRODUCTION (Semaine 9-20)   : 3 mois
LAUNCH (Semaine 21+)        : 1+ mois

TOTAL RÉALISTE : 5-6 mois (solo dev, temps partiel 20h/semaine)
         ou     : 3-4 mois (solo dev, full-time 40h/semaine)
```

---

## Difficulté Technique par Système

```
SYSTÈME                  | DIFFICULTÉ | TEMPS ESTIMÉ
-------------------------|------------|-------------
Combat basique           | ⭐⭐⭐     | 1 semaine
Flow system              | ⭐⭐⭐⭐   | 1 semaine
AI ennemis               | ⭐⭐⭐     | 1 semaine
Boss fights              | ⭐⭐⭐⭐   | 2-3 jours/boss
Parts system             | ⭐⭐⭐⭐   | 1.5 semaines
Génération procédurale   | ⭐⭐⭐     | 1 semaine
Game feel (VFX/SFX)      | ⭐⭐⭐⭐⭐ | Continu
UI/UX                    | ⭐⭐⭐     | 1 semaine
Balance                  | ⭐⭐⭐⭐⭐ | Continu
```

---

## Risques & Mitigation

### Risque #1 : Scope Creep
**Probabilité :** Élevée
**Impact :** Projet jamais fini
**Mitigation :**
- Stick au scope minimal validé
- Note les idées pour post-launch
- Timeboxing strict (si feature prend 2x temps estimé → cut)

---

### Risque #2 : Combat Feel Pas Satisfaisant
**Probabilité :** Moyenne
**Impact :** Jeu entier raté
**Mitigation :**
- Prototype en 2 semaines MAX
- Test GO/NO-GO early
- Itération rapide sur feedback
- Référence constante : Hades, DMC

---

### Risque #3 : Burnout
**Probabilité :** Moyenne
**Impact :** Projet abandonné
**Mitigation :**
- Pauses régulières (week-ends off)
- Célébrer chaque milestone
- Montrer progrès (devlogs)
- Ne pas cruncher (pace soutenable)

---

### Risque #4 : Technical Blockers
**Probabilité :** Moyenne
**Impact :** Délais
**Mitigation :**
- Budget temps pour "unknown unknowns" (+20%)
- Community support (forums Unreal, Discord)
- Willingness to simplify si bloqué
- Backup plans (ex: skip génération procédurale si trop dur)

---

## Budget (si applicable)

```
COÛTS POSSIBLES :
├─ Unreal Engine : GRATUIT (royalty 5% après 1M$ revenus)
├─ Mixamo assets : GRATUIT
├─ Freesound SFX : GRATUIT
├─ Musique (composer/royalty-free) : 0-500€
├─ Marketing (ads optionnel) : 0-1000€
├─ Steam fee (per-game) : 100€
└─ Playtesting incentives : 0-200€

TOTAL MINIMUM : 100€ (juste Steam)
TOTAL CONFORTABLE : 500-1000€
```

---

## Potentiel Commercial

### Comparaison avec jeux similaires (solo/small team)

```
Vampire Survivors : 10€, 5M+ ventes, solo dev
Dead Cells        : 25€, 5M+ ventes, small team
Hades            : 25€, 5M+ ventes, 20+ team
Risk of Rain 2    : 25€, 4M+ ventes, 3 devs
Brotato          : 5€, 500k+ ventes, solo dev
```

### Projection réaliste (conservatrice)

```
Prix de vente    : 10-15€
Wishlists target : 10k-20k (via devlogs TikTok/Twitter)

Scenario pessimiste :
├─ 5k wishlists → 1k ventes semaine 1
└─ Revenue : 10k€ (avant taxes/fees)

Scenario réaliste :
├─ 15k wishlists → 3k ventes semaine 1
└─ Revenue : 30k€

Scenario optimiste :
├─ 50k wishlists → 10k ventes semaine 1
└─ Revenue : 100k€

Scenario viral (rare mais possible) :
├─ 100k+ wishlists → 30k+ ventes
└─ Revenue : 300k€+
```

**Note :** Ces chiffres AVANT Steam cut (30%), taxes, etc.

---

## Marketing Strategy (Teaser)

**Pendant le dev (devlogs) :**
- TikTok/YouTube Shorts : 2-3 posts/semaine
- Twitter : Daily updates (GIFs)
- Reddit : r/gamedev, r/IndieGaming (milestones)

**1 mois avant launch :**
- Demo gratuite (Steam Next Fest)
- Press kit aux streamers/journalistes
- AMA Reddit

**Launch :**
- Discount -15% première semaine
- Community management actif
- Patch rapide si bugs
