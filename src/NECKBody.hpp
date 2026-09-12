/*
 * ============================================================================
 * NECKBody.hpp
 * ============================================================================
 *
 * Structural definitions used to represent the embedded body in NECK.
 *
 * In the NECK model, the body is composed of Apparatus, and each Apparatus is
 * composed of Elements. Elements are the bodily units through which physical
 * capabilities and bodily processes are associated with an Apparatus.
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
 * At the embedded level, this file provides the internal structures required
 * to associate bodily processes with Elements:
 *
 *   Act
 *     An intentional bodily operation requested by the BDI mind and performed
 *     through an Element. An Act returns an ActionResponse indicating the
 *     outcome of the requested operation.
 *
 *   Percept
 *     Information that an Element exposes to the BDI mind when requested.
 *     A Percept returns a PerceptReturn containing its current status and,
 *     when available, the perceived value or values.
 *
 *   Sensing
 *     A continuous body-side process through which an Element acquires or
 *     updates information about the body or the physical world.
 *
 *   Behaving
 *     A continuous body-side process through which an Element maintains or
 *     evolves physical behavior.
 *
 *   Trieb
 *     A body-originated drive produced through an Element that exerts a
 *     demand on the BDI mind with an associated Drang intensity.
 *
 * Elements keep a reference to the Apparatus that contains them. This allows
 * bodily processes originating from an Element, such as Trieb, to be routed
 * through the corresponding Apparatus runtime.
 *
 * This file is an internal component of NECK. It is included by NECK.hpp
 * inside namespace NECK and precedes the complete definition of Apparatus.
 *
 * ============================================================================
 */

/*
 * Apparatus is defined later in NECKApparatus.hpp.
 *
 * A forward declaration is sufficient here because ElementDef only stores a
 * pointer to its containing Apparatus. Operations that require the complete
 * Apparatus definition are implemented after Apparatus has been defined.
 */
class Apparatus;


/*
 * ============================================================================
 * Element
 * ============================================================================
 */


/*
 * Internal representation of an Element belonging to an Apparatus.
 *
 * An Element is identified by name and maintains a reference to the
 * Apparatus that contains it.
 *
 * User-facing Element declarations are created by the NECK DSL. ElementDef
 * provides the runtime representation used internally by the library.
 */
struct ElementDef {

  /*
   * Emits a Trieb from this Element through its containing Apparatus.
   *
   * The implementations are placed in NECKApparatus.hpp because sending a
   * Trieb requires access to the complete Apparatus definition.
   */
  void trieb(
    const char* triebName,
    double drang
  );

  /*
   * Emits a Trieb carrying additional arguments from this Element through
   * its containing Apparatus.
   */
  void trieb(
    const char* triebName,
    const NECKArgs& args,
    double drang
  );


  // Symbolic name of this Element.
  const char* name;

  // Apparatus that physically contains this Element.
  Apparatus* apparatus;


  /*
   * Creates an Element associated with a containing Apparatus.
   *
   * Defined after the complete Apparatus declaration.
   */
  ElementDef(
    const char* n,
    Apparatus* a
  );
};


/*
 * ============================================================================
 * Bodily process function types
 * ============================================================================
 *
 * These callback signatures connect DSL declarations to the runtime
 * registries maintained by Apparatus.
 * ============================================================================
 */


/*
 * Function implementing an Act.
 *
 * An Act receives arguments supplied by the BDI mind and returns an
 * ActionResponse describing the outcome of the requested operation.
 */
typedef ActionResponse (*ActionFn)(
  const NECKArgs& ActionArgs
);


/*
 * Function implementing a Percept.
 *
 * A Percept returns its current status and, optionally, one or more values
 * to be exposed to the BDI mind.
 */
typedef PerceptReturn (*PerceptionFn)();


/*
 * Function implementing the continuous Sensing process of an Element.
 */
typedef void (*SensingFn)();


/*
 * Function implementing the continuous Behaving process of an Element.
 */
typedef void (*BehavingFn)();


/*
 * ============================================================================
 * Runtime registry entries
 * ============================================================================
 *
 * Apparatus maintains linked lists of the processes provided by its Elements.
 *
 * Each entry associates a bodily process with the Element that provides it
 * and stores the callback invoked by the Apparatus runtime.
 * ============================================================================
 */


/*
 * Runtime registration of an Act provided by an Element.
 */
struct ActionEntry {

  const char* elementName;
  const char* actionName;

  ActionFn fn;

  ActionEntry* next;


  ActionEntry()
    : elementName(nullptr),
      actionName(nullptr),
      fn(nullptr),
      next(nullptr) {}
};


/*
 * Runtime registration of a Percept provided by an Element.
 *
 * Besides its name and callback, each Percept carries a PerceptionType
 * describing whether it corresponds to interoception, proprioception or
 * exteroception.
 */
struct PerceptionEntry {

  const char* elementName;
  const char* perceptName;

  PerceptionType type;
  PerceptionFn fn;

  PerceptionEntry* next;


  PerceptionEntry()
    : elementName(nullptr),
      perceptName(nullptr),
      type(PROPRIOCEPTION),
      fn(nullptr),
      next(nullptr) {}
};


/*
 * Runtime registration of the Sensing process associated with an Element.
 *
 * NECK allows each Element to provide at most one Sensing declaration at the
 * DSL level.
 */
struct SensingEntry {

  const char* elementName;

  SensingFn fn;

  SensingEntry* next;


  SensingEntry()
    : elementName(nullptr),
      fn(nullptr),
      next(nullptr) {}
};


/*
 * Runtime registration of the Behaving process associated with an Element.
 *
 * NECK allows each Element to provide at most one Behaving declaration at the
 * DSL level.
 */
struct BehavingEntry {

  const char* elementName;

  BehavingFn fn;

  BehavingEntry* next;


  BehavingEntry()
    : elementName(nullptr),
      fn(nullptr),
      next(nullptr) {}
};


/*
 * ============================================================================
 * Trieb
 * ============================================================================
 *
 * Trieb provides the callable object made available inside Act and Percept
 * declarations by the NECK DSL.
 *
 * A Trieb represents a body-originated drive communicated from an Element
 * toward the BDI mind.
 *
 * Its intensity is represented by Drang, constrained to the interval:
 *
 *     0.0 <= Drang <= 1.0
 *
 * Calling Trieb without explicitly providing a Drang uses the maximum
 * intensity (1.0).
 *
 * Examples:
 *
 *     trieb("attention");
 *
 *     trieb("attention", 0.5);
 *
 *     trieb("attention", args, 0.75);
 *
 * The actual transmission is delegated to ElementDef, which forwards the
 * drive through the Apparatus containing the Element.
 *
 * ============================================================================
 */
struct Trieb {

  // Element from which this Trieb originates.
  ElementDef* element;


  explicit Trieb(ElementDef* e)
    : element(e) {}


  /*
   * Emits a Trieb using the default maximum Drang.
   */
  void operator()(
    const char* name
  ) const {

    (*this)(
      name,
      1.0
    );
  }


  /*
   * Emits a Trieb with an explicitly specified Drang.
   *
   * Values outside the valid interval are clamped to [0.0, 1.0].
   */
  void operator()(
    const char* name,
    double drang
  ) const {

    if (!element)
      return;

    if (drang < 0.0)
      drang = 0.0;

    if (drang > 1.0)
      drang = 1.0;

    element->trieb(
      name,
      drang
    );
  }


  /*
   * Emits a Trieb with arguments and an optional Drang.
   *
   * When omitted, Drang defaults to its maximum value (1.0).
   * Values outside the valid interval are clamped to [0.0, 1.0].
   */
  void operator()(
    const char* name,
    const NECKArgs& args,
    double drang = 1.0
  ) const {

    if (!element)
      return;

    if (drang < 0.0)
      drang = 0.0;

    if (drang > 1.0)
      drang = 1.0;

    element->trieb(
      name,
      args,
      drang
    );
  }
};