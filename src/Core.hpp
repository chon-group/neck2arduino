/*
 * ============================================================================
 * NECKCore.hpp
 * ============================================================================
 *
 * Core types, runtime registries, and Apparatus implementation for NECK.
 *
 * This header combines the former core and Apparatus definitions. It provides
 * typed mind-body arguments, Act and Percept results, bodily process
 * registrations, the embodiment lifecycle, and JSON_SLP communication.
 *
 * The body cycle is sensing() -> enhancing() -> behaving().
 * enhancing() processes incoming requests and streams active bodily demands.
 *
 * Included by NECK.hpp inside namespace NECK. The including header must
 * provide the Arduino, ArduinoJson, JSON_SLP, and PROGMEM dependencies.
 * ============================================================================
 */

/* Build-specific component of the Apparatus identifier. */
static const char BUILD_SIGNATURE[] = __DATE__ " " __TIME__;

/* ============================================================================
 * Runtime contracts
 * ============================================================================
 */

using PreparationFn = void (*)();

enum Response : uint8_t {
  EXECUTED,   // Act completed successfully.
  UNABLE,     // Element cannot currently perform the Act.
  ALREADY,    // Requested state is already established.
  REJECTED,   // Act was refused.
  INVALID,    // Request or arguments are invalid.
  UNKNOWN,    // No matching Act exists.
  PERCEPTED,  // Percept was successfully evaluated.
  UNAVAILABLE,// Percept information is unavailable.
  UNCHANGED   // Percept value has not changed.
};

enum PerceptionType : uint8_t {
  INTEROCEPTION,   // Internal bodily state.
  PROPRIOCEPTION,  // Body configuration or movement.
  EXTEROCEPTION    // External physical environment.
};

/* ============================================================================
 * Tacit knowledge
 * ============================================================================
 *
 * DSL entries reside in the "neck_tacit" linker section, in program memory.
 * A null context denotes an unconditional skill. The sentinel guarantees
 * that the section exists even when no skill is declared.
 */
struct TacitEntry {
  const char* skill;
  const char* context;
  const char* plan;
};

__attribute__((used, section("neck_tacit")))
static const TacitEntry __neck_tacit_sentinel = {nullptr, nullptr, nullptr};

/* ============================================================================
 * Typed mind-body arguments
 * ============================================================================
 *
 * NECKArgs preserves the types of up to eight arguments. Accessors assume
 * the caller has checked the index and type with the corresponding is*().
 */
enum ArgType : uint8_t {
  ARG_BOOL, ARG_INT, ARG_UINT, ARG_FLOAT, ARG_STRING
};

struct ArgValue {
  ArgType type;
  union {
    bool b;
    int32_t i;
    uint32_t u;
    float f;
  } v;
  String s;

  ArgValue() : type(ARG_INT) { v.i = 0; }
};

class NECKArgs {
public:
  static const uint8_t MAX = 8;

  NECKArgs() : _n(0) {}

  void clear() { _n = 0; }
  uint8_t size() const { return _n; }

  bool add(bool x) {
    if (!reserve(ARG_BOOL)) return false;
    _a[_n].v.b = x;
    ++_n;
    return true;
  }

  bool add(int32_t x) {
    if (!reserve(ARG_INT)) return false;
    _a[_n].v.i = x;
    ++_n;
    return true;
  }

  // Native int and int32_t are the same type on common 32-bit Arduino boards.
  // Keep only the int32_t overload to avoid duplicate declarations.

  bool add(uint32_t x) {
    if (!reserve(ARG_UINT)) return false;
    _a[_n].v.u = x;
    ++_n;
    return true;
  }

  bool add(float x) {
    if (!reserve(ARG_FLOAT)) return false;
    _a[_n].v.f = x;
    ++_n;
    return true;
  }

  bool add(double x) { return add(static_cast<float>(x)); }

  // Explicit C-string overload prevents string literals becoming booleans.
  bool add(const char* x) { return add(String(x ? x : "")); }

  bool add(const String& x) {
    if (!reserve(ARG_STRING)) return false;
    _a[_n].s = x;
    ++_n;
    return true;
  }

  bool isBool(uint8_t idx) const { return isType(idx, ARG_BOOL); }
  bool isInt(uint8_t idx) const { return isType(idx, ARG_INT); }
  bool isUInt(uint8_t idx) const { return isType(idx, ARG_UINT); }
  bool isFloat(uint8_t idx) const { return isType(idx, ARG_FLOAT); }
  bool isString(uint8_t idx) const { return isType(idx, ARG_STRING); }

  bool asBool(uint8_t idx) const { return _a[idx].v.b; }
  int32_t asInt(uint8_t idx) const { return _a[idx].v.i; }
  uint32_t asUInt(uint8_t idx) const { return _a[idx].v.u; }
  float asFloat(uint8_t idx) const { return _a[idx].v.f; }
  String asString(uint8_t idx) const { return _a[idx].s; }
  const ArgValue& at(uint8_t idx) const { return _a[idx]; }

private:
  bool reserve(ArgType type) {
    if (_n >= MAX) return false;
    _a[_n].type = type;
    _a[_n].s = "";
    return true;
  }

  bool isType(uint8_t idx, ArgType type) const {
    return idx < _n && _a[idx].type == type;
  }

  ArgValue _a[MAX];
  uint8_t _n;
};

/* ============================================================================
 * Percept results
 * ============================================================================
 *
 * PerceptReturn wraps a perception status and optional typed values. Primitive
 * constructors allow DSL Percept blocks to return values directly.
 */
struct PerceptReturn {
  Response status;
  bool hasArgs;
  NECKArgs args;

  PerceptReturn() : status(PERCEPTED), hasArgs(false) {}
  PerceptReturn(Response r) : status(r), hasArgs(false) {}
  PerceptReturn(const NECKArgs& a)
    : status(PERCEPTED), hasArgs(true), args(a) {}

  PerceptReturn(bool x) : PerceptReturn() { set(x); }
  PerceptReturn(int32_t x) : PerceptReturn() { set(x); }
  PerceptReturn(uint32_t x) : PerceptReturn() { set(x); }
  PerceptReturn(float x) : PerceptReturn() { set(x); }
  PerceptReturn(double x) : PerceptReturn() { set(x); }
  PerceptReturn(const String& x) : PerceptReturn() { set(x); }
  PerceptReturn(const char* x) : PerceptReturn() { set(x); }

private:
  template<typename T>
  void set(const T& x) { hasArgs = args.add(x); }
};

/* ============================================================================
 * Element definitions and callback signatures
 * ============================================================================
 *
 * ElementDef stores its owning Apparatus. Its constructor is defined here;
 * its Trieb forwarding methods follow the complete Apparatus definition.
 */
class Apparatus;

struct ElementDef {
  const char* name;
  Apparatus* apparatus;

  ElementDef(const char* n, Apparatus* a) : name(n), apparatus(a) {}

  void trieb(const char* triebName, double drang);
  void trieb(const char* triebName, const NECKArgs& args, double drang);
};

using ActionFn = Response (*)(const NECKArgs&);
using PerceptionFn = PerceptReturn (*)();
using SensingFn = void (*)();
using BehavingFn = void (*)();
using TriebFn = double (*)();

/* ============================================================================
 * Runtime registries
 * ============================================================================
 *
 * Each entry links a DSL callback to the Element that declares it. Entries
 * are owned by the Apparatus and populated during static registration.
 */
struct ActionEntry {
  const char* elementName = nullptr;
  const char* actionName = nullptr;
  ActionFn fn = nullptr;
  ActionEntry* next = nullptr;
};

struct PerceptionEntry {
  const char* elementName = nullptr;
  const char* perceptName = nullptr;
  PerceptionType type = PROPRIOCEPTION;
  PerceptionFn fn = nullptr;
  PerceptionEntry* next = nullptr;
};

struct SensingEntry {
  const char* elementName = nullptr;
  SensingFn fn = nullptr;
  SensingEntry* next = nullptr;
};

struct BehavingEntry {
  const char* elementName = nullptr;
  BehavingFn fn = nullptr;
  BehavingEntry* next = nullptr;
};

/* Independent bodily demand evaluated during enhancing(). */
struct TriebEntry {
  const char* elementName = nullptr;
  const char* triebName = nullptr;
  TriebFn fn = nullptr;
  TriebEntry* next = nullptr;
};


/* ============================================================================
 * Apparatus runtime
 * ============================================================================
 *
 * An Apparatus owns the runtime registries for its Elements, mediates
 * communication with the BDI mind, and coordinates the bodily lifecycle.
 *
 * Body cycle:
 *     sensing() -> enhancing() -> behaving()
 *
 * enhancing() handles incoming requests through inhabitance() and evaluates
 * independently declared Triebs through streamTriebs(). Percepts continue
 * to be evaluated only when the mind requests them.
 */
class Apparatus {
public:
  explicit Apparatus(const char* name)
    : _jslp(Serial), _name(name), _begun(false),
      _actions(nullptr), _percepts(nullptr),
      _sensings(nullptr), _behavings(nullptr), _triebs(nullptr) {
    instanceRef() = this;
    _apparatusID = fnv1a(BUILD_SIGNATURE, _apparatusID);
    _apparatusID = fnv1a(_name, _apparatusID);
  }

  static Apparatus* instance() { return instanceRef(); }
  const char* name() const { return _name; }

  /* Registration by the user-facing DSL. */
  void addAction(const char* elementName, const char* actionName, ActionFn fn) {
    ActionEntry* e = new ActionEntry();
    e->elementName = elementName;
    e->actionName = actionName;
    e->fn = fn;
    e->next = _actions;
    _actions = e;
  }

  void addPerception(const char* elementName, const char* perceptName,
                     PerceptionType type, PerceptionFn fn) {
    PerceptionEntry* e = new PerceptionEntry();
    e->elementName = elementName;
    e->perceptName = perceptName;
    e->type = type;
    e->fn = fn;
    e->next = _percepts;
    _percepts = e;
  }

  void addSensing(const char* elementName, SensingFn fn) {
    SensingEntry* e = new SensingEntry();
    e->elementName = elementName;
    e->fn = fn;
    e->next = _sensings;
    _sensings = e;
  }

  void addBehaving(const char* elementName, BehavingFn fn) {
    BehavingEntry* e = new BehavingEntry();
    e->elementName = elementName;
    e->fn = fn;
    e->next = _behavings;
    _behavings = e;
  }

  /* Register a DSL-defined bodily demand and its Drang callback. */
  void addTrieb(const char* elementName, const char* triebName, TriebFn fn) {
    TriebEntry* e = new TriebEntry();
    e->elementName = elementName;
    e->triebName = triebName;
    e->fn = fn;
    e->next = _triebs;
    _triebs = e;
  }

  void sensing() {
    for (SensingEntry* e = _sensings; e; e = e->next) e->fn();
  }

  void behaving() {
    for (BehavingEntry* e = _behavings; e; e = e->next) e->fn();
  }

  /* Wait for the initial embodiment handshake, then run the first cycle. */
  void embody() {
    if (!_begun) {
      while (!_jslp.incoming()) {}
      _begun = true;
    }
    bodyCycle();
  }

  void bodyCycle() {
    sensing();
    inhabitance();
    behaving();
  }



  /*
 * Evaluates registered bodily demands and transmits active Triebs.
 *
 * This method is called by streamPercepts() and assumes that a
 * JSON_SLP transmission is already open.
 *
 * A Trieb is transmitted only when its Drang is within (0.0, 1.0].
 */
  void streamTriebs() {

    for (TriebEntry* e = _triebs; e; e = e->next) {

      if (!e->fn)
        continue;

      const double drang = e->fn();

      if (!(drang > 0.0 && drang <= 1.0))
        continue;

      sendTriebJSON(e->elementName,e->triebName,drang);
    }
  }


  /* Process at most one incoming request without blocking bodily behavior. */
  void inhabitance() {
    if (!_jslp.incoming()) return;
    if (!_jslp.updateDoc(_JSONmsg)) return;

    const char* msg = _JSONmsg["msg"];
    if (!msg || msg[0] == '\0') return;

    if (strstr(msg, "getPercepts")) {
      streamPercepts();
    } else if (strstr(msg, "getKnowHow")) {
      streamKnowHow();
    } else if (strstr(msg, "getActions")) {
      String element = _JSONmsg["element"].as<String>();
      streamActions(element.length() == 0 || element == "null"
                      ? nullptr : element.c_str());
    } else {
      streamAction(_JSONmsg["msg"].as<String>(),
                   _JSONmsg["element"].as<String>(),
                   JSONtoNECKArgs(_JSONmsg["args"]));
    }

    lastPresence = millis();
  }

  /* Execute a requested Act on one Element or broadcast it by name. */
  void streamAction(String action, String element, NECKArgs args) {
    _jslp.startTransmission();

    if (action.length() != 0) {
      if (element.length() > 0 && element != "null") {
        Response result = dispatchActionUnicast(element.c_str(),
                                                action.c_str(), args);
        sendActionResultJSON(element.c_str(), action.c_str(), result);
      } else if (!dispatchActionBroadcast(action.c_str(), args)) {
        _JSONmsg.clear();
        _JSONmsg["request"] = action;
        _JSONmsg["response"] = actionResponseToStr(UNKNOWN);
        _JSONmsg["apparatus"] = _name;
        _JSONmsg["apparatusID"] = _apparatusID;
        _jslp.transmit(_JSONmsg);
      }
    }

    _jslp.endTransmission();
  }

  /* Evaluate registered Percepts on demand and transmit their values. */
  void streamPercepts() {
    _jslp.startTransmission();
    sendHeader("getPercepts", EXECUTED);

    for (PerceptionEntry* e = _percepts; e; e = e->next) {
      PerceptReturn result = perceive(e);
      _JSONmsg.clear();
      _JSONmsg["percept"] = e->perceptName;
      _JSONmsg["element"] = e->elementName;
      _JSONmsg["type"] = perceptionTypeToStr(e->type);
      _JSONmsg["status"] = perceptionResponseToStr(result.status);
      if (result.hasArgs) writeArgsJSON(result.args);
      _jslp.transmit(_JSONmsg);
    }

    // Evaluate and transmit bodily demands in the same transaction.
    streamTriebs();

    _jslp.endTransmission();
  }

  PerceptReturn perceive(PerceptionEntry* entry) {
    return entry->fn();
  }

  /* Shared JSON header for successful metadata requests. */
  void sendHeader(const char* request, Response response) {
    _JSONmsg.clear();
    _JSONmsg["request"] = request;
    _JSONmsg["response"] = actionResponseToStr(response);
    _JSONmsg["apparatus"] = _name;
    _JSONmsg["apparatusID"] = _apparatusID;
    _jslp.transmit(_JSONmsg);
  }

  /* Expose registered Acts, optionally filtering by Element name. */
  void streamActions(const char* onlyElement) {
    _jslp.startTransmission();
    sendHeader("getActions", EXECUTED);

    for (ActionEntry* e = _actions; e; e = e->next) {
      if (onlyElement && strcmp(e->elementName, onlyElement) != 0)
        continue;
      _JSONmsg.clear();
      _JSONmsg["action"] = e->actionName;
      _JSONmsg["element"] = e->elementName;
      _jslp.transmit(_JSONmsg);
    }

    _jslp.endTransmission();
  }

  /* Expose flash-resident TacitKnowledge discovered via linker boundaries. */
  void streamKnowHow() {
    _jslp.startTransmission();
    sendHeader("getKnowHow", EXECUTED);

    const uintptr_t start = (uintptr_t)::__start_neck_tacit;
    const uintptr_t stop = (uintptr_t)::__stop_neck_tacit;
    if (stop <= start) {
      _jslp.endTransmission();
      return;
    }

    const uint16_t total =
      (uint16_t)((stop - start) / sizeof(TacitEntry));
    const TacitEntry* entries = (const TacitEntry*)start;

    for (uint16_t i = 0; i < total; ++i) {
      const TacitEntry* entry = &entries[i];
      const char* skill = (const char*)pgm_read_ptr(&entry->skill);
      const char* context = (const char*)pgm_read_ptr(&entry->context);
      const char* plan = (const char*)pgm_read_ptr(&entry->plan);
      if (!skill || !plan) continue;

      char buffer[128];
      _JSONmsg.clear();
      copyPROGMEM(skill, buffer, sizeof(buffer));
      _JSONmsg["skill"] = buffer;

      if (context) copyPROGMEM(context, buffer, sizeof(buffer));
      _JSONmsg["context"] = context ? buffer : nullptr;

      copyPROGMEM(plan, buffer, sizeof(buffer));
      _JSONmsg["plan"] = buffer;
      _jslp.transmit(_JSONmsg);
    }

    _jslp.endTransmission();
  }

  /* Dispatch an Act to the named Element; UNKNOWN means no matching entry. */
  Response dispatchActionUnicast(const char* elementName,
                                 const char* actionName,
                                 const NECKArgs& args) {
    for (ActionEntry* e = _actions; e; e = e->next) {
      if (strcmp(e->elementName, elementName) == 0 &&
          strcmp(e->actionName, actionName) == 0)
        return e->fn ? e->fn(args) : UNABLE;
    }
    return UNKNOWN;
  }

  /* Dispatch to all Elements providing the requested Act name. */
  bool dispatchActionBroadcast(const char* actionName,
                               const NECKArgs& args) {
    bool any = false;
    for (ActionEntry* e = _actions; e; e = e->next) {
      if (strcmp(e->actionName, actionName) != 0) continue;
      any = true;
      sendActionResultJSON(e->elementName, actionName,
                           e->fn ? e->fn(args) : UNABLE);
    }
    return any;
  }

  /*
   * Send a body-originated Trieb. The caller is responsible for opening the
   * JSON_SLP transmission; this method only emits its message.
   */
  void sendTriebJSON(const char* elementName, const char* triebName, double drang) {
    _JSONmsg.clear();
    _JSONmsg["trieb"] = triebName;
    _JSONmsg["element"] = elementName;
    //_JSONmsg["apparatus"] = _name;
    _JSONmsg["drang"] = drang;
    //_JSONmsg["apparatusID"] = _apparatusID;
    _jslp.transmit(_JSONmsg);
  }

  void sendActionResultJSON(const char* elementName,
                            const char* actionName, Response response) {
    _JSONmsg.clear();
    _JSONmsg["request"] = actionName;
    _JSONmsg["response"] = actionResponseToStr(response);
    _JSONmsg["apparatus"] = _name;
    _JSONmsg["apparatusID"] = _apparatusID;
    _JSONmsg["element"] = elementName;
    _jslp.transmit(_JSONmsg);
  }

  unsigned long getLastPresence() { return lastPresence; }

  /* Optional one-time hardware initialization called by DSL-generated setup(). */
  static void setPreparation(PreparationFn fn) { preparationSlot() = fn; }
  static void prepare() {
    PreparationFn fn = preparationSlot();
    if (fn) fn();
  }

private:
  static PreparationFn& preparationSlot() {
    static PreparationFn fn = nullptr;
    return fn;
  }

  JSON_SLP _jslp;
  JsonDocument _JSONmsg;
  const char* _name;
  bool _begun;

  ActionEntry* _actions;
  PerceptionEntry* _percepts;
  SensingEntry* _sensings;
  BehavingEntry* _behavings;
  TriebEntry* _triebs;

  unsigned long lastPresence = 0;
  uint32_t _apparatusID = 2166136261UL;



  /* Serialize NECKArgs without losing boolean, integer or string types. */
  void writeArgsJSON(const NECKArgs& args) {
    for (uint8_t i = 0; i < args.size(); ++i) {
      const ArgValue& value = args.at(i);
      switch (value.type) {
        case ARG_BOOL:   _JSONmsg["args"][i] = value.v.b; break;
        case ARG_INT:    _JSONmsg["args"][i] = value.v.i; break;
        case ARG_UINT:   _JSONmsg["args"][i] = value.v.u; break;
        case ARG_FLOAT:  _JSONmsg["args"][i] = value.v.f; break;
        case ARG_STRING: _JSONmsg["args"][i] = value.s; break;
      }
    }
  }

  static uint32_t fnv1a(const char* text, uint32_t hash) {
    while (*text) {
      hash ^= (uint8_t)*text++;
      hash *= 16777619UL;
    }
    return hash;
  }

  static Apparatus*& instanceRef() {
    static Apparatus* ptr = nullptr;
    return ptr;
  }

  static const __FlashStringHelper* actionResponseToStr(Response response) {
    switch (response) {
      case EXECUTED: return F("executed");
      case UNABLE:   return F("unable");
      case ALREADY:  return F("already");
      case REJECTED: return F("rejected");
      case INVALID:  return F("invalid");
      case UNKNOWN:  return F("unknown");
      default:       return F("unknown");
    }
  }

  static const __FlashStringHelper* perceptionResponseToStr(Response response) {
    switch (response) {
      case PERCEPTED:   return F("percepted");
      case UNAVAILABLE: return F("unavailable");
      case UNCHANGED:   return F("unchanged");
      default:          return F("percepted");
    }
  }

  static const __FlashStringHelper* perceptionTypeToStr(PerceptionType type) {
    switch (type) {
      case INTEROCEPTION:  return F("interoception");
      case PROPRIOCEPTION: return F("proprioception");
      case EXTEROCEPTION:  return F("exteroception");
      default:             return F("proprioception");
    }
  }

  static void copyPROGMEM(const char* source, char* output,
                          size_t outputSize) {
    if (!source || outputSize == 0) return;
    size_t n = strnlen_P(source, outputSize - 1);
    memcpy_P(output, source, n);
    output[n] = '\0';
  }

  /* Parse a JSON argument array while preserving supported primitive types. */
  static NECKArgs JSONtoNECKArgs(JsonVariantConst jsonArgs) {
    NECKArgs out;
    if (!jsonArgs.is<JsonArrayConst>()) return out;

    for (JsonVariantConst value : jsonArgs.as<JsonArrayConst>()) {
      if (value.is<bool>()) out.add(value.as<bool>());
      else if (value.is<int>()) out.add((int32_t)value.as<int>());
      else if (value.is<long>()) out.add((int32_t)value.as<long>());
      else if (value.is<uint32_t>()) out.add(value.as<uint32_t>());
      else if (value.is<float>()) out.add(value.as<float>());
      else if (value.is<double>()) out.add((float)value.as<double>());
      else if (value.is<const char*>()) out.add(value.as<const char*>());
      else {
        String text;
        serializeJson(value, text);
        out.add(text);
      }
    }
    return out;
  }
};