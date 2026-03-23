# Instructions for the Claude Code Unreal Engine Plugin

> ⚠️ **IMPORTANT — Scope of this file**
> These instructions are addressed **exclusively to the Claude Code Unreal Engine plugin** operating inside the editor.
> The plugin must follow **both** `CLAUDE.md` (shared rules) **and** this file.
> The CLI agent (`claude` command line) must **ignore this file entirely** — these instructions do not apply to it.

---

## Context Optimization — Required Behavior

Token consumption per request can reach 20–50k tokens, which is excessive. The plugin must apply the following rules on every request without exception.

### 1. Load only what the request needs

- **Never** dump the entire world state, all actors, or all components into the context.
- Before building the context, identify the **minimum set of assets/objects** relevant to the request.
- Example: if the request is about `ABP_GreatSwordChar`, only load that ABP and its direct dependencies — not the whole Content Browser.

### 2. Truncate or summarize large data

- If an asset or struct is large, extract only the fields relevant to the task.
- Summarize list data (e.g. "47 nodes in state machine — showing only dash-related nodes") rather than serializing everything.
- Cap raw data payloads: if a single object would exceed ~2k tokens of context, summarize it instead.

### 3. No redundant re-injection

- Do **not** re-send data that was already provided earlier in the same conversation and has not changed.
- Track what has already been sent in the conversation and skip re-injection.

### 4. Prefer targeted queries over broad scans

- When looking up an asset or property, query it directly rather than scanning collections.
- Avoid enumerating all assets of a type when only one specific asset is needed.

### 5. Conversation history — sliding window

- Keep a maximum of **10 recent exchanges** in the active context.
- Summarize older exchanges into a short paragraph rather than dropping them entirely, to preserve intent continuity.
- Never send the full raw conversation history from the beginning of the session.

### 6. System prompt — send once

- The system prompt (project rules, code style, etc.) should be injected **once per session**, not on every request.
- If the plugin supports prompt caching, enable it for the system prompt.

---

## What to Preserve — Do Not Over-Optimize

The goal is efficiency, not cutting corners. The following must never be sacrificed:

- Full content of the specific file or asset being modified
- Relevant type definitions (`EAttackType`, `FAttackData`, `FCombo`, etc.) when editing combat/input code
- The current request's full user message — never truncate what the user wrote
- Error messages and compiler output in full — these are small and critical

---

## Target Budget

| Request type | Target token range |
|---|---|
| Simple asset read / property check | < 3k |
| Code edit (single file) | 5–10k |
| Multi-file refactor | 10–20k |
| Complex Blueprint / ABP task | 15–25k |

Requests consistently exceeding these ranges should be flagged internally as a context hygiene issue.
