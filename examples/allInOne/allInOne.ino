#include <NECK.hpp>   /* https://github.com/chon-group/neck2arduino */

#define LED_PIN 13
uint32_t sensingCycles  = 0;
uint32_t behavingCycles = 0;
bool actuatorRunning = false;
uint32_t movementSteps = 0;

// Deterministic state used by the standalone Trieb tests.
bool typedImpulseActive = false;
double attentionDrang = 0.0;


Apparatus(validationBody) {
  Element(sensor);
  Element(actuator);
}

Preparation{
  pinMode(LED_PIN, OUTPUT);
}

/* --------------------------------------------------------------------------
   Sensing / Behaving
   -------------------------------------------------------------------------- */

Sensing(sensor) {sensingCycles++;}

Behaving(actuator) {
  behavingCycles++;

  if (actuatorRunning) {
    movementSteps++;
  }
}

/* --------------------------------------------------------------------------
   Percepts
   -------------------------------------------------------------------------- */

// EXTEROCEPTION + float
Percept(sensor, externalValue, EXTEROCEPTION) {
  return 23.5f;
}

// INTEROCEPTION
Percept(sensor, internalLoad, INTEROCEPTION) {
  return 0.50f;
}

// PROPRIOCEPTION + uint32_t
Percept(actuator, movementSteps, PROPRIOCEPTION) {
  return movementSteps;
}

// All supported argument types
Percept(sensor, mixedValues, INTEROCEPTION) {
  NECKArgs values;

  values.add(true);
  values.add((int32_t)-7);
  values.add((uint32_t)4000000000UL);
  values.add(1.25f);
  values.add("neck");

  return values;
}

// Response variants
Percept(sensor, unavailableValue, EXTEROCEPTION) {
  return UNAVAILABLE;
}

Percept(sensor, unchangedValue, INTEROCEPTION) {
  return UNCHANGED;
}

/*
  Regression detector for embody():

        Sensing
           ↓
      communication
           ↓
        Behaving

  During getPercepts(), Sensing has already executed and Behaving
  has not executed yet. Therefore cycleDelta should always be 1.

  If Behaving is accidentally skipped because of a return inside
  protocol handling, repeated requests will produce 1, 2, 3, 4...
*/
Percept(sensor, cycleDelta, PROPRIOCEPTION) {
  return (int32_t)(sensingCycles - behavingCycles);
}

/* --------------------------------------------------------------------------
   Triebs
   -------------------------------------------------------------------------- */

// Independently declared bodily demand, evaluated on getPercepts.
Trieb(sensor, perceptObserved) {
  return 0.25;
}

// Enabled by a successful testTypes action.
Trieb(actuator, typedImpulse) {
  return typedImpulseActive ? 0.5 : 0.0;
}

// Intensity is controlled by testTrieb or setDrang.
Trieb(actuator, attention) {
  return attentionDrang;
}

/* --------------------------------------------------------------------------
   Acts
   -------------------------------------------------------------------------- */

// bool + EXECUTED / ALREADY / INVALID
Act(actuator, setRunning) {
  if (!ActionArgs.isBool(0))
    return INVALID;

  bool requested = ActionArgs.asBool(0);

  if (requested == actuatorRunning)
    return ALREADY;

  actuatorRunning = requested;
  return EXECUTED;
}

// Every NECKArgs input type; successful validation activates a Trieb
Act(actuator, testTypes) {
  if (ActionArgs.size() != 5)
    return INVALID;

  if (!ActionArgs.isBool(0))   return INVALID;
  if (!ActionArgs.isInt(1))    return INVALID;
  if (!ActionArgs.isUInt(2))   return INVALID;
  if (!ActionArgs.isFloat(3))  return INVALID;
  if (!ActionArgs.isString(4)) return INVALID;

  typedImpulseActive = true;

  return EXECUTED;
}

// Activate a registered Trieb with a fractional Drang.
Act(actuator, testTrieb) {
  attentionDrang = 0.111;
  return EXECUTED;
}

// Configure a registered Trieb deterministically for boundary tests.
// Values outside (0, 1] are not transmitted by streamTriebs().
Act(actuator, setDrang) {
  if (ActionArgs.size() != 1 || !ActionArgs.isFloat(0))
    return INVALID;

  attentionDrang = ActionArgs.asFloat(0);
  return EXECUTED;
}

// Restore the initial Trieb test state.
Act(actuator, resetTriebs) {
  typedImpulseActive = false;
  attentionDrang = 0.0;
  return EXECUTED;
}

// Other Response values
Act(actuator, returnUnable) {
  return UNABLE;
}

Act(actuator, returnRejected) {
  return REJECTED;
}

/* --------------------------------------------------------------------------
   Tacit Knowledge
   -------------------------------------------------------------------------- */

TacitKnowledge(startMovement,
  "+!start <- .myBody.act(setRunning(true)).");

TacitKnowledge(safeMovement,
  "battery(Level) & Level > 20",
  "+!start <- .myBody.act(setRunning(true)).");
