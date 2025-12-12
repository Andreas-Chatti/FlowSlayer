# FLOW SLAYER - GUIDE DE DÉVELOPPEMENT

---

## RÉSUMÉ DU PROJET

**Flow Slayer** est un action roguelite en 3D inspiré de Hades, Devil May Cry, Dead Cells et Furi.

**Objectifs Core :**
- Massacrer des hordes d'ennemis dans des mini-donjons générés aléatoirement
- Système **Flow/Momentum** au cœur du gameplay : plus tu maintiens ton flow, plus tu deviens rapide et puissant
- **Runs courts** (30-40 min max, 4 étages)
- **Craft d'arme modulaire** : Lame + Handle + Gem (150 builds de base)
- **100% skill-based** : Pas de méta-progression stats, pure progression in-run
- **Plusieurs personnages** avec playstyles différents
- **Combos stylés** mais accessibles

**Tech Stack :**
- Unreal Engine 5.4.4
- C++ (gameplay) + Blueprints (UI, VFX)
- Third-Person 3D
- PC (KB+M)

---

## RÈGLES DE DÉVELOPPEMENT

### 1. Conventions de Nommage et Style

#### Variables
```cpp
// ✅ CORRECT : Utiliser bracket initialization
int myInt{3};
float myFloat{1.5f};
bool isActive{false};

// ❌ INCORRECT
int myInt = 3;
```

#### camelCase pour variables locales
```cpp
void MyFunction()
{
    int enemyCount{10};
    float dashDistance{1250.0f};
    bool hasMovementInput{false};
}
```

#### PascalCase pour fonctions et variables membres
```cpp
class AMyCharacter
{
public:
    void PerformAttack();
    float GetDashDistance() const;

    UPROPERTY()
    float MaxHealth{100.f};

    UPROPERTY()
    int CurrentLevel{1};
};
```

#### Conventions Unreal pour booléens
```cpp
// ✅ CORRECT : Préfixe 'b' pour les bool
bool bIsDead{false};
bool bCanDash{true};
bool bHasMovementInput{false};

// ❌ INCORRECT
bool isDead{false};
bool canDash{true};
```

---

### 2. Style de Code

#### Brackets sur nouvelle ligne
```cpp
// ✅ CORRECT
if (statement)
{
    // Code here
}

void MyFunction()
{
    // Code here
}

// ❌ INCORRECT
if (statement) {
    // Code here
}
```

#### Commentaires explicatifs
```cpp
// ✅ TOUJOURS commenter variables/fonctions/classes membres
class AFlowSlayerCharacter
{
    /** Is Player Dead ? */
    UPROPERTY(BlueprintReadOnly)
    bool bIsDead{false};

    /** Maximum velocity required to enter sprint state */
    UPROPERTY(BlueprintReadOnly)
    float SprintSpeedThreshold{900.f};

    /** Rotates character smoothly toward camera direction */
    void RotatePlayerToCameraDirection();
};

// ✅ Commenter code complexe
void ComplexFunction()
{
    // Calculate dynamic flow decay based on time since last hit
    // Formula: baseDecay * (1 + timeSinceHit^2)
    float dynamicDecay = baseFlowDecay * (1.0f + FMath::Square(timeSinceLastHit));
}
```

---

### 3. Priorités Techniques

#### 🔥 TOUJOURS chercher solutions natives Unreal
```cpp
// ✅ CORRECT : Solution native Unreal
APlayerController* PC = Cast<APlayerController>(GetController());
bool isLMBPressed = PC->IsInputKeyDown(EKeys::LeftMouseButton);

// ❌ INCORRECT : Créer des flags inutiles
bool bLeftClickPressed{false};  // Inutile !
```

**Avant d'implémenter une solution custom, TOUJOURS vérifier :**
- Documentation Unreal officielle
- Engine source code (ex: ACharacter, UCharacterMovementComponent)
- Unreal forums / AnswerHub
- Est-ce que Unreal a déjà une solution native ?

#### Certitude minimale : 85%
```
❓ Si incertain de la solution (< 85% de confiance) :
   1. Faire une recherche internet AVANT de proposer
   2. OU préciser à l'utilisateur : "Je ne suis pas sûr à 100%, je te recommande de vérifier X"

✅ Ne proposer que des solutions validées et testées
```

#### Version Unreal : 5.4.4
```cpp
// ⚠️ ATTENTION : Pas d'API deprecated
// Toujours vérifier si la fonction/classe est marquée "DEPRECATED" dans la doc

// ✅ CORRECT (UE 5.4)
EnhancedInputComponent->BindAction(...)

// ❌ INCORRECT (Deprecated depuis UE 5.0)
PlayerInputComponent->BindAction(...)  // Old Input System
```

---

### 4. Communication et Réponses

#### Réponses concises et au point
```
✅ CORRECT :
- Aller droit au but
- Donner les précisions essentielles
- Pas trop court (éviter réponses de 2 lignes si complexe)
- Pas trop long (éviter pavés inutiles)

❌ INCORRECT :
- Expliquer en profondeur si l'utilisateur n'a pas demandé
- Donner 10 exemples si 1-2 suffisent
- Répéter plusieurs fois la même info
```

**Exception :** Si l'utilisateur demande explicitement "explique-moi en détail", alors développer.

---

## EXEMPLES DE CODE

### Input System (Enhanced Input)
```cpp
// Helper pour éviter répétition de code
TPair<bool, bool> AFlowSlayerCharacter::GetMouseButtonStates() const
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
        return TPair<bool, bool>(false, false);

    bool isLMBPressed = PC->IsInputKeyDown(EKeys::LeftMouseButton);
    bool isRMBPressed = PC->IsInputKeyDown(EKeys::RightMouseButton);

    return TPair<bool, bool>(isLMBPressed, isRMBPressed);
}

// Utilisation avec structured bindings (C++17)
void AFlowSlayerCharacter::OnDashAttackActionStarted(const FInputActionInstance& Value)
{
    auto [isLMBPressed, isRMBPressed] = GetMouseButtonStates();

    if (isLMBPressed)
    {
        // LMB logic
    }
    else if (isRMBPressed)
    {
        // RMB logic
    }
}
```

### Combat System (Enum + TMap)
```cpp
// Header
UENUM(BlueprintType)
enum class EAttackType : uint8
{
    None,
    StandingLight,
    StandingHeavy,
    DashPierce,
    // ... etc
};

TMap<EAttackType, FCombo*> ComboLookupTable;

// CPP
void UFSCombatComponent::InitializeComboLookupTable()
{
    ComboLookupTable.Add(EAttackType::StandingLight, &StandingLightCombo);
    ComboLookupTable.Add(EAttackType::StandingHeavy, &StandingHeavyCombo);
    ComboLookupTable.Add(EAttackType::DashPierce, &DashPierceAttack);
    // ... etc
}

FCombo* UFSCombatComponent::SelectComboBasedOnState(EAttackType attackType)
{
    FCombo** foundCombo = ComboLookupTable.Find(attackType);
    return foundCombo ? *foundCombo : nullptr;
}
```

### Bracket Initialization
```cpp
// ✅ Variables membres
UPROPERTY(EditDefaultsOnly, Category = "Movements")
float DashDistance{1250.0f};

UPROPERTY(EditDefaultsOnly, Category = "Movements")
float DashCooldown{0.75f};

// ✅ Variables locales
void MyFunction()
{
    int count{0};
    float speed{600.f};
    bool isValid{true};
}
```

---

## CHECKLIST AVANT COMMIT

- [ ] Bracket initialization utilisée
- [ ] camelCase pour variables locales
- [ ] PascalCase pour fonctions/variables membres
- [ ] Booléens préfixés avec 'b'
- [ ] Brackets sur nouvelle ligne
- [ ] Commentaires pour variables/fonctions membres
- [ ] Code complexe commenté
- [ ] Solution native Unreal vérifiée
- [ ] Aucune API deprecated (UE 5.4.4)
- [ ] Certitude ≥ 85% ou recherche effectuée

---

## RESSOURCES

**Documentation Unreal :**
- [Unreal Engine 5.4 Documentation](https://docs.unrealengine.com/5.4)
- [Enhanced Input System](https://docs.unrealengine.com/5.4/enhanced-input-in-unreal-engine)
- [Gameplay Ability System](https://docs.unrealengine.com/5.4/gameplay-ability-system-for-unreal-engine)

**Code Standards :**
- [Unreal Coding Standard](https://docs.unrealengine.com/5.4/epic-cplusplus-coding-standard-for-unreal-engine)

**Community :**
- [Unreal Engine Forums](https://forums.unrealengine.com/)
- [Unreal Slackers Discord](https://unrealslackers.org/)
