# NECK — ageNt Embodied Cognition development Kit

**NECK** (ageNt Embodied Cognition development Kit) is a lightweight C++ framework for describing and operating the **physical body of BDI agents** on resource-constrained embedded devices, such as Arduino-class microcontrollers.

NECK was conceived as part of the **MAOP+b** model and provides embedded mechanisms for explicitly representing bodily constitution and integrating bodily processes with the agent's BDI mind.

The conceptual foundations of NECK are presented in *My Body, My Perceptions: A Shift from Computationalism to Embodied Cognition in BDI-agent-based Embedded Systems* (AAMAS 2026).

<img width="3375" height="2775" alt="image" src="https://github.com/user-attachments/assets/6b0d8825-ca97-42b3-8ef8-2c680b9929dc" />

---

## Motivation

Traditional BDI architectures primarily represent the cognitive processes of an agent, while its physical constitution is often treated as an external implementation concern.

In physically embedded systems, however, sensors, actuators and other physical components constitute the body through which the agent perceives and acts in the world.

Inspired by **Embodied Cognition**, NECK explicitly represents this physical constitution and the bodily processes that connect the agent's BDI mind with the physical world.

---

## Conceptual Foundations

NECK is grounded in the notion of a **mechatropsychosocial entity**, integrating:

- **Mechatronic aspects** — the physical body, including sensors, actuators and hardware constraints;
- **Psychological aspects** — the BDI mind and its cognitive processes;
- **Social aspects** — interactions with other agents and organizational structures.

The framework supports three sources of bodily perception derived from cognitive science:

- **Interoception** — perception of the body's internal state;
- **Proprioception** — perception of the body's position and movement;
- **Exteroception** — perception of stimuli originating outside the body.

These concepts are aligned with the **MAOP+b** model proposed in the associated research, which extends the Multi-Agent Oriented Programming paradigm by explicitly representing the bodies of embodied agents.

---

## Body Model

A body is composed of one or more **Apparatus**, each representing a physical subsystem composed of **Elements**.

At the embedded-device level, NECK implements an Apparatus and its Elements.

Conceptually:

```text
                 ┌────── Embodied Agent ──────┐
                 │                            │
                 │           Mind             │
                 │          (BDI)             │
                 │            │               │
                 │   Percept ↑│↓ Act          │
                 │            │               │
                 │      ┌──── Body ────┐      │
                 │      │              │      │
                 │      │  Apparatus   │      │
                 │      │      │       │      │
                 │      │   Element    │      │
                 │      └──────┬───────┘      │
                 └─────────────┼──────────────┘
                    Sensing ↑  │  ↓ Behaving
                               │
                        Physical World
```

The Apparatus therefore operates at the intersection of two relationships:

- **Mind ↔ Body** — through `Percept` and `Act`;
- **Body ↔ Physical World** — through `Sensing` and `Behaving`.

---

## Architectural Principles

NECK follows these principles:

- one `Apparatus` per embedded device;
- explicit representation of the physical constitution of the agent;
- distinction between cognitive and bodily processes while integrating them as parts of the same embodied entity;
- explicit representation of `Percept`, `Act`, `Sensing`, `Behaving`, `Trieb` and `TacitKnowledge`;
- deliberation remains in the BDI mind;
- bodily processes may continuously evolve independently of new deliberative commands.

---

## Basic Structure

A NECK application normally includes only:

```cpp
#include <NECK.hpp>
```

An Apparatus and its Elements can then be declared as follows:

```cpp
Apparatus(myApparatus) {
    Element(led);
    Element(motor);
}
```

The current embedded NECK model allows exactly one `Apparatus` per microcontroller.

---

## Percept

A `Percept` represents information that an Element exposes to the BDI mind when requested.

A Percept returns a perceived value, a set of values, or a `PerceptionResponse`. Percepts are classified according to their source as `INTEROCEPTION`, `PROPRIOCEPTION` or `EXTEROCEPTION`.

```cpp
Percept(led, ledStatus, PROPRIOCEPTION) {
    return digitalRead(13);
}

Percept(motor, motorStatus, PROPRIOCEPTION) {
    return strMotorStatus;
}
```

Percept is distinct from `Sensing`: Sensing continuously acquires or updates bodily state, whereas a Percept exposes information to the BDI mind when requested.

---

## Act

An `Act` represents an intentional bodily operation requested by the BDI mind and performed through an Element.

An Act returns an `ActionResponse` indicating the outcome of the requested operation, such as `EXECUTED`, `UNABLE`, `ALREADY`, `REJECTED` or `INVALID`.

```cpp
Act(led, toggleLED) {
    digitalWrite(13, !digitalRead(13));
    return EXECUTED;
}
```

Arguments supplied by the BDI mind are available through `ActionArgs`:

```cpp
Act(motor, machine) {

    if (!ActionArgs.isString(0))
        return INVALID;

    if (strMotorStatus == ActionArgs.asString(0))
        return ALREADY;

    if (ActionArgs.asString(0) == "goAhead")
        goAheadFunction();

    if (ActionArgs.asString(0) == "stopRightNow")
        stopRightNow();

    return EXECUTED;
}
```

Act is distinct from `Behaving`: an Act may establish a bodily state or initiate a behavior whose continuation occurs through Behaving.

---

## Sensing

`Sensing` represents a continuous body-side sensing process associated with an Element.

It executes during every embodiment cycle, before `inhabitance()`, and may acquire or update bodily state independently of whether the BDI mind requests a Percept.

```cpp
Sensing(sensor) {
    temperature = analogRead(A0);
}
```

---

## Behaving

`Behaving` represents a continuous body-side behavioral process associated with an Element.

It executes during every embodiment cycle, after `inhabitance()`, allowing physical behavior initiated by an Act, or arising from the body's own operation, to persist and evolve independently of new deliberative commands.

```cpp
Behaving(motor) {
    if (running) {
        // continue bodily behavior
    }
}
```

---

## Embodiment Cycle

The runtime of an Apparatus is coordinated by `embody()`:

```cpp
void loop() {
    myApparatus.embody();
}
```

Each embodiment cycle executes:

```text
sensing();
inhabitance();
behaving();
```

The body first updates its physical state through Sensing. `inhabitance()` then maintains the operational interaction between body and BDI mind. Finally, Behaving allows bodily processes to continue in the physical world.

---

## Trieb (Embodied Drives)

`Trieb` represents a body-originated drive associated with an Element.

A Trieb expresses a bodily demand toward the BDI mind and carries a `Drang` value representing its intensity. It is not itself a BDI desire, goal or intention; it allows bodily conditions to participate in the cognitive process without placing deliberation inside the body.

Within `Act` and `Percept`, the callable object `trieb` is automatically available:

```cpp
Percept(sensor, temperature, INTEROCEPTION) {

    if (temperature > 70)
        trieb("coolDown", 0.85);

    return temperature;
}
```

A Trieb generated by an Element is conveyed through its Apparatus to the BDI mind.

---

## Tacit Knowledge (Know-How)

`TacitKnowledge` represents practical know-how associated with bodily capabilities.

It allows knowledge associated with the physical constitution of an Apparatus to be exposed to the BDI mind through `getKnowHow`.

```cpp
TacitKnowledge(
    startMovement,
    "+!start <- .myBody.act(move)."
);
```

An Apparatus can therefore provide not only physical capabilities, but also the know-how required for the BDI mind to use those capabilities.

---

## Communication Protocol

NECK uses **JSON-SLP**, based on JSON-seq over SLIP over Serial, for the operational interaction between an Apparatus and the BDI mind.

Typical requests include:

```json
{"msg":"getPercepts"}
```

```json
{"msg":"getActions"}
```

```json
{"msg":"getKnowHow"}
```

An Act may be requested directly by its name:

```json
{"msg":"toggleLED"}
```

or with arguments:

```json
{"msg":"machine","args":["goAhead"]}
```

---

## Minimal Example

```cpp
#include <NECK.hpp>   /* https://github.com/chon-group/neck2arduino */

#define LED_PIN 13

/*Apparatus Description*/
Apparatus(arduinoBoard) {
    Element(led);
}

bool blinking = false;
bool ledStatus = false;
unsigned long previousBlink = 0;
void setup() {pinMode(LED_PIN, OUTPUT);}
void loop() {arduinoBoard.embody();}

/* Apparatus configuration*/
Percept(led, ledStatus, PROPRIOCEPTION) {
  if(digitalRead(LED_PIN)) return true;
  return false;
}

Act(led, blinkOn) {
  if (blinking) return ALREADY;

  blinking = true;
  return EXECUTED;
}

Act(led, blinkOff) {
  if (!blinking) return ALREADY;

  blinking = false;
  digitalWrite(LED_PIN, LOW);

  return EXECUTED;
}

Behaving(led) {
  if (blinking && millis() - previousBlink >= 250) {
      previousBlink = millis();
      ledStatus = !ledStatus;
      digitalWrite(LED_PIN, ledStatus);
  }
}
```

---

## License

![](https://i.creativecommons.org/l/by/4.0/88x31.png)

NECK is licensed under a [Creative Commons Attribution 4.0 International License](http://creativecommons.org/licenses/by/4.0/). The licensor cannot revoke these freedoms as long as you follow the license terms:

* **Attribution** — You must give appropriate credit as follows:

Nilson Lazarin, Carlos Pantoja, and Jose Viterbo. 2026.  
*My Body, My Perceptions: A Shift from Computationalism to Embodied Cognition in BDI-agent-based Embedded Systems.*  
In Proc. of the 25th International Conference on Autonomous Agents and Multiagent Systems (AAMAS 2026),  
Paphos, Cyprus, May 25–29, 2026. IFAAMAS, 10 pages.  
https://doi.org/10.65109/QIVX3835
