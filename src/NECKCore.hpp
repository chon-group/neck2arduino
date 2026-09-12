/*
 * ============================================================================
 * NECKCore.hpp
 * ============================================================================
 *
 * Fundamental types and contracts shared by the NECK runtime.
 *
 * This file defines the common data structures used by the embedded body,
 * Apparatus runtime and user-facing DSL. It contains no body execution logic;
 * instead, it establishes the types through which the different NECK
 * components communicate.
 *
 * The definitions provided here include:
 *
 *   - response types produced by Acts and Percepts;
 *   - perception classifications;
 *   - the representation of TacitKnowledge entries;
 *   - strongly typed arguments exchanged between mind and body; and
 *   - the return envelope used by Percepts.
 *
 * This file is an internal component of NECK and is included by NECK.hpp
 * inside namespace NECK before NECKBody.hpp and NECKApparatus.hpp.
 *
 * Dependency order:
 *
 *     NECKCore.hpp
 *          ↓
 *     NECKBody.hpp
 *          ↓
 *     NECKApparatus.hpp
 *
 * ============================================================================
 */


/*
 * Build signature used as part of the Apparatus identifier generation.
 *
 * The compilation date and time make the signature specific to a particular
 * build of the embedded application.
 */
static const char BUILD_SIGNATURE[] =
  __DATE__ " " __TIME__;


/*
 * ============================================================================
 * Runtime contracts
 * ============================================================================
 */


/*
 * Classifies an Element according to its physical role.
 */
enum ElementType : uint8_t {
  SENSOR,
  EFFECTOR
};


/*
 * Result of an Act requested by the BDI mind.
 *
 * EXECUTED
 *   The requested Act was successfully performed.
 *
 * UNABLE
 *   The Act exists, but the Element is currently unable to perform it.
 *
 * ALREADY
 *   The requested condition or effect is already established.
 *
 * REJECTED
 *   The Act was intentionally refused.
 *
 * INVALID
 *   The request or its arguments are invalid for the Act.
 *
 * UNKNOWN
 *   No corresponding Act could be found.
 */
enum ActionResponse : uint8_t {
  EXECUTED,
  UNABLE,
  ALREADY,
  REJECTED,
  INVALID,
  UNKNOWN
};


/*
 * Result of a Percept evaluated by the body.
 *
 * PERCEPTED
 *   The requested information was successfully perceived.
 *
 * UNAVAILABLE
 *   The information is currently unavailable.
 *
 * UNCHANGED
 *   The percept exists, but its relevant value has not changed.
 */
enum PerceptionResponse : uint8_t {
  PERCEPTED,
  UNAVAILABLE,
  UNCHANGED
};


/*
 * Classifies a Percept according to the source of the information exposed
 * to the BDI mind.
 *
 * INTEROCEPTION
 *   Information concerning the internal condition of the body.
 *
 * PROPRIOCEPTION
 *   Information concerning the body's own configuration, movement or state.
 *
 * EXTEROCEPTION
 *   Information concerning the external physical world.
 */
enum PerceptionType : uint8_t {
  INTEROCEPTION,
  PROPRIOCEPTION,
  EXTEROCEPTION
};


/*
 * ============================================================================
 * TacitKnowledge representation
 * ============================================================================
 */


/*
 * Internal representation of one TacitKnowledge entry.
 *
 * skill
 *   Symbolic name of the bodily skill.
 *
 * context
 *   Optional condition under which the corresponding plan is applicable.
 *   A null context represents an unconditional skill.
 *
 * plan
 *   BDI plan associated with the skill.
 *
 * The strings are stored in program memory by the DSL in order to conserve
 * RAM on embedded platforms.
 */
struct TacitEntry {
  const char* skill;
  const char* context;
  const char* plan;
};


/*
 * Ensures that the "neck_tacit" linker section exists even when the
 * application declares no TacitKnowledge.
 *
 * NECKApparatus.hpp uses the boundaries of this section to discover the
 * TacitKnowledge carried by the body at runtime.
 */
__attribute__((used, section("neck_tacit")))
static const TacitEntry __neck_tacit_sentinel = {
  nullptr,
  nullptr,
  nullptr
};


/*
 * ============================================================================
 * Typed arguments
 * ============================================================================
 *
 * NECKArgs provides the argument representation used when values cross the
 * mind-body interface.
 *
 * Arguments preserve their logical type rather than being converted into a
 * single generic representation. This allows Acts and Trieb to distinguish
 * values such as:
 *
 *     true
 *     -7
 *     4000000000
 *     1.25
 *     "neck"
 *
 * Supported types:
 *
 *     ARG_BOOL
 *     ARG_INT
 *     ARG_UINT
 *     ARG_FLOAT
 *     ARG_STRING
 *
 * ============================================================================
 */


/*
 * Type tag associated with one NECK argument.
 */
enum ArgType : uint8_t {
  ARG_BOOL,
  ARG_INT,
  ARG_UINT,
  ARG_FLOAT,
  ARG_STRING
};


/*
 * Stores one typed argument.
 *
 * Numeric and boolean values share a union, while String values are stored
 * separately.
 */
struct ArgValue {

  ArgType type;

  union {
    bool b;
    int32_t i;
    uint32_t u;
    float f;
  } v;

  String s;


  /*
   * Initializes the value as a signed integer containing zero.
   */
  ArgValue()
    : type(ARG_INT) {

    v.i = 0;
  }
};


/*
 * Fixed-capacity collection of typed arguments exchanged by NECK.
 *
 * The collection preserves the type of every argument and provides strict
 * type inspection through isBool(), isInt(), isUInt(), isFloat() and
 * isString().
 *
 * Type accessors such as asInt() and asFloat() assume that the caller has
 * already verified the corresponding type.
 */
class NECKArgs {

public:

  /*
   * Maximum number of arguments carried by a single NECKArgs instance.
   */
  static const uint8_t MAX = 8;


  NECKArgs()
    : _n(0) {}


  /*
   * Removes all currently stored arguments.
   */
  void clear() {
    _n = 0;
  }


  /*
   * Returns the number of stored arguments.
   */
  uint8_t size() const {
    return _n;
  }


  /*
   * Adds a boolean argument.
   *
   * Returns false when the collection has reached MAX capacity.
   */
  bool add(bool x) {

    if (_n >= MAX)
      return false;

    _a[_n].type = ARG_BOOL;
    _a[_n].v.b = x;
    _a[_n].s = "";

    _n++;

    return true;
  }


  /*
   * Adds a signed 32-bit integer argument.
   */
  bool add(int32_t x) {

    if (_n >= MAX)
      return false;

    _a[_n].type = ARG_INT;
    _a[_n].v.i = x;
    _a[_n].s = "";

    _n++;

    return true;
  }


  /*
   * Convenience overload for native int values.
   */
  bool add(int x) {
    return add((int32_t)x);
  }


  /*
   * Adds a floating-point argument.
   */
  bool add(float x) {

    if (_n >= MAX)
      return false;

    _a[_n].type = ARG_FLOAT;
    _a[_n].v.f = x;
    _a[_n].s = "";

    _n++;

    return true;
  }


  /*
   * Adds a C string as ARG_STRING.
   *
   * This explicit overload prevents string literals from being accidentally
   * resolved to the boolean overload.
   *
   * A null pointer is represented as an empty String.
   */
  bool add(const char* x) {

    if (_n >= MAX)
      return false;

    _a[_n].type = ARG_STRING;
    _a[_n].s =
      (x == nullptr)
        ? String("")
        : String(x);

    _n++;

    return true;
  }


  /*
   * Accepts double literals without overload ambiguity.
   *
   * NECK stores floating-point arguments as float.
   */
  bool add(double x) {
    return add((float)x);
  }


  /*
   * Adds an Arduino String as ARG_STRING.
   */
  bool add(const String& x) {

    if (_n >= MAX)
      return false;

    _a[_n].type = ARG_STRING;
    _a[_n].s = x;

    _n++;

    return true;
  }


  /*
   * Adds an unsigned 32-bit integer argument.
   */
  bool add(uint32_t x) {

    if (_n >= MAX)
      return false;

    _a[_n].type = ARG_UINT;
    _a[_n].v.u = x;
    _a[_n].s = "";

    _n++;

    return true;
  }


  /*
   * Strict type inspection.
   *
   * These methods return false both when the index is outside the current
   * argument range and when the stored argument has a different type.
   */
  bool isBool(uint8_t idx) const {
    return idx < _n &&
           _a[idx].type == ARG_BOOL;
  }

  bool isInt(uint8_t idx) const {
    return idx < _n &&
           _a[idx].type == ARG_INT;
  }

  bool isUInt(uint8_t idx) const {
    return idx < _n &&
           _a[idx].type == ARG_UINT;
  }

  bool isFloat(uint8_t idx) const {
    return idx < _n &&
           _a[idx].type == ARG_FLOAT;
  }

  bool isString(uint8_t idx) const {
    return idx < _n &&
           _a[idx].type == ARG_STRING;
  }


  /*
   * Typed accessors.
   *
   * The caller is expected to verify the type and index before accessing the
   * stored value.
   */
  bool asBool(uint8_t idx) const {
    return _a[idx].v.b;
  }

  int32_t asInt(uint8_t idx) const {
    return _a[idx].v.i;
  }

  uint32_t asUInt(uint8_t idx) const {
    return _a[idx].v.u;
  }

  float asFloat(uint8_t idx) const {
    return _a[idx].v.f;
  }

  String asString(uint8_t idx) const {
    return _a[idx].s;
  }


  /*
   * Provides direct read-only access to one typed argument.
   *
   * Used internally when values must be serialized while preserving their
   * original types.
   */
  const ArgValue& at(uint8_t idx) const {
    return _a[idx];
  }


private:

  ArgValue _a[MAX];
  uint8_t _n;
};


/*
 * ============================================================================
 * Percept return envelope
 * ============================================================================
 */


/*
 * Result produced by a Percept.
 *
 * A Percept may return:
 *
 *   - only a PerceptionResponse;
 *   - one typed value;
 *   - multiple values represented by NECKArgs; or
 *   - no explicit value, in which case PERCEPTED is assumed.
 *
 * hasArgs indicates whether the Percept produced values to be transmitted
 * toward the BDI mind.
 *
 * Constructors intentionally allow common primitive values to be returned
 * directly from a Percept declaration.
 *
 * Examples:
 *
 *     return 23.5f;
 *     return true;
 *     return UNAVAILABLE;
 *
 * or:
 *
 *     NECKArgs values;
 *     values.add(true);
 *     values.add((int32_t)-7);
 *     return values;
 */
struct PerceptReturn {

  PerceptionResponse status;
  bool hasArgs;
  NECKArgs args;


  /*
   * A Percept with no explicit value is considered successfully perceived.
   */
  PerceptReturn()
    : status(PERCEPTED),
      hasArgs(false) {}


  /*
   * Creates a result containing only a perception status.
   */
  PerceptReturn(PerceptionResponse r)
    : status(r),
      hasArgs(false) {}


  /*
   * Creates a successful Percept containing multiple typed arguments.
   */
  PerceptReturn(const NECKArgs& a)
    : status(PERCEPTED),
      hasArgs(true),
      args(a) {}


  /*
   * Convenience constructors for single-valued Percepts.
   *
   * Each value is wrapped in NECKArgs while preserving its corresponding
   * argument type.
   */
  PerceptReturn(bool x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }


  PerceptReturn(int32_t x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }


  PerceptReturn(int x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }


  PerceptReturn(uint32_t x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }


  PerceptReturn(float x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }


  PerceptReturn(const String& x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }


  PerceptReturn(const char* x)
    : status(PERCEPTED),
      hasArgs(true) {

    args.add(x);
  }
};