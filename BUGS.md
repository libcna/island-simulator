# CNA and Sharp Runtime Issues Affecting island-simulator

This file is the authoritative record of confirmed or strongly evidenced framework defects that
materially affect `island-simulator`. CNA and Sharp Runtime are read-only dependencies. Framework
issues are worked around only inside this repository and are never patched in their worktrees.

No qualifying CNA or Sharp Runtime defect has been confirmed yet.

Consumer integration constraints, unsupported features, renderer limitations, and incorrect game
code do not belong here unless investigation demonstrates a framework defect. Suspected entries
must say `Status: Suspected` and must not present a guessed cause as fact.

## Entry template

<!-- Copy this section only after a qualifying issue has been investigated. -->

### CNA-ISLAND-NNN — Concise title

**Component:** CNA or Sharp Runtime

**Branch:** next

**Revision:** commit hash

**Platform:** affected platform(s)

**Renderer:** affected renderer(s), or N/A

**Severity:** Low / Medium / High / Critical

**Status:** Suspected / Confirmed / Workaround active / Fixed upstream, workaround removed

**Date discovered:** YYYY-MM-DD

#### Symptoms

Describe the observable failure.

#### Expected behaviour

Describe the supported behaviour the framework should provide.

#### Actual behaviour

Describe the measured behaviour.

#### Reproduction

Give the smallest practical reproduction and its prerequisites.

#### Suspected cause

Name relevant framework locations and distinguish evidence from inference.

#### Impact on island-simulator

Describe affected game systems and platforms.

#### Workaround

Describe the isolated game-side workaround and link its source location.

#### Workaround limitations

Describe compromises, platform differences, or residual risk.

#### Workaround removal

Explain how to detect an upstream fix and remove the workaround safely.

#### Validation

List regression tests, builds, and runtime checks performed.
