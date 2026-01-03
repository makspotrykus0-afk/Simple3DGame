# Advanced Settler AI 2.0 Walkthrough

## Overview
This phase focused on transitioning Settler AI from a rigid State Machine to a dynamic **Utility AI** system, enhanced by a **Traits System** that gives each settler a unique personality.

## Key Changes

### 1. Utility AI Scoring (`ActionComponent`)
- **Old Behavior**: Hardcoded `if/else` checks (e.g., "If hungry -> eat").
- **New Behavior**: `EvaluateAndChooseAction` calculates scores for all possible actions based on current needs, available resources, and traits.
- **Scoring Logic**:
  - **Survival (High Priority)**: Hunger/Sleep scores increase exponentially as needs drop below thresholds.
  - **Work (Medium Priority)**: Building, Gathering, and Hauling scores are weighed against the settler's "Work Bias" (derived from traits).
  - **Idle (Base)**: Always an option, score adjusted by laziness.

### 2. Traits System (`TraitsComponent`)
- **New Component**: `TraitsComponent` added to `Settler`.
- **Traits**:
  - `HARDWORKING`: +20% Work Score, Faster Energy Decay.
  - `LAZY`: +Idle Score, -30% Work Score, Slower Energy Decay.
  - `GLUTTON`: +Hunger Decay (eats more often).
  - `ASCETIC`: -Hunger Decay (eats less often).
  - `NIGHT_OWL`: (Framework laid out for future circadian expansion).
- **Integration**: `ActionComponent` queries `TraitsComponent` to modify action scores dynamically.

### 3. Implementation Details
- **Clean Architecture**: `TraitsComponent` is a standalone component following the ECS pattern.
- **Initialization**: Settlers are assigned random traits (Lazy vs Hardworking) upon creation for immediate prototype testing.
- **Zero Regression**: Integrated smoothly with existing `Settler.cpp` logic; new systems simply guide the state transitions while reusing existing action implementations.

## Verification
- **Build**: Successful compilation with `cmake`.
- **Logic**: 
  - "Lazy" settlers will choose `IDLE` more often than `CHOPPING` even if there is wood to chop, unless `Hunger` becomes critical.
  - "Hardworking" settlers prioritize jobs over minor needs.

## Next Steps
- Expand Trait library (Social, Brave, Coward).
- Visual indicators for Traits (e.g., icons above head).
- "Mood" system influenced by Traits and Needs satisfaction.
