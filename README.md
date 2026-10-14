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
                 │ Percept ↑  │  ↓ Act        │
                 │ Trieb   ↑  │               │
                 │            │               │
                 │      ┌──── Body ────┐      │
                 │      │              │      │
                 │      │  Apparatus   │      │
                 │      │      │       │      │
                 │      │   Element    │      │
                 │      └──────┬───────┘      │
                 └──────────────┼──────────────┘
                    Sensing ↑  │  ↓ Behaving
                               │
                        Physical World
```

The Apparatus therefore operates at the intersection of two relationships:

- **Mind ↔ Body** — through `Percept`, `Trieb` and `Act`;
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

The `Apparatus` declaration also generates the Arduino `setup()` and `loop()` functions. **Do not declare them in your sketch**:

- `setup()` runs the optional `Preparation` block, starts the serial port (115200 baud) and waits for the initial embodiment handshake from the BDI mind;
- `loop()` continuously runs the body cycle.

---

## Preparation

`Preparation` is an optional block executed once, during `setup()`, before the embodiment process begins. It is the place for hardware initialization such as pin configuration or sensor setup.

```cpp
Preparation {
    pinMode(LED_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT);
}
```

Only one `Preparation` block may be declared per application.

---

## Percept

A `Percept` represents information that an Element exposes to the BDI mind when requested.

A Percept returns a perceived value, a set of values (`NECKArgs`), or a `Response` such as `UNAVAILABLE` or `UNCHANGED`. Percepts are classified according to their source as `INTEROCEPTION`, `PROPRIOCEPTION` or `EXTEROCEPTION`.

```cpp
Percept(led, ledStatus, PROPRIOCEPTION) {
    return digitalRead(13);
}

Percept(motor, motorStatus, PROPRIOCEPTION) {
    return strMotorStatus;
}
```

Multiple values of different types can be returned at once:

```cpp
Percept(sensor, mixedValues, INTEROCEPTION) {
    NECKArgs values;
    values.add(true);
    values.add((int32_t)-7);
    values.add(1.25f);
    values.add("neck");
    return values;
}
```

Percept is distinct from `Sensing`: Sensing continuously acquires or updates bodily state, whereas a Percept exposes information to the BDI mind when requested.

---

## Act

An `Act` represents an intentional bodily operation requested by the BDI mind and performed through an Element.

An Act returns a `Response` indicating the outcome of the requested operation, such as `EXECUTED`, `UNABLE`, `ALREADY`, `REJECTED` or `INVALID`.

```cpp
Act(led, toggleLED) {
    digitalWrite(13, !digitalRead(13));
    return EXECUTED;
}
```

Arguments supplied by the BDI mind are available through `ActionArgs`, which preserves the types `bool`, `int32_t`, `uint32_t`, `float` and `String`:

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

Each Element may declare at most one `Sensing` process.

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

Each Element may declare at most one `Behaving` process.

---

## Embodiment Cycle

The runtime of an Apparatus is coordinated by the cycle generated in `loop()`:

```text
sensing();
inhabitance();
behaving();
```

The body first updates its physical state through Sensing. `inhabitance()` then maintains the operational interaction between body and BDI mind, processing at most one incoming request without blocking bodily behavior. Finally, Behaving allows bodily processes to continue in the physical world.

---

## Trieb (Embodied Drives)

`Trieb` represents a body-originated drive associated with an Element.

A Trieb expresses a bodily demand toward the BDI mind and carries a `Drang` value representing its intensity. It is not itself a BDI desire, goal or intention; it allows bodily conditions to participate in the cognitive process without placing deliberation inside the body.

A Trieb is declared independently, like a Percept or an Act, and returns its Drang:

```cpp
Trieb(sensor, coolDown) {
    if (temperature > 70)
        return 0.85;

    return false;
}
```

The returned Drang must be within `(0.0, 1.0]` to be transmitted. A value of `0.0` or `false` (or any value outside this interval) suppresses the transmission.

Triebs are evaluated when the BDI mind requests percepts (`getPercepts`) and are streamed in the same transmission, right after the percepts. Since `Sensing` runs before `inhabitance()`, a Trieb may use bodily state updated by Sensing, but it does not require a Sensing or Percept declaration to exist.

Multiple Triebs may be associated with the same Element, provided each has a distinct name.

---

## Tacit Knowledge (Know-How)

`TacitKnowledge` represents practical know-how associated with bodily capabilities.

It allows knowledge associated with the physical constitution of an Apparatus to be exposed to the BDI mind through `getKnowHow`. Entries are stored in program memory and may optionally carry a context.

```cpp
TacitKnowledge(
    startMovement,
    "+!start <- .myBody.act(move)."
);
```

With a context:

```cpp
TacitKnowledge(
    safeMovement,
    "battery(Level) & Level > 20",
    "+!start <- .myBody.act(move)."
);
```

An Apparatus can therefore provide not only physical capabilities, but also the know-how required for the BDI mind to use those capabilities.

---

## Communication Protocol

NECK uses **JSON-SLP** (`JSON_SLP.hpp`) for the operational interaction between an Apparatus and the BDI mind. It is inspired by [RFC 1055](https://www.rfc-editor.org/rfc/rfc1055) (SLIP) and [RFC 7464](https://www.rfc-editor.org/rfc/rfc7464) (JSON text sequences), without implementing either in full, and works over any Arduino `Stream` (NECK uses `Serial`).

A transmission is delimited by `END` (`0xC0`) and carries one or more JSON records, each framed as `RS <JSON> LF` (`0x1E`, `0x0A`):

```text
0xC0
  0x1E {"msg":"first"}  0x0A
  0x1E {"msg":"second"} 0x0A
0xC0
```

> The SLIP escaping mechanism is **not** implemented. An incoming transmission that stays idle for more than 2.2 s is closed by timeout.

### Requests

Typical requests include:

```json
{"msg":"getPercepts"}
```

```json
{"msg":"getActions"}
```

```json
{"msg":"getActions","element":"led"}
```

```json
{"msg":"getKnowHow"}
```

An Act may be requested directly by its name (broadcast to every Element that declares it):

```json
{"msg":"toggleLED"}
```

to a specific Element:

```json
{"msg":"toggleLED","element":"led"}
```

or with arguments:

```json
{"msg":"machine","element":"motor","args":["goAhead"]}
```

### Responses

Every response starts with a header record identifying the request and the Apparatus (`apparatusID` is derived from the build date/time and the Apparatus name):

```json
{"request":"getPercepts","response":"executed","apparatus":"myApparatus","apparatusID":123456789}
```

`getPercepts` then streams one record per Percept, followed by the active Triebs:

```json
{"percept":"ledStatus","element":"led","type":"proprioception","status":"percepted","args":[true]}
{"trieb":"coolDown","element":"sensor","drang":0.85}
```

`getActions` streams one record per Act:

```json
{"action":"toggleLED","element":"led"}
```

`getKnowHow` streams one record per TacitKnowledge entry:

```json
{"skill":"safeMovement","context":"battery(Level) & Level > 20","plan":"+!start <- .myBody.act(move)."}
```

An Act request is answered with its outcome (`executed`, `unable`, `already`, `rejected`, `invalid` or `unknown`):

```json
{"request":"toggleLED","response":"executed","apparatus":"myApparatus","apparatusID":123456789,"element":"led"}
```

---

## Minimal Example

```cpp
#include <NECK.hpp>   /* https://github.com/chon-group/neck2arduino */

#define LED_PIN 13

/* Apparatus description (also generates setup() and loop()) */
Apparatus(arduinoBoard) {
  Element(led);
}

bool blinking = false;
bool ledStatus = false;
unsigned long previousBlink = 0;

/* Hardware initialization */
Preparation {
  pinMode(LED_PIN, OUTPUT);
}

/* Apparatus configuration */
Percept(led, ledStatus, PROPRIOCEPTION) {
  return digitalRead(LED_PIN) ? true : false;
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

Nilson Lazarin, Carlos Pantoja, and Jose Viterbo. 2026. *My Body, My Perceptions: A Shift from Computationalism to Embodied Cognition in BDI-agent-based Embedded Systems.* In Proc. of the 25th International Conference on Autonomous Agents and Multiagent Systems (AAMAS 2026), Paphos, Cyprus, May 25–29, 2026. IFAAMAS, 10 pages. https://doi.org/10.65109/QIVX3835