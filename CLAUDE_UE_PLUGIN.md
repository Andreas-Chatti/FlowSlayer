# Instructions for Agents with Unreal Editor MCP Tools

> **Who must follow this file?** Any agent that has `mcp__unreal-editor__*` tools available.
> These rules complement `CLAUDE.md` (shared rules) and the plugin's own injected instructions.
> Agents **without** MCP tools (CLI-only) must **ignore this file entirely**.
>
> The plugin already injects workflow rules (read before edit, compile, save, verify, troubleshooting, safety, node layout). **Do not duplicate those here.** This file covers only what the plugin does NOT inject.

---

## 1. Tool Call Efficiency

### Read surgically
- Always use `read_asset` with `graph=` to read **specific graphs** — never the whole Blueprint.
- Don't re-read a graph you already read in this conversation unless you modified it since.
- Use `find_nodes` (via `edit_graph`) to locate a node **before** reading the full graph.

### Batch independent calls
- Multiple independent reads (e.g. two different assets) → call in parallel, not sequentially.

### Don't fish
- If you don't know an asset path, ask the user. Don't scan the Content Browser.
- If the user gives a name like `BP_Grunt`, construct the path directly — don't enumerate.

---

## 2. Tool Selection Guide

| Task | Tool | Notes |
|---|---|---|
| Read BP structure / graph | `read_asset` (with `graph=`) | Always first step |
| Find a node in a graph | `edit_graph` (`operation='find_nodes'`) | Before manual scanning |
| Edit BP graph nodes/wires | `edit_graph` | Marks dirty only — must compile after |
| Edit BP components/properties | `edit_blueprint` | Marks dirty only — must compile after |
| Compile | `read_logs` (`operation='compile_blueprint'`) | |
| Save asset | `execute_python` | `unreal.EditorAssetLibrary.save_asset(path)` |
| List/find assets by path | `execute_python` | `unreal.EditorAssetLibrary.list_assets(folder)` |
| Bulk operations / batch | `execute_python` | Any operation on multiple assets |
| Visual verification | `screenshot` | UI, materials, animations, layout |
| Edit Animation BP | `edit_animation_asset` | State machines, blend spaces |
| Edit Behavior Tree | `edit_ai_tree` | AI logic |
| Edit Niagara VFX | `edit_niagara` | Particle systems |
| Edit Sequencer | `edit_sequencer` | Cinematics, level sequences |
| Generate texture/mesh | `generate_asset` | AI-generated assets |

---

## 3. Useful `execute_python` Patterns

```python
# List assets in a folder
unreal.EditorAssetLibrary.list_assets('/Game/Blueprints', recursive=True)

# Save an asset after compile
unreal.EditorAssetLibrary.save_asset('/Game/Blueprints/BP_Grunt')

# Get/set a default property value
obj = unreal.load_object(None, '/Game/Blueprints/BP_Grunt.Default__BP_Grunt_C')

# Find all actors in the current level
actors = unreal.EditorLevelLibrary.get_all_level_actors()

# Filter actors by class
enemies = unreal.GameplayStatics.get_all_actors_of_class(unreal.EditorLevelLibrary.get_editor_world(), unreal.Class)
```

---

## 4. What to Always Preserve (never over-optimize)

- The **complete graph** being modified — always read in full before editing
- **Type definitions** (`EAttackType`, `FAttackData`, `FCombo`, etc.) when editing combat/input logic
- **Compiler errors and logs** — always in full, never summarized
- The **user's full request** — never truncate or paraphrase

---

## 5. Project-Specific Rules

### C++ vs Blueprint boundary
This project is **~95% C++ gameplay**. Blueprints are for asset wiring only (montage refs, data tables, widget layout). Never solve a gameplay logic problem in Blueprint when it should be C++.

### Context bundles — read docs/ before touching a system
Before modifying any major system via MCP tools, read the corresponding `docs/` file:

| System | Doc file |
|---|---|
| Combos, attacks, hitbox, guard, streak | `docs/CombatSystem_Context.md` |
| Input, Enhanced Input, chorded actions | `docs/InputSystem_Context.md` |
| Flow, momentum, tiers, decay, immunity | `docs/FlowSystem_Context.md` |
| Enemies, AI, death, hit reception | `docs/EnemySystem_Context.md` |
| Lock-on, targeting, camera | `docs/LockOnSystem_Context.md` |
| AnimNotify, montage, timing | `docs/AnimNotify_Context.md` |
| Dash, direction, cooldown, flow cost | `docs/DashSystem_Context.md` |

### Visual verification
Use `screenshot` after modifying anything visual (materials, widget layout, animation blends, Niagara VFX) to confirm the result matches intent.
