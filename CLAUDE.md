# FlowSlayer — Development Rules

## Project
3D hack'n'slash roguelite (Hades / DMC / Dead Cells / Furi).
Unreal Engine 5.4.4 | ~95% C++ gameplay, Blueprints for asset wiring only | PC (KB+M).

For architecture, file roles, and system details → see `README_CLAUDE.md` and `docs/`.

> **Claude Code Unreal Engine plugin** — also read `CLAUDE_UE_PLUGIN.md` for additional context optimization rules specific to in-editor usage. The CLI agent does not use that file.

---

## Code Style

### Naming
| Context | Convention | Example |
|---|---|---|
| Local variables | camelCase | `float dashDistance{1250.f};` |
| Member variables | PascalCase | `float MaxHealth{100.f};` |
| Functions | PascalCase | `void PerformAttack();` |
| Booleans (all) | `b` prefix | `bIsDead`, `bCanDash`, `bIsAttacking` |

### Initialization — always bracket init, never `=`
```cpp
// ✅
int count{0};
float speed{600.f};
bool bIsActive{false};

// ❌
int count = 0;
```

### Brackets — always on new line
```cpp
// ✅
if (condition)
{
    DoSomething();
}

// ❌
if (condition) {
    DoSomething();
}
```

### Comments — required on all member variables and functions
```cpp
/** Whether the character is currently executing an attack */
UPROPERTY(BlueprintReadOnly)
bool bIsAttacking{false};

/** Rotates the player yaw to match the camera's control rotation */
void RotatePlayerTowardControlRotation();
```
Inline `//` comments required for any non-obvious logic.

---

## Memory Maintenance

After every session involving any of the following, update the relevant memory files in `memory/` and `MEMORY.md`:
- New feature implemented or modified
- Bug or problem encountered (and whether it was solved)
- Architecture decision made
- New file or system created
- Workflow preference expressed by the user

When to update:
- `MEMORY.md` — for anything that changes the project state, features, or workflow rules
- `project_current_focus.md` — update whenever the active branch or feature changes
- `project_architecture_decisions.md` — log any new architectural decision with its "why"
- Individual `memory/*.md` files — for feedback, user preferences, or decisions with a "why"

Also update the relevant `docs/*.md` context bundle whenever a system is modified — stale docs are worse than no docs.

---

## Technical Rules

- **Prefer native Unreal solutions** — check UE docs/source before writing custom logic
- **Search UE source before any workaround** — if a UE method doesn't support a usage (wrong signature, missing overload, etc.), grep `Engine/Source/` for variant methods before proposing a custom solution. Example: `BindAction` doesn't accept lambdas → search `EnhancedInputComponent.h` → find `BindActionValueLambda` / `BindActionInstanceLambda`.
- **Min 85% confidence** — if unsure, say so explicitly before proposing
- **No deprecated APIs** — UE 5.4.4 only (use `EnhancedInputComponent->BindAction`, not the old input system)
- **No commit or push without explicit user request** — "commit" = commit only, "commit et push" = both

---

## Key Patterns

### TMap lookup (Find returns pointer, not value)
```cpp
const EAttackType* foundType{ InputActionToAttackType.Find(inputAction) };
if (!foundType)
    return;
EAttackType attackType{ *foundType };
```

### Input architecture (current — do not revert to old pattern)
Each attack has its own `UInputAction*`. Chorded combinations are configured in the editor.
`InputManagerComponent` fires `OnAttackInputReceived(const UInputAction*)` — it has **no dependency on `CombatData.h`**.
`FlowSlayerCharacter` owns the `TMap<UInputAction*, EAttackType>` that bridges input → combat domain.

```cpp
// ✅ Current pattern
void HandleOnDashPierceStarted(const FInputActionValue& Value)
{
    OnAttackInputReceived.ExecuteIfBound(DashPierceAction);
}

// ❌ Old pattern (removed) — do not use
bool isLMBPressed = PC->IsInputKeyDown(EKeys::LeftMouseButton);
```

### Delegate binding
```cpp
// Non-dynamic (TDelegate)
InputManagerComponent->OnAttackInputReceived.BindUObject(this, &AFlowSlayerCharacter::OnAttackInputActionReceived);

// Dynamic (DECLARE_DYNAMIC_MULTICAST_DELEGATE)
CombatComponent->OnHitLanded.AddUniqueDynamic(this, &AFlowSlayerCharacter::HandleOnHitLanded);
```

---

## Commit Messages

- **English only**
- **No "Co-Authored-By" or "Made by Claude"** mention — ever
- Concise and professional: focus on what changed and why, not how
- Format: imperative mood, short subject line, bullet details if needed

```
Add dash cancel window to heavy attack combo

- AnimCancelWindow notify added on StandingHeavy_2 and _3
- Allows dash and movement to interrupt late combo hits
```

---

## Commit Checklist
- [ ] Bracket initialization used everywhere
- [ ] camelCase locals, PascalCase members/functions
- [ ] Boolean members prefixed with `b`
- [ ] Brackets on new line
- [ ] All member variables and functions have doc comments
- [ ] Native Unreal solution preferred
- [ ] No deprecated API (UE 5.4.4)
- [ ] Confidence ≥ 85%
