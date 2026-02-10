# NECK — ageNt Embodied Cognition development Kit

**NECK** (ageNt Embodied Cognition development Kit) is a lightweight C++ framework for developing **embodied cognitive agent bodies** on resource-constrained embedded devices (e.g., Arduino-class microcontrollers).

NECK was conceived as a concrete instantiation of an **embodied cognition perspective applied to BDI-agent-based embedded systems**, addressing theoretical and practical limitations of traditional computationalist approaches when agents are physically situated in the real world.

This framework is grounded on the research presented in *My Body, My Perceptions: A Shift from Computationalism to Embodied Cognition in BDI-agent-based Embedded Systems* (AAMAS 2026).

---

## Motivation

Most BDI-based agent frameworks adopt a **brain-centric and computationalist metaphor**, where cognition is treated as symbolic manipulation detached from the agent’s physical reality.
When applied to embedded systems, this approach leads to architectural ambiguities, such as:

- Treating the agent’s body as an environmental artifact
- Mixing bodily and environmental perceptions
- Lacking explicit representation of internal, positional, and external bodily states
- Weak support for accountability and traceability in physical action

Inspired by **Embodied Cognition**, NECK assumes that **the body is not an interface to the agent — the body is the agent itself**.

---

## Conceptual Foundations

NECK operationalizes the notion of a **mechatropsychosocial entity**, integrating:

- Mechatronic aspects (sensors, actuators, hardware constraints)
- Psychological aspects (perceptions and action capabilities exposed to the agent’s mind)
- Social aspects (interaction with external agents and organizational structures)

The framework explicitly supports three sources of bodily perception derived from cognitive science:

- **Interoception** — internal bodily state
- **Exteroception** — perception of external stimuli
- **Proprioception** — perception of body position and movement

These concepts are aligned with the **MAOP+ᵇ** model proposed in the associated research, extending the Multi-Agent Oriented Programming paradigm with an explicit notion of body.

---

## Architectural Principles

NECK enforces the following design principles:

- One Apparatus per embedded device
- Clear separation between body and mind
- Explicit representation of perceptions, actions, and tacit (procedural) know-how
- No hidden autonomy or implicit deliberation
- Minimal and inspectable communication

The framework is intentionally **non-deliberative**: decision-making, planning, and reasoning are externalized to cognitive agents (e.g., BDI agents implemented in Jason or JaCaMo).

---

## Core Abstractions

### Apparatus

An `Apparatus` represents the **entire embodied agent** on a single device.

- Exactly one `Apparatus` is allowed per microcontroller (enforced at compile time)
- Acts as the root container for the agent’s body

```cpp
#include <NECK.hpp>
Apparatus(myApparatus);
```

---

### Elements

`Elements` represent physical or logical parts of the body (e.g., sensors, actuators).

```cpp
Element(myApparatus, led);
Element(myApparatus, motor);
```

---

### Perceptions

Perceptions expose bodily states to the agent’s mind and are classified by source.

```cpp
Perception(led, ledStatus, PROPRIOCEPTION) {
  return digitalRead(13);
}
```

---

### Actions

Actions define what the body can do and expose **capabilities**, not intentions.

```cpp
Action(led, toggleLED) {
  digitalWrite(13, !digitalRead(13));
  return EXECUTED;
}
```

---

### Tacit Knowledge (Know-How)

Tacit knowledge represents **procedural, embodied know-how**, not symbolic plans.

```cpp
TacitKnowledge(myApparatus, blinkSkill, "context", "instructions");
```

---

## Communication Protocol

NECK uses a minimal and transparent protocol: **JSON-SLP (JSON-seq over SLIP over Serial)**.

Example command:
```json
{"msg":"blinkOperation","args":[true]}
```

---

## Intended Use

NECK is intended for embedded multi-agent systems, agent-based robotics, hybrid BDI architectures, and research on embodied cognition, particularly when accountability and inspectability are required.


---

## License

![](https://i.creativecommons.org/l/by/4.0/88x31.png)

NECK is licensed under a [Creative Commons Attribution 4.0 International License](http://creativecommons.org/licenses/by/4.0/). The licensor cannot revoke these freedoms as long as you follow the license terms:

* __Attribution__ — You must give __appropriate credit__ like below:

Nilson Lazarin, Carlos Pantoja, and Jose Viterbo. 2026.
*My Body, My Perceptions: A Shift from Computationalism to Embodied Cognition in BDI-agent-based Embedded Systems.*
In Proc. of the 25th International Conference on Autonomous Agents and Multiagent Systems (AAMAS 2026),
Paphos, Cyprus, May 25–29, 2026. IFAAMAS, 10 pages.
https://doi.org/10.65109/QIVX3835
