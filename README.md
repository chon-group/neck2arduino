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
/*  perceive()            <-- agent level 
  {"msg":"getPercepts"}    <-- communication 

{"belief":"ledStatus","element":"led","type":"proprioception","response":"percepted","args":[1]} <-- communication reply
{"belief":"motorStatus","element":"motor","type":"proprioception","response":"percepted","args":["running"]} <-- communication reply

+motorStatus(running).  <-- agent level
+ledStatus(1).          <-- agent level
*/

Perception(led, ledStatus, PROPRIOCEPTION) {
  return digitalRead(13);
}


Perception(motor, motorStatus, PROPRIOCEPTION) {
  return strMotorStatus;
} 



```

---

### Actions

Actions define what the body can do and expose **capabilities**, not intentions.

```cpp
/* act(toggleLED); <-- agent level
    {"msg":"toggleLED"}    <-- communication
    {"apparatus":"myApparatus","response":"executed","request":"toggleLED","apparatusID":3464630983,"element":"led"} <-- communication replay
*/
Action(led, toggleLED) {
  digitalWrite(13, !digitalRead(13));
  return EXECUTED;
}

/* act(machine(goAhead));        <-- agent level
    {"msg":"machine","args":["goAhead"]} <-- communication
    {"apparatus":"myApparatus","response":"already","request":"machine","apparatusID":3464630983,"element":"motor"} <-- communication replay
*/
Action (motor,machine){
  if(!ActionArgs.isString(0)) return INVALID; 
  if(strMotorStatus == ActionArgs.asString(0)) return ALREADY;

  if(ActionArgs.asString(0) == "goAhead") goAheadFunction();
  if(ActionArgs.asString(0) == "stopRightNow") stopRightNow();
  
  return EXECUTED;  

```

---

### Tacit Knowledge (Know-How)

Tacit knowledge represents **procedural, embodied know-how**, not symbolic plans.

```cpp
TacitKnowledge(myApparatus, blinkSkill, "context", "instructions");
```

---

### Trieb (Embodied Drives)

NECK adopts the notion of **Trieb** (drive) to characterize **pre-deliberative bodily tendencies** that arise from the agent’s concrete engagement with the world.

A *trieb* is neither a goal, nor a desire, nor an intention. It does not belong to the space of reasons, but to the space of **corporeal causation**. It expresses how the body, through its ongoing activity, establishes gradients of relevance, urgency, and constraint that precede symbolic deliberation.

From an embodied cognition perspective, *trieb* captures the idea that the body is not a passive executor of decisions, but an active source of modulation for cognition. In this sense, bodily activity does not merely follow intentions; it **conditions what can meaningfully become an intention**.

In NECK, actions and perceptions may emit *trieb* signals associated with an intensity value, representing the strength with which a bodily state asserts itself:

```cpp
motor.trieb("overheat_warning", 0.85);

/* 
{"trieb":"overheat_warning","element":"motor","drang":0.85,"apparatus":"myApparatus"} <-- communication replay

!overheat_warning. <-- agent level
*/

```

These signals are exposed to the agent’s mind as embodied drives, allowing external cognitive agents to take corporeal dynamics into account without embedding motivational or deliberative mechanisms inside the body itself.

Conceptually, trieb occupies an intermediate level between perception and desire: it is neither a mere sensory datum nor a symbolic commitment, but a pre-intentional force through which the body participates in shaping the agent’s cognitive horizon.

---

## Communication Protocol

NECK uses a minimal and transparent protocol: **JSON-SLP (JSON-seq over SLIP over Serial)**.

Example command:
```json
{"msg":"blinkOperation","args":[true]}
```

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
