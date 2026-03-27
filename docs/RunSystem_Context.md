# RunSystem — Context Bundle

## Vue d'ensemble

Orchestration de la game loop complète : progression entre arènes, révélation du portail, téléportation, fin de run. La mort et le reset sont gérés par `FlowSlayerCharacter` + `OpenLevel`.

---

## Acteurs impliqués

| Classe | Rôle |
|---|---|
| `ARunManager` | Singleton placé dans le level. Orchestre les transitions — ne connaît que les ArenaManagers. |
| `AFSArenaManager` | Gère UNE arène + possède son portail de sortie (`ExitPortal`). Appelle `ShowPortal()` au clear. |
| `AArenaPortal` | Placé dans le level, caché par défaut. Téléporte le joueur à l'overlap, broadcast `OnPlayerTeleported`. |

---

## Pipeline complète

```
ARunManager::BeginPlay()
    → StartRun() → ActivateArena(Arenas[0])
        → bind Arena->OnArenaCleared → HandleOnArenaCleared
        → bind Arena->GetExitPortal()->OnPlayerTeleported → StartNextArena
        → Arena->StartArena()

AFSArenaManager::CheckArenaCompletion()
    → ExitPortal->ShowPortal()          // ArenaManager révèle son portail
    → OnArenaCleared.Broadcast()
        → ARunManager::HandleOnArenaCleared()
            → [si dernière arène] OnRunCompleted.Broadcast()
            → [sinon] OnRunArenaCleared.Broadcast()

Joueur entre dans le portail (overlap box)
    → AArenaPortal::TeleportPlayer()
        → SetActorLocation(DestinationActor->GetActorLocation())
        → SetControlRotation(DestinationActor->GetActorRotation())
        → OnPlayerTeleported.Broadcast()
            → ARunManager::StartNextArena()
                → CurrentArenaIndex++
                → ActivateArena(Arenas[CurrentArenaIndex])
```

---

## ARunManager — Interface publique

```cpp
void StartRun();          // appelé en BeginPlay
void StartNextArena();    // BlueprintCallable — appelé via OnPlayerTeleported

int32 GetCurrentArenaIndex();
int32 GetTotalArenas();
bool  IsLastArena();

FOnRunArenaCleared OnRunArenaCleared;  // coffre / autres systèmes réagissent ici
FOnRunCompleted    OnRunCompleted;     // fin de run
```

### Configuration (Details panel)

```
Arenas[]  — liste ordonnée des AFSArenaManager (c'est tout)
```

RunManager ne connaît pas `AArenaPortal` dans son `.h`. Le bind sur `OnPlayerTeleported` se fait via `Arena->GetExitPortal()` dans `ActivateArena`.

---

## AFSArenaManager — Ownership du portail

Chaque `FSArenaManager` possède son portail de sortie :

```cpp
UPROPERTY(EditAnywhere, Category = "Arena|Navigation")
AArenaPortal* ExitPortal{nullptr};  // null pour la dernière arène
```

`CheckArenaCompletion` appelle `ExitPortal->ShowPortal()` avant de broadcaster `OnArenaCleared`.

**Pourquoi ArenaManager et pas RunManager :** l'arène est la propriétaire naturelle de sa sortie. Évite les arrays parallèles dans RunManager (source d'erreurs d'index silencieuses).

---

## AArenaPortal — Design

### Placement statique, révélation dynamique

Placé dans le level par le designer, invisible et sans collision par défaut. `FSArenaManager` l'affiche au clear via `ShowPortal()`.

**Spawn dynamique** redeviendrait justifié uniquement si le portail doit apparaître à une position calculée à runtime (ex: près du joueur). Pour des arènes fixes, le placement statique est plus clair.

### ShowPortal()

Active mesh, collision (`QueryOnly`), et Niagara si `PortalVFX` est assigné.

### DestinationActor

`AActor* DestinationActor` (EditAnywhere) — référence à un acteur placé dans le level (ex: `ATargetPoint`) représentant le point d'arrivée dans l'arène suivante. Visible et déplaçable dans le viewport.

### Preview éditeur

`OnConstruction` active le VFX si `WorldType == Editor || EditorPreview`. Invisible en runtime (PIE/Game) — `ShowPortal()` s'en charge.

---

## Setup éditeur

1. Placer un `ARunManager` dans le level
2. Placer les `BP_ArenaPortal` dans le level (invisibles par défaut)
3. Pour chaque portail : assigner un `ATargetPoint` dans `DestinationActor`
4. Sur chaque `FSArenaManager` : assigner ses `SpawnZones` + son `ExitPortal` (null sur la dernière arène)
5. Dans `RunManager` Details : assigner `Arenas[]` dans l'ordre
6. Désactiver `bForceActivate` sur tous les `FSArenaManager`

---

## Reset de run / mort

Pas de `ResetRun()` dans RunManager. La mort → death screen → `UGameplayStatics::OpenLevel()`. Le reload remet tout à zéro proprement.

---

## État actuel

- [x] `ARunManager` — orchestrateur, transitions, bind portail via getter
- [x] `AFSArenaManager` — SpawnZones manuelles, `ExitPortal`, `ShowPortal()` au clear
- [x] `AArenaPortal` — C++ complet, placement statique, `DestinationActor`, preview editor
- [x] **Boucle complète validée en runtime** (session 12)
- [ ] BP enfant de `AArenaPortal` — mesh + VFX, à créer en editor
- [ ] Death screen + OpenLevel sur mort joueur
- [ ] Coffre placeholder (react à `OnRunArenaCleared`)
