#include <NECK.hpp>   /* https://github.com/chon-group/neck2arduino */

Apparatus(validationBody) {
  Element(sensor);
  Element(actuator);
}

/* --------------------------------------------------------------------------
   Test state
   -------------------------------------------------------------------------- */

uint32_t sensingCycles  = 0;
uint32_t behavingCycles = 0;

bool actuatorRunning = false;
uint32_t movementSteps = 0;

/* --------------------------------------------------------------------------
   Arduino lifecycle
   -------------------------------------------------------------------------- */
void setup() {}
void loop() {validationBody.embody();}

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

// EXTEROCEPTION + float + trieb inside Percept
Percept(sensor, externalValue, EXTEROCEPTION) {
  trieb("perceptObserved", 0.25);
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

// PerceptionResponse variants
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

// Every NECKArgs input type + trieb carrying args
Act(actuator, testTypes) {
  if (ActionArgs.size() != 5)
    return INVALID;

  if (!ActionArgs.isBool(0))   return INVALID;
  if (!ActionArgs.isInt(1))    return INVALID;
  if (!ActionArgs.isUInt(2))   return INVALID;
  if (!ActionArgs.isFloat(3))  return INVALID;
  if (!ActionArgs.isString(4)) return INVALID;

  trieb("typedImpulse", ActionArgs, 0.5);

  return EXECUTED;
}

// Default drang + saturation
Act(actuator, testTrieb) {
  trieb("attention");
  trieb("attention", 0.111);
  trieb("attention", 0);
  trieb("attention", -0.7);
  trieb("attention", 5);

  return EXECUTED;
}

// Other ActionResponse values
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
