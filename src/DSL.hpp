/*
 * ============================================================================
 * DSL.hpp
 * ============================================================================
 *
 * User-facing Domain-Specific Language (DSL) for describing an embedded
 * body with NECK.
 *
 * The NECK DSL allows the physical constitution and bodily processes of an
 * agent to be described directly in an Arduino/C++ application using the
 * concepts introduced by the NECK body model:
 *
 *     Apparatus
 *     Element
 *     Act
 *     Percept
 *     Trieb
 *     Sensing
 *     Behaving
 *     TacitKnowledge
 *
 * Conceptually:
 *
 *                  ┌────── Embodied Agent ──────┐
 *                  │                            │
 *                  │           Mind             │
 *                  │          (BDI)             │
 *                  │            │               │
 *                  │ Percept ↑  │  ↓ Act        │
 *                  │ Trieb   ↑  │               │
 *                  │            │               │
 *                  │      ┌──── Body ────┐      │
 *                  │      │              │      │
 *                  │      │  Apparatus   │      │
 *                  │      │      │       │      │
 *                  │      │   Element    │      │
 *                  │      └──────┬───────┘      │
 *                  └─────────────┼──────────────┘
 *                     Sensing ↑  │  ↓ Behaving
 *                                │
 *                         Physical World
 *
 * Percept
 *   Declares information that an Element can expose to the BDI mind when
 *   requested. A Percept returns a perceived value, a set of values, or a
 *   Response describing the result of the request.
 *
 * Act
 *   Declares an intentional bodily operation requested by the BDI mind and
 *   performed through an Element. An Act returns a Response indicating
 *   the outcome of the requested operation.
 *
 * Trieb
 *   Declares a bodily demand originating from an Element. A Trieb returns
 *   a numeric value representing its intensity (Drang), between 0 and 1.
 *   A value greater than 0 causes the Trieb to be transmitted to the BDI
 *   mind, while 0 or false suppresses its transmission.
 *
 *   Trieb declarations are evaluated during enhancing(), independently
 *   of Percept requests and Act executions.
 *
 * Sensing
 *   Declares a continuous sensing process performed by an Element. Sensing
 *   executes during the embodiment cycle and may acquire or update bodily
 *   state independently of whether the BDI mind requests a Percept.
 *
 * Behaving
 *   Declares a continuous behavioral process performed by an Element.
 *   Behaving executes during the embodiment cycle and allows bodily behavior
 *   initiated by an Act, or arising from the body's own operation, to persist
 *   and evolve independently of new deliberative commands.
 *
 * TacitKnowledge
 *   Declares practical know-how associated with bodily capabilities. Tacit
 *   knowledge can be exposed to the BDI mind through getKnowHow.
 *
 * The embodiment cycle follows this execution order:
 *
 *     sensing()
 *     enhancing()
 *     behaving()
 *
 * The DSL allows these concepts to be expressed using declarations such as:
 *
 *     Apparatus(robot) {
 *       Element(sensor);
 *       Element(motor);
 *     }
 *
 *     Sensing(sensor) {
 *       temp = analogRead(A0);
 *     }
 *
 *     Percept(sensor, temperature, EXTEROCEPTION) {
 *       return temp;
 *     }
 *
 *     Trieb(sensor, attention) {
 *       if (temp > THRESHOLD)
 *         return 0.8;
 *       return false;
 *     }
 *
 *     Act(motor, move) {
 *       ...
 *       return EXECUTED;
 *     }
 *
 *     Behaving(motor) {
 *       ...
 *     }
 *
 *     TacitKnowledge(
 *       startMovement,
 *       "+!start <- .myBody.act(move)."
 *     );
 *
 * The macros declared in this file translate these body-level declarations
 * into the internal runtime structures implemented by NECKBody.hpp and
 * Apparatus.hpp.
 *
 * Users normally do not include this file directly. It is included by
 * NECK.hpp after the NECK namespace has been closed so the DSL constructs
 * can be used directly in application code.
 *
 * ============================================================================
 */

#ifndef NECK_NO_GLOBAL_USING

using NECK::NECKArgs;
using NECK::PerceptReturn;

using NECK::Response;
using NECK::PerceptionType;


using NECK::EXECUTED;
using NECK::UNABLE;
using NECK::ALREADY;
using NECK::REJECTED;
using NECK::INVALID;
using NECK::UNKNOWN;

using NECK::PERCEPTED;
using NECK::UNAVAILABLE;
using NECK::UNCHANGED;

using NECK::INTEROCEPTION;
using NECK::PROPRIOCEPTION;
using NECK::EXTEROCEPTION;

#endif


/*
 * ============================================================================
 * Internal macro helpers
 * ============================================================================
 *
 * The following concatenation helpers generate unique internal symbols used
 * by the DSL implementation.
 *
 * They are implementation details and are not intended to be used directly
 * by NECK applications.
 * ============================================================================
 */

#define NECK_CONCAT_INNER(a,b) a##b
#define NECK_CONCAT(a,b) NECK_CONCAT_INNER(a,b)

/*
 * ============================================================================
 * Apparatus
 * ============================================================================
 *
 * Declares the Apparatus implemented by the current embedded device and
 * defines its execution lifecycle.
 *
 * Example:
 *
 *     Apparatus(robot) {
 *       Element(camera);
 *       Element(motor);
 *     }
 *
 * The declaration creates a NECK::Apparatus instance and opens the internal
 * namespace in which the Elements belonging to that Apparatus are declared.
 *
 * The Arduino execution lifecycle is managed internally by NECK:
 *
 *   setup()
 *     Executes the optional Preparation block, initializes serial
 *     communication, and starts the embodiment process.
 *
 *   loop()
 *     Continuously executes the body cycle through bodyCycle().
 *
 * Consequently, the embedded application does not need to explicitly
 * declare or manage the Arduino setup() and loop() functions.
 *
 * The intentionally repeated internal symbol
 *
 *     __neck_only_one_Apparatus_per_Microcontroller
 *
 * causes compilation to fail if more than one Apparatus is declared in the
 * same embedded application.
 *
 * This reflects the current embedded NECK model in which one microcontroller
 * implements one Apparatus.
 * ============================================================================
 */

#define Apparatus(NAME)                                 \
  bool __neck_only_one_Apparatus_per_Microcontroller;   \
  NECK::Apparatus NAME(#NAME);                          \
  void setup() {                                        \
    NECK::Apparatus::prepare();                          \
    Serial.begin(115200);                               \
    NAME.embody();                                      \
  }                                                     \
  void loop() { NAME.bodyCycle(); }                     \
  namespace __neck_body


/*
 * ============================================================================
 * Element
 * ============================================================================
 *
 * Declares an Element belonging to the current Apparatus.
 *
 * Example:
 *
 *     Apparatus(robot) {
 *       Element(sensor);
 *       Element(motor);
 *     }
 *
 * An Element is associated with the currently instantiated Apparatus and
 * becomes the anchor through which Act, Percept, Trieb, Sensing and Behaving
 * declarations are registered.
 *
 * Referencing an undeclared Element from one of these declarations causes a
 * compilation error because no corresponding symbol exists in __neck_body.
 * ============================================================================
 */

#define Element(EL) \
  NECK::ElementDef EL(#EL, NECK::Apparatus::instance())


/*
 * ============================================================================
 * Act
 * ============================================================================
 *
 * Declares an intentional bodily operation that may be requested by the BDI
 * mind through an Element.
 *
 * Syntax:
 *
 *     Act(ElementName, ActName) {
 *       ...
 *       return EXECUTED;
 *     }
 *
 * Example:
 *
 *     Act(motor, setRunning) {
 *
 *       if (!ActionArgs.isBool(0))
 *         return INVALID;
 *
 *       bool running = ActionArgs.asBool(0);
 *
 *       ...
 *
 *       return EXECUTED;
 *     }
 *
 * Inside an Act declaration, ActionArgs provides the arguments supplied by
 * the BDI mind when requesting the operation.
 *
 * Internally, this macro creates the Act callback and registers it with
 * the Apparatus during static initialization.
 *
 * The Element must previously have been declared with Element(...).
 * ============================================================================
 */

#define Act(EL, ACTNAME) \
  static NECK::Response \
    NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
    (const NECK::NECKArgs& ActionArgs); \
  \
  struct NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) { \
    NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))() { \
      if ((__neck_body::EL).apparatus) \
        (__neck_body::EL).apparatus->addAction( \
          #EL, \
          #ACTNAME, \
          &NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
        ); \
    } \
  }; \
  \
  static NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
    NECK_CONCAT(__neck_action_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))); \
  \
  static NECK::Response \
    NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
    (const NECK::NECKArgs& ActionArgs)

// #define Act(EL, ACTNAME) \
//   static NECK::Response \
//     NECK_CONCAT(__neck_action_body_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
//     (const NECK::NECKArgs& ActionArgs, NECK::Trieb trieb); \
//   \
//   static NECK::Response \
//     NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
//     (const NECK::NECKArgs& ActionArgs); \
//   \
//   struct NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) { \
//     NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))() { \
//       if ((__neck_body::EL).apparatus) \
//         (__neck_body::EL).apparatus->addAction( \
//           #EL, \
//           #ACTNAME, \
//           &NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
//         ); \
//     } \
//   }; \
//   \
//   static NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
//     NECK_CONCAT(__neck_action_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))); \
//   \
//   static NECK::Response \
//     NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
//     (const NECK::NECKArgs& ActionArgs) { \
//       return NECK_CONCAT(__neck_action_body_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))( \
//         ActionArgs, \
//         NECK::Trieb(&(__neck_body::EL)) \
//       ); \
//     } \
//   \
//   static NECK::Response \
//     NECK_CONCAT(__neck_action_body_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
//     (const NECK::NECKArgs& ActionArgs, NECK::Trieb trieb)


/*
 * ============================================================================
 * Percept
 * ============================================================================
 *
 * Declares information that an Element can expose to the BDI mind.
 *
 * Syntax:
 *
 *     Percept(ElementName, PerceptName, PerceptionType) {
 *       ...
 *       return value;
 *     }
 *
 * Example:
 *
 *     Percept(sensor, temperature, EXTEROCEPTION) {
 *       return 23.5f;
 *     }
 *
 * PerceptionType must be one of:
 *
 *     INTEROCEPTION
 *     PROPRIOCEPTION
 *     EXTEROCEPTION
 *
 * A Percept may return:
 *
 *   - a primitive value supported by PerceptReturn;
 *   - NECKArgs containing multiple values;
 *   - a Response such as UNAVAILABLE or UNCHANGED.
 *
 * A Percept provides information about bodily or environmental conditions.
 * It is distinct from Trieb, which represents a bodily demand and returns
 * its intensity (Drang).
 *
 * A Percept is evaluated when requested by the BDI mind. It is therefore
 * distinct from Sensing, which executes continuously as part of the body
 * cycle.
 *
 * Internally, this macro creates the Percept callback and registers it with
 * the Apparatus during static initialization.
 * ============================================================================
 */

// #define Percept(EL, PERCEPT, TYPE) \
//   static NECK::PerceptReturn \
//     NECK_CONCAT(__neck_percept_body_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
//     (NECK::Trieb trieb); \
//   \
//   static NECK::PerceptReturn \
//     NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))(); \
//   \
//   struct NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) { \
//     NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))() { \
//       if ((__neck_body::EL).apparatus) \
//         (__neck_body::EL).apparatus->addPerception( \
//           #EL, \
//           #PERCEPT, \
//           NECK::TYPE, \
//           &NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
//         ); \
//     } \
//   }; \
//   \
//   static NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
//     NECK_CONCAT(__neck_percept_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))); \
//   \
//   static NECK::PerceptReturn \
//     NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))() { \
//       return NECK_CONCAT(__neck_percept_body_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))( \
//         NECK::Trieb(&(__neck_body::EL)) \
//       ); \
//     } \
//   \
//   static NECK::PerceptReturn \
//     NECK_CONCAT(__neck_percept_body_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
//     (NECK::Trieb trieb)

#define Percept(EL, PERCEPT, TYPE) \
  static NECK::PerceptReturn \
    NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))(); \
  \
  struct NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) { \
    NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))() { \
      if ((__neck_body::EL).apparatus) \
        (__neck_body::EL).apparatus->addPerception( \
          #EL, \
          #PERCEPT, \
          NECK::TYPE, \
          &NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
        ); \
    } \
  }; \
  \
  static NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
    NECK_CONCAT(__neck_percept_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))); \
  \
  static NECK::PerceptReturn \
    NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))()

/*
 * ============================================================================
 * Sensing
 * ============================================================================
 *
 * Declares the continuous sensing process associated with an Element.
 *
 * Syntax:
 *
 *     Sensing(ElementName) {
 *       ...
 *     }
 *
 * Example:
 *
 *     Sensing(sensor) {
 *       temperature = analogRead(A0);
 *     }
 *
 * Sensing belongs to the Body <-> Physical World relationship. It executes
 * during every embodiment cycle, before enhancing(), and may continuously
 * update bodily state independently of whether the BDI mind requests a
 * Percept.
 *
 * State updated by Sensing may be used by Percept and Trieb declarations.
 *
 * Each Element may declare at most one Sensing process. Declaring Sensing
 * twice for the same Element generates duplicate internal symbols and causes
 * a compile-time error.
 * ============================================================================
 */

#define Sensing(EL) \
  static void NECK_CONCAT(__neck_sensing_fn_, EL)(); \
  struct NECK_CONCAT(__neck_sensing_reg_, EL) { \
    NECK_CONCAT(__neck_sensing_reg_, EL)() { \
      if ((__neck_body::EL).apparatus) \
        (__neck_body::EL).apparatus->addSensing( \
          #EL, \
          &NECK_CONCAT(__neck_sensing_fn_, EL) \
        ); \
    } \
  }; \
  static NECK_CONCAT(__neck_sensing_reg_, EL) \
    NECK_CONCAT(__neck_sensing_reg_instance_, EL); \
  static void NECK_CONCAT(__neck_sensing_fn_, EL)()


/*
 * ============================================================================
 * Behaving
 * ============================================================================
 *
 * Declares the continuous behavioral process associated with an Element.
 *
 * Syntax:
 *
 *     Behaving(ElementName) {
 *       ...
 *     }
 *
 * Example:
 *
 *     Behaving(motor) {
 *
 *       if (running) {
 *         ...
 *       }
 *     }
 *
 * Behaving belongs to the Body <-> Physical World relationship. It executes
 * during every embodiment cycle after enhancing().
 *
 * Behaving allows bodily behavior to persist and evolve independently of new
 * deliberative commands from the BDI mind. An Act may therefore establish a
 * bodily state or initiate a behavior whose continuation occurs through
 * Behaving.
 *
 * Each Element may declare at most one Behaving process. Declaring Behaving
 * twice for the same Element generates duplicate internal symbols and causes
 * a compile-time error.
 * ============================================================================
 */

#define Behaving(EL) \
  static void NECK_CONCAT(__neck_behaving_fn_, EL)(); \
  struct NECK_CONCAT(__neck_behaving_reg_, EL) { \
    NECK_CONCAT(__neck_behaving_reg_, EL)() { \
      if ((__neck_body::EL).apparatus) \
        (__neck_body::EL).apparatus->addBehaving( \
          #EL, \
          &NECK_CONCAT(__neck_behaving_fn_, EL) \
        ); \
    } \
  }; \
  static NECK_CONCAT(__neck_behaving_reg_, EL) \
    NECK_CONCAT(__neck_behaving_reg_instance_, EL); \
  static void NECK_CONCAT(__neck_behaving_fn_, EL)()


/*
 * ============================================================================
 * TacitKnowledge
 * ============================================================================
 *
 * Declares knowledge associated with bodily capabilities.
 *
 * TacitKnowledge represents know-how that accompanies the physical body and
 * can be exposed to the BDI mind through getKnowHow.
 *
 * Two forms are supported:
 *
 *     TacitKnowledge(NAME, PLAN);
 *
 * and:
 *
 *     TacitKnowledge(NAME, CONTEXT, PLAN);
 *
 * Example:
 *
 *     TacitKnowledge(
 *       startMovement,
 *       "+!start <- .myBody.act(setRunning(true))."
 *     );
 *
 * Example with context:
 *
 *     TacitKnowledge(
 *       safeMovement,
 *       "battery(Level) & Level > 20",
 *       "+!start <- .myBody.act(setRunning(true))."
 *     );
 *
 * The skill name, context and plan are stored in program memory. Each entry is
 * placed in the "neck_tacit" linker section, allowing Apparatus to discover
 * the complete set of TacitKnowledge entries carried by the body at runtime.
 *
 * A missing context is represented internally as nullptr and therefore
 * corresponds to an unconditional skill.
 * ============================================================================
 */


/*
 * Internal implementation for:
 *
 *     TacitKnowledge(NAME, PLAN)
 */
#define NECK_TK2(NAME, PLAN) \
  static const char NECK_CONCAT(__neck_tk_name_, __LINE__)[] PROGMEM = #NAME; \
  static const char NECK_CONCAT(__neck_tk_plan_, __LINE__)[] PROGMEM = PLAN; \
  __attribute__((used, section("neck_tacit"))) \
  static const NECK::TacitEntry NECK_CONCAT(__neck_tk_entry_, __LINE__) = { \
    NECK_CONCAT(__neck_tk_name_, __LINE__), \
    nullptr, \
    NECK_CONCAT(__neck_tk_plan_, __LINE__) \
  }


/*
 * Internal implementation for:
 *
 *     TacitKnowledge(NAME, CONTEXT, PLAN)
 */
#define NECK_TK3(NAME, CTX, PLAN) \
  static const char NECK_CONCAT(__neck_tk_name_, __LINE__)[] PROGMEM = #NAME; \
  static const char NECK_CONCAT(__neck_tk_ctx_, __LINE__)[] PROGMEM = CTX; \
  static const char NECK_CONCAT(__neck_tk_plan_, __LINE__)[] PROGMEM = PLAN; \
  __attribute__((used, section("neck_tacit"))) \
  static const NECK::TacitEntry NECK_CONCAT(__neck_tk_entry_, __LINE__) = { \
    NECK_CONCAT(__neck_tk_name_, __LINE__), \
    NECK_CONCAT(__neck_tk_ctx_, __LINE__), \
    NECK_CONCAT(__neck_tk_plan_, __LINE__) \
  }


/*
 * Selects the appropriate TacitKnowledge implementation according to the
 * number of supplied arguments.
 */
#define NECK_GET_4TH_ARG(_1,_2,_3,_4,...) _4

#define TacitKnowledge(...) \
  NECK_GET_4TH_ARG(__VA_ARGS__, NECK_TK3, NECK_TK2, _NA)(__VA_ARGS__)


/*
 * ============================================================================
 * Preparation
 * ============================================================================
 *
 * Declares an optional initialization routine for the Apparatus.
 *
 * Syntax:
 *
 *     Preparation {
 *       ...
 *     }
 *
 * Example:
 *
 *     Preparation {
 *       pinMode(LED_PIN, OUTPUT);
 *       pinMode(SENSOR_PIN, INPUT);
 *     }
 *
 * Preparation defines the hardware initialization required by the body,
 * such as pin configuration, sensor initialization, actuator attachment,
 * and other apparatus-specific setup operations.
 *
 * Unlike Sensing and Behaving, which execute continuously during the
 * embodiment cycle, Preparation executes only once during the Arduino
 * setup phase, before the Apparatus begins its embodiment process.
 *
 * Preparation is optional. When it is not declared, no user-defined
 * preparation routine is executed.
 *
 * Internally, this macro:
 *
 *   1. creates the function containing the user-defined preparation code;
 *   2. registers the function with Apparatus through setPreparation()
 *      during static initialization;
 *   3. makes the function available for execution through prepare(),
 *      which is called by the Arduino setup() generated by the DSL.
 *
 * Only one Preparation block may be declared per embedded application.
 * ============================================================================
 */

#define Preparation                                      \
    void __neck_user_preparation();                      \
    static struct __neck_preparation_registrar {         \
        __neck_preparation_registrar() {                 \
            NECK::Apparatus::setPreparation(             \
                __neck_user_preparation                  \
            );                                           \
        }                                                \
    } __neck_preparation_instance;                       \
    void __neck_user_preparation()

/*
 * ============================================================================
 * Trieb
 * ============================================================================
 *
 * Declares a bodily demand originating from an Element.
 *
 * Syntax:
 *
 *     Trieb(ElementName, TriebName) {
 *       ...
 *       return drang;
 *     }
 *
 * Example:
 *
 *     Trieb(bladder, fullBladder) {
 *       if (bladderLevel >= THRESHOLD)
 *         return (bladderLevel - THRESHOLD) / (1.0f - THRESHOLD);
 *
 *       return false;
 *     }
 *
 * A Trieb represents a bodily demand, while its returned value represents
 * the intensity of that demand (Drang).
 *
 * The return value must be between 0.0 and 1.0:
 *
 *   0.0 or false
 *     The Trieb is not transmitted.
 *
 *   0.0 < drang <= 1.0
 *     The Trieb is transmitted with the returned intensity.
 *
 * Trieb declarations are evaluated during streamTriebs(), called by
 * enhancing() in the embodiment cycle.
 *
 * Since sensing() executes before enhancing(), a Trieb may use bodily state
 * updated by Sensing. However, a Trieb does not require a Sensing or Percept
 * declaration to exist.
 *
 * Multiple Trieb declarations may be associated with the same Element,
 * provided that each has a distinct name.
 *
 * Internally, this macro:
 *
 *   1. creates a callback returning the Drang as a double;
 *   2. creates a registrar associated with the originating Element;
 *   3. registers the callback with the Apparatus during static
 *      initialization.
 *
 * The Element must previously have been declared with Element(...).
 * ============================================================================
 */


#define Trieb(EL, NAME) \
  static double \
    NECK_CONCAT(__neck_trieb_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME)))(); \
  \
  struct NECK_CONCAT(__neck_trieb_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME))) { \
    NECK_CONCAT(__neck_trieb_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME)))() { \
      if ((__neck_body::EL).apparatus) \
        (__neck_body::EL).apparatus->addTrieb( \
          #EL, \
          #NAME, \
          &NECK_CONCAT(__neck_trieb_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME))) \
        ); \
    } \
  }; \
  \
  static NECK_CONCAT(__neck_trieb_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME))) \
    NECK_CONCAT(__neck_trieb_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME))); \
  \
  static double \
    NECK_CONCAT(__neck_trieb_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, NAME)))()

