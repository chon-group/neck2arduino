/*
 * ============================================================================
 * NECKDSL.hpp
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
 *                  │   Percept ↑│↓ Act          │
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
 *   PerceptionResponse describing the result of the request.
 *
 * Act
 *   Declares an intentional bodily operation requested by the BDI mind and
 *   performed through an Element. An Act returns an ActionResponse indicating
 *   the outcome of the requested operation.
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
 * NECKApparatus.hpp.
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

using NECK::ActionResponse;
using NECK::PerceptionResponse;
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
 * Declares the Apparatus implemented by the current embedded device.
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

#define Apparatus(NAME)                                        \
  static bool __neck_only_one_Apparatus_per_Microcontroller;   \
  NECK::Apparatus NAME(#NAME);                                 \
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
 * becomes the anchor through which Act, Percept, Sensing and Behaving
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
 *       bool running =
 *         ActionArgs.asBool(0);
 *
 *       ...
 *
 *       return EXECUTED;
 *     }
 *
 * Inside an Act declaration, two objects are automatically available:
 *
 *   ActionArgs
 *     Arguments supplied by the BDI mind when requesting the Act.
 *
 *   trieb
 *     Callable object through which the body may emit a Trieb toward the
 *     BDI mind.
 *
 * Example:
 *
 *     trieb("attention", 0.5);
 *
 * Internally, this macro:
 *
 *   1. creates the function containing the user-provided Act body;
 *   2. creates a runtime wrapper with the ActionFn signature;
 *   3. creates a Trieb bound to the originating Element;
 *   4. registers the Act with its Apparatus during static initialization.
 *
 * The Element must previously have been declared with Element(...).
 * ============================================================================
 */

#define Act(EL, ACTNAME) \
  static NECK::ActionResponse \
    NECK_CONCAT(__neck_action_body_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
    (const NECK::NECKArgs& ActionArgs, NECK::Trieb trieb); \
  \
  static NECK::ActionResponse \
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
  static NECK::ActionResponse \
    NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
    (const NECK::NECKArgs& ActionArgs) { \
      return NECK_CONCAT(__neck_action_body_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))( \
        ActionArgs, \
        NECK::Trieb(&(__neck_body::EL)) \
      ); \
    } \
  \
  static NECK::ActionResponse \
    NECK_CONCAT(__neck_action_body_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) \
    (const NECK::NECKArgs& ActionArgs, NECK::Trieb trieb)


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
 *   - a PerceptionResponse such as UNAVAILABLE or UNCHANGED.
 *
 * Inside a Percept declaration, trieb is automatically available. This allows
 * a Percept evaluation to produce a body-originated drive when appropriate.
 *
 * Example:
 *
 *     Percept(sensor, danger, EXTEROCEPTION) {
 *       trieb("attention", 0.8);
 *       return true;
 *     }
 *
 * A Percept is evaluated when requested by the BDI mind. It is therefore
 * distinct from Sensing, which executes continuously as part of the body
 * cycle.
 *
 * Internally, this macro creates the Percept callback and registers it with
 * the Apparatus during static initialization.
 * ============================================================================
 */

#define Percept(EL, PERCEPT, TYPE) \
  static NECK::PerceptReturn \
    NECK_CONCAT(__neck_percept_body_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
    (NECK::Trieb trieb); \
  \
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
    NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))() { \
      return NECK_CONCAT(__neck_percept_body_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))( \
        NECK::Trieb(&(__neck_body::EL)) \
      ); \
    } \
  \
  static NECK::PerceptReturn \
    NECK_CONCAT(__neck_percept_body_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) \
    (NECK::Trieb trieb)


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
 * during every embodiment cycle, before inhabitance(), and may continuously
 * update bodily state independently of whether the BDI mind requests a
 * Percept.
 *
 * Sensing does not expose trieb. A Trieb is intentionally available only
 * within Act and Percept declarations, where the body-mind interaction takes
 * place.
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
 * during every embodiment cycle after inhabitance().
 *
 * Behaving allows bodily behavior to persist and evolve independently of new
 * deliberative commands from the BDI mind. An Act may therefore establish a
 * bodily state or initiate a behavior whose continuation occurs through
 * Behaving.
 *
 * Behaving does not expose trieb. A Trieb is intentionally available only
 * within Act and Percept declarations.
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

