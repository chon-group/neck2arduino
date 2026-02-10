#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "JSON_SLP.hpp"

// Linker-provided bounds for section "neck_tacit" (global symbols).
// Declared as bytes to avoid namespace/type mangling issues on AVR.
extern "C" {
  extern const uint8_t __start_neck_tacit[];
  extern const uint8_t __stop_neck_tacit[];
}

// PROGMEM helpers (AVR-friendly). On non-AVR boards, PROGMEM becomes a no-op.
#if defined(ARDUINO_ARCH_AVR)
  #include <avr/pgmspace.h>
#else
  #ifndef PROGMEM
    #define PROGMEM
  #endif
  #ifndef pgm_read_byte
    #define pgm_read_byte(addr) (*(const uint8_t*)(addr))
  #endif
  #ifndef pgm_read_ptr
    #define pgm_read_ptr(addr) (*(const void* const*)(addr))
  #endif
#endif

/*
  NECK.hpp — Single-header scaffold for Arduino (AVR-friendly)

  DSL supported (global scope):
    Apparatus(myApparatus);

    Element(myApparatus, led);

    Action(led, toggleLED) { ... return EXECUTED; }

    Perception(led, ledStatus, PROPRIOCEPTION) { return true; }
    Perception(led, ledID, PROPRIOCEPTION) { return UNAVAILABLE; }

  Minimal runtime:
    - myApparatus.embody() reads incoming lines from Serial (or setIO Stream)
    - If line contains "getPercepts" -> streams all registered perceptions as JSON
    - Else tries to parse JSON-ish line with "element" and "action" and optional "args":[...]
*/

namespace NECK {

static const char BUILD_SIGNATURE[] = __DATE__ " " __TIME__;

/* =========================
   Enums (contracts)
   ========================= */

enum ElementType : uint8_t { SENSOR, EFFECTOR };

enum ActionResponse : uint8_t {
  EXECUTED,
  UNABLE,
  ALREADY,
  REJECTED,
  INVALID,
  UNKNOWN
};

enum PerceptionResponse : uint8_t {
  PERCEPTED,
  UNAVAILABLE,
  UNCHANGED
};

enum PerceptionType : uint8_t {
  INTEROCEPTION,
  PROPRIOCEPTION,
  EXTEROCEPTION
};


struct TacitEntry {
  const char* apparatus; // PROGMEM string (PSTR)
  const char* skill; // PROGMEM string (PSTR)
  const char* context;   // PROGMEM string (PSTR) or nullptr => true
  const char* plan;      // PROGMEM string (PSTR)
};

// Linker-provided bounds for section "neck_tacit" are declared globally above.

// Ensure section exists even if user not to declare any TacitKnowledge.
__attribute__((used, section("neck_tacit")))
static const TacitEntry __neck_tacit_sentinel = { nullptr, nullptr, nullptr, nullptr };

/* =========================
   Args (dynamic)
   ========================= */

enum ArgType : uint8_t { ARG_BOOL, ARG_INT, ARG_FLOAT, ARG_STRING };

struct ArgValue {
  ArgType type;
  union { bool b; int32_t i; float f; } v;
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
    if (_n >= MAX) return false;
    _a[_n].type = ARG_BOOL; _a[_n].v.b = x; _a[_n].s = ""; _n++;
    return true;
  }
  bool add(int32_t x) {
    if (_n >= MAX) return false;
    _a[_n].type = ARG_INT; _a[_n].v.i = x; _a[_n].s = ""; _n++;
    return true;
  }
  bool add(int x) { return add((int32_t)x); } // convenience
  bool add(float x) {
    if (_n >= MAX) return false;
    _a[_n].type = ARG_FLOAT; _a[_n].v.f = x; _a[_n].s = ""; _n++;
    return true;
  }

  // Ergonomics: allow string literals without accidental conversion to bool.
  bool add(const char* x) {
    if (_n >= MAX) return false;
    _a[_n].type = ARG_STRING;
    _a[_n].s = (x == nullptr) ? String("") : String(x);
    _n++;
    return true;
  }

  // Accept double literals (on AVR Uno, double == float, but overload avoids ambiguity).
  bool add(double x) { return add((float)x); }

  bool add(const String& x) {
    if (_n >= MAX) return false;
    _a[_n].type = ARG_STRING; _a[_n].s = x; _n++;
    return true;
  }

  bool isBool(uint8_t idx) const   { return idx < _n && _a[idx].type == ARG_BOOL; }
  bool isInt(uint8_t idx) const    { return idx < _n && _a[idx].type == ARG_INT; }
  bool isFloat(uint8_t idx) const  { return idx < _n && _a[idx].type == ARG_FLOAT; }
  bool isString(uint8_t idx) const { return idx < _n && _a[idx].type == ARG_STRING; }

  bool   asBool(uint8_t idx) const   { return _a[idx].v.b; }
  int32_t asInt(uint8_t idx) const   { return _a[idx].v.i; }
  float  asFloat(uint8_t idx) const  { return _a[idx].v.f; }
  String asString(uint8_t idx) const { return _a[idx].s; }

  const ArgValue& at(uint8_t idx) const { return _a[idx]; }

private:
  ArgValue _a[MAX];
  uint8_t _n;
};

/* =========================
   Perception return envelope
   ========================= */

struct PerceptReturn {
  PerceptionResponse status;
  bool hasArgs;
  NECKArgs args;

  PerceptReturn() : status(PERCEPTED), hasArgs(false) {}

  // status only
  PerceptReturn(PerceptionResponse r) : status(r), hasArgs(false) {}

  // args => PERCEPTED
  PerceptReturn(const NECKArgs& a) : status(PERCEPTED), hasArgs(true), args(a) {}

  // primitives => PERCEPTED + args[0]
  PerceptReturn(bool x) : status(PERCEPTED), hasArgs(true) { args.add(x); }
  PerceptReturn(int32_t x) : status(PERCEPTED), hasArgs(true) { args.add(x); }
  PerceptReturn(int x) : status(PERCEPTED), hasArgs(true) { args.add(x); } // fixes ambiguity
  PerceptReturn(float x) : status(PERCEPTED), hasArgs(true) { args.add(x); }
  PerceptReturn(const String& x) : status(PERCEPTED), hasArgs(true) { args.add(x); }
};

/* =========================
   Registry nodes
   ========================= */

class Apparatus;

struct ElementDef {
  // --- Trieb (drive) ---
  // Sends a JSON message immediately:
  //   {"trieb":"name","args":[...],"apparatus":"...","element":"...","drang":0.7}
  void trieb(const char* triebName, double drang);
  void trieb(const char* triebName, const NECKArgs& args, double drang);

  const char* name;
  Apparatus* apparatus;
  ElementDef* next;
  ElementDef(const char* n, Apparatus* a);
};

typedef ActionResponse (*ActionFn)(const NECKArgs& ActionArgs);
typedef PerceptReturn (*PerceptionFn)();

struct ActionEntry {
  const char* elementName;
  const char* actionName;
  ActionFn fn;
  ActionEntry* next;
  ActionEntry() : elementName(nullptr), actionName(nullptr), fn(nullptr), next(nullptr) {}
};

struct PerceptionEntry {
  const char* elementName;
  const char* perceptName;
  PerceptionType type;
  PerceptionFn fn;
  PerceptionEntry* next;
  PerceptionEntry() : elementName(nullptr), perceptName(nullptr), type(PROPRIOCEPTION), fn(nullptr), next(nullptr) {}
};

/* =========================
   Apparatus
   ========================= */

class Apparatus {
public:
  explicit Apparatus(const char* name)
    : _name(name), _begun(false), _elements(nullptr), _actions(nullptr), _percepts(nullptr), _jslp(Serial), 
      _lineLen(0) {
        _apparatusID = fnv1a(BUILD_SIGNATURE, _apparatusID);
        _apparatusID = fnv1a(_name, _apparatusID);
      }

  const char* name() const { return _name; }
 
  void begin(unsigned long baud) {
    Serial.begin(baud);
    _begun = true;
  }

  void attachElement(ElementDef* e) {
    e->next = _elements;
    _elements = e;
  }

  void addAction(const char* elementName, const char* actionName, ActionFn fn) {
    ActionEntry* ae = new ActionEntry();
    ae->elementName = elementName;
    ae->actionName  = actionName;
    ae->fn          = fn;
    ae->next        = _actions;
    _actions        = ae;
  }

  void addPerception(const char* elementName, const char* perceptName, PerceptionType t, PerceptionFn fn) {
    PerceptionEntry* pe = new PerceptionEntry();
    pe->elementName = elementName;
    pe->perceptName  = perceptName;
    pe->type        = t;
    pe->fn          = fn;
    pe->next        = _percepts;
    _percepts       = pe;
  }

  void embody() {
    if (!_begun) begin(115200);

    if (_jslp.incoming()) {
      if (_jslp.updateDoc(_JSONmsg)) {
      // valida "msg" obrigatoria e string nao vazia
        const char* msg = _JSONmsg["msg"];
        if (!msg || msg[0] == '\0') return;
        
        if (strstr(msg, "getPercepts")      != nullptr){streamPercepts();  return;}
        else if (strstr(msg, "getKnowHow")  != nullptr){streamKnowHow();   return;}
        else if (strstr(msg, "getActions")  != nullptr){
          String element = _JSONmsg["element"].as<String>();
          if (element.length() == 0 || element == "null") streamActions(nullptr);
          else streamActions(element.c_str());
          return;
        }else{
          streamAction(_JSONmsg["msg"].as<String>(),
                    _JSONmsg["element"].as<String>(),
                    JSONtoNECKArgs(_JSONmsg["args"]));
        }
      }
    }
  }


  void streamAction(String action, String element, NECKArgs args){    
    _jslp.startTransmission();

    if (!action.length() == 0){
      if (element.length() > 0 && element != "null") {
        // UNICAST
        ActionResponse r = dispatchActionUnicast(element.c_str(), action.c_str(), args);
        sendActionResultJSON(element.c_str(), action.c_str(), r);
      } else {
        // BROADCAST
        digitalWrite(12,HIGH);
        bool any = dispatchActionBroadcast(action.c_str(), args);
        if (!any){
          _JSONmsg.clear(); 
          _JSONmsg["apparatus"]   = _name;
          _JSONmsg["bodyResponse"]    = actionResponseToStr(ActionResponse::UNKNOWN);
          _JSONmsg["request"]     = action;
          _JSONmsg["apparatusID"] = _apparatusID;
          _jslp.transmit(_JSONmsg);
        }
      }
    }
     _jslp.endTransmission();
  }


  void streamPercepts() {
    _jslp.startTransmission();

    _JSONmsg.clear(); 
    _JSONmsg["apparatus"]   = _name;
    _JSONmsg["bodyResponse"]    = actionResponseToStr(ActionResponse::EXECUTED);
    _JSONmsg["request"]     = "getPercepts";
    _JSONmsg["apparatusID"] = _apparatusID;
    _jslp.transmit(_JSONmsg);

    for (PerceptionEntry* p = _percepts; p != nullptr; p = p->next) {
      PerceptReturn ret = p->fn();

      _JSONmsg.clear(); 
      _JSONmsg["percept"]     = p->perceptName;
      _JSONmsg["element"] = p->elementName;
      _JSONmsg["type"]     = perceptionTypeToStr(p->type);
      _JSONmsg["status"]  = perceptionResponseToStr(ret.status);

      if(ret.hasArgs){
        for (uint8_t i = 0; i < ret.args.size(); i++) {
          
          const ArgValue& av = ret.args.at(i);
          switch (av.type) {
            case ARG_BOOL:   _JSONmsg["args"][i] = (av.v.b); break;
            case ARG_INT:    _JSONmsg["args"][i] = (av.v.i); break;
            case ARG_FLOAT:  _JSONmsg["args"][i] = (av.v.f, 6); break;
            case ARG_STRING: _JSONmsg["args"][i] = (av.s); break;
          }
        }
      }
      _jslp.transmit(_JSONmsg);
    }
    _jslp.endTransmission();
  }

  void streamActions(const char* onlyElement) {
    _jslp.startTransmission();

    _JSONmsg.clear(); 
    _JSONmsg["apparatus"]   = _name;
    _JSONmsg["bodyResponse"]    = actionResponseToStr(ActionResponse::EXECUTED);
    _JSONmsg["request"]     = "getActions";
    _JSONmsg["apparatusID"] = _apparatusID;
    _jslp.transmit(_JSONmsg);

    for (ActionEntry* a = _actions; a != nullptr; a = a->next) {
      if (onlyElement && strcmp(a->elementName, onlyElement) != 0) continue;
      _JSONmsg.clear(); 
      _JSONmsg["action"]     = a->actionName;
      _JSONmsg["element"] = a->elementName;
      _jslp.transmit(_JSONmsg);
    }
    _jslp.endTransmission();
  }

  void streamKnowHow() {
    _jslp.startTransmission();

    _JSONmsg.clear(); 
    _JSONmsg["apparatus"]   = _name;
    _JSONmsg["bodyResponse"]    = actionResponseToStr(ActionResponse::EXECUTED);
    _JSONmsg["request"]     = "getKnowHow";
    _JSONmsg["apparatusID"] = _apparatusID;
    _jslp.transmit(_JSONmsg);

    // Iterate over TacitKnowledge entries stored in Flash (section "neck_tacit")
    const uintptr_t start = (uintptr_t)::__start_neck_tacit;
    const uintptr_t stop  = (uintptr_t)::__stop_neck_tacit;
    if (stop <= start) { _jslp.endTransmission(); return; }

    const uint16_t total = (uint16_t)((stop - start) / sizeof(TacitEntry));
    const TacitEntry* base = (const TacitEntry*)start;

    for (uint16_t i = 0; i < total; i++) {
      const TacitEntry* e = &base[i];

      // Read pointers from PROGMEM/flash-stored struct
      const char* appP = (const char*)pgm_read_ptr(&e->apparatus);
      const char* knP  = (const char*)pgm_read_ptr(&e->skill);
      const char* ctxP = (const char*)pgm_read_ptr(&e->context);
      const char* plP  = (const char*)pgm_read_ptr(&e->plan);

      if (!appP || !knP || !plP) continue; // skip sentinel/empty

      // Filter: only entries for this apparatus
      if (strcmp_P_ram(appP, _name) != 0) continue;

      _JSONmsg.clear();
      
      char tempBuffer[128];
      copyPROGMEM(knP, tempBuffer, sizeof(tempBuffer));
      _JSONmsg["skill"] = tempBuffer;

      if (ctxP) copyPROGMEM(ctxP, tempBuffer, sizeof(tempBuffer));
      _JSONmsg["context"]   = ctxP ? tempBuffer : nullptr;

      copyPROGMEM(plP, tempBuffer, sizeof(tempBuffer));
      _JSONmsg["plan"]      = tempBuffer;

      _jslp.transmit(_JSONmsg);
    }
    _jslp.endTransmission();
  }


  ActionResponse dispatchAction(const char* elementName, const char* actionName, const NECKArgs& args) {
    for (ActionEntry* a = _actions; a != nullptr; a = a->next) {
      if (strcmp(a->elementName, elementName) == 0 && strcmp(a->actionName, actionName) == 0) {
        if (a->fn) return a->fn(args);
        return UNABLE;
      }
    }
    return UNKNOWN;
  }


  ActionResponse dispatchActionUnicast(const char* elementName, const char* actionName, const NECKArgs& args) {
    for (ActionEntry* a = _actions; a != nullptr; a = a->next) {
      if (strcmp(a->elementName, elementName) == 0 && strcmp(a->actionName, actionName) == 0) {
        return a->fn ? a->fn(args) : UNABLE;
      }
    }
    return UNKNOWN;
  }

  bool dispatchActionBroadcast(const char* actionName, const NECKArgs& args) {
    bool any = false;
    for (ActionEntry* a = _actions; a != nullptr; a = a->next) {
      if (strcmp(a->actionName, actionName) == 0) {
        any = true;
        ActionResponse r = a->fn ? a->fn(args) : UNABLE;
        sendActionResultJSON(a->elementName, actionName, r);
      }
    }
    return any;
  }



  /* ===== JSON Transmission ===== */

  void sendTriebJSON(const char* elementName, const char* triebName, const NECKArgs* argsOrNull, double drang) {
    _JSONmsg.clear();
    _JSONmsg["trieb"]   = triebName;
    _JSONmsg["element"] = elementName;  
    _JSONmsg["drang"]   = drang;

    if (argsOrNull && argsOrNull->size() > 0) {
      for (uint8_t i = 0; i < argsOrNull->size(); i++) {
        const ArgValue& av = argsOrNull->at(i);
        switch (av.type) {
          case ARG_BOOL: _JSONmsg["args"][i] = av.v.b; break;
          case ARG_INT:    _JSONmsg["args"][i] = av.v.i; break;
          case ARG_FLOAT:  _JSONmsg["args"][i] = av.v.f; break;
          case ARG_STRING: _JSONmsg["args"][i] = av.s; break;
        }
      }
    
    }
    _jslp.transmit(_JSONmsg);

  }

  void sendActionResultJSON(const char* elementName, const char* actionName, ActionResponse r) {
    _JSONmsg.clear(); 
    _JSONmsg["apparatus"]   = _name;
    _JSONmsg["bodyResponse"]    = actionResponseToStr(r);
    _JSONmsg["request"]     = actionName;
    _JSONmsg["apparatusID"] = _apparatusID;
    _JSONmsg["element"]     = elementName;
    _jslp.transmit(_JSONmsg);
    
  }


private:
  JSON_SLP _jslp;
  JsonDocument _JSONmsg;
  const char* _name;
  bool _begun;

  ElementDef* _elements;
  ActionEntry* _actions;
  PerceptionEntry* _percepts;

  char _line[200];
  uint8_t _lineLen;
  uint32_t _apparatusID = 2166136261UL;

  static uint32_t fnv1a(const char* s, uint32_t hash) {
    while (*s) {
      hash ^= (uint8_t)*s++;
      hash *= 16777619UL;
    }
    return hash;
  }

  static const __FlashStringHelper* actionResponseToStr(ActionResponse r) {
    switch (r) {
      case EXECUTED: return F("executed");
      case UNABLE:   return F("unable");
      case ALREADY:  return F("already");
      case REJECTED: return F("rejected");
      case INVALID:  return F("invalid");
      case UNKNOWN:  return F("unknown");
      default:       return F("unknown");
    }
  }

  static const __FlashStringHelper* perceptionResponseToStr(PerceptionResponse r) {
    switch (r) {
      case PERCEPTED:   return F("percepted");
      case UNAVAILABLE: return F("unavailable");
      case UNCHANGED:   return F("unchanged");
      default:          return F("percepted");
    }
  }

  static const __FlashStringHelper* perceptionTypeToStr(PerceptionType t) {
    switch (t) {
      case INTEROCEPTION:  return F("interoception");
      case PROPRIOCEPTION: return F("proprioception");
      case EXTEROCEPTION:  return F("exteroception");
      default:             return F("proprioception");
    }
  }

  // Compare a PROGMEM string (pgmStr) with a RAM string (ramStr).
  static int strcmp_P_ram(const char* pgmStr, const char* ramStr) {
    if (!pgmStr && !ramStr) return 0;
    if (!pgmStr) return -1;
    if (!ramStr) return 1;
    while (true) {
      char a = (char)pgm_read_byte(pgmStr++);
      char b = *ramStr++;
      if (a != b) return (int)((uint8_t)a) - (int)((uint8_t)b);
      if (a == '\0') return 0;
    }
  }

  static void copyPROGMEM(const char* p, char* out, size_t outSize) {
    if (!p || outSize == 0) return;
    size_t n = strnlen_P(p, outSize - 1);
    memcpy_P(out, p, n);
    out[n] = '\0';
  }

  static NECKArgs JSONtoNECKArgs(JsonVariantConst jargs) {

    NECKArgs out;

    if (jargs.isNull()) return out;
    if (!jargs.is<JsonArrayConst>()) return out;

    for (JsonVariantConst v : jargs.as<JsonArrayConst>()) {
      if (v.is<bool>())              out.add(v.as<bool>());
      else if (v.is<int>())          out.add((int32_t)v.as<int>());
      else if (v.is<long>())         out.add((int32_t)v.as<long>());
      else if (v.is<float>())        out.add((float)v.as<float>());
      else if (v.is<double>())       out.add((float)v.as<double>());
      else if (v.is<const char*>())  out.add(v.as<const char*>());
      else {
        String tmp;
        serializeJson(v, tmp);       // fallback: vira string JSON
        out.add(tmp);
      }
    }
    return out;
  }

};

/* ElementDef ctor: auto-attach to apparatus */
inline ElementDef::ElementDef(const char* n, Apparatus* a)
  : name(n), apparatus(a), next(nullptr) {
  if (apparatus) apparatus->attachElement(this);
}

// --- Trieb (drive) ---
inline void ElementDef::trieb(const char* triebName, double drang) {
  if (!apparatus || !triebName) return;
  apparatus->sendTriebJSON(name, triebName, nullptr, drang);
}

inline void ElementDef::trieb(const char* triebName, const NECKArgs& args, double drang) {
  if (!apparatus || !triebName) return;
  apparatus->sendTriebJSON(name, triebName, &args, drang);
}

} // namespace NECK

/* =========================
   Bring DSL names to global
   ========================= */

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

/* =========================
   DSL Macros
   ========================= */

//#define Apparatus(NAME) NECK::Apparatus NAME(#NAME)
#define Apparatus(NAME)                                        \
  static bool __neck_only_one_Apparatus_per_Microcontroller;    \
  NECK::Apparatus NAME(#NAME)

/*
  Element(APPARATUS, ElementName);
*/
#define Element(APP, EL) NECK::ElementDef EL(#EL, &APP)

/* Unique name helpers */
#define NECK_CONCAT_INNER(a,b) a##b
#define NECK_CONCAT(a,b) NECK_CONCAT_INNER(a,b)

/*
  Action(Element, ACTNAME) { ... }
*/
#define Action(EL, ACTNAME) \
  static NECK::ActionResponse NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))(const NECK::NECKArgs& ActionArgs); \
  struct NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) { \
    NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))() { \
      if ((EL).apparatus) (EL).apparatus->addAction(#EL, #ACTNAME, &NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))); \
    } \
  }; \
  static NECK_CONCAT(__neck_action_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))) NECK_CONCAT(__neck_action_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME))); \
  static NECK::ActionResponse NECK_CONCAT(__neck_action_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, ACTNAME)))(const NECK::NECKArgs& ActionArgs)

/*
  Perception(Element, PERCEPT, TYPE) { ... }
*/
#define Perception(EL, PERCEPT, TYPE) \
  static NECK::PerceptReturn NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))(); \
  struct NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) { \
    NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))() { \
      if ((EL).apparatus) (EL).apparatus->addPerception(#EL, #PERCEPT, NECK::TYPE, &NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))); \
    } \
  }; \
  static NECK_CONCAT(__neck_percept_reg_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))) NECK_CONCAT(__neck_percept_reg_instance_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT))); \
  static NECK::PerceptReturn NECK_CONCAT(__neck_percept_fn_, NECK_CONCAT(EL, NECK_CONCAT(_, PERCEPT)))()

/*
  TacitKnowledge(APP, NAME, "plan");
  TacitKnowledge(APP, NAME, "context", "plan");
*/

#define NECK_TK3(APP, NAME, PLAN) \
  enum { NECK_CONCAT(__neck_tk_require_app_, __LINE__) = (int)sizeof(APP) }; \
  static const char NECK_CONCAT(__neck_tk_app_, __LINE__)[] PROGMEM = #APP; \
  static const char NECK_CONCAT(__neck_tk_name_, __LINE__)[] PROGMEM = #NAME; \
  static const char NECK_CONCAT(__neck_tk_plan_, __LINE__)[] PROGMEM = PLAN; \
  __attribute__((used, section("neck_tacit"))) \
  static const NECK::TacitEntry NECK_CONCAT(__neck_tk_entry_, __LINE__) = { \
    NECK_CONCAT(__neck_tk_app_, __LINE__), \
    NECK_CONCAT(__neck_tk_name_, __LINE__), \
    nullptr, \
    NECK_CONCAT(__neck_tk_plan_, __LINE__) \
  }

#define NECK_TK4(APP, NAME, CTX, PLAN) \
  enum { NECK_CONCAT(__neck_tk_require_app_, __LINE__) = (int)sizeof(APP) }; \
  static const char NECK_CONCAT(__neck_tk_app_, __LINE__)[] PROGMEM = #APP; \
  static const char NECK_CONCAT(__neck_tk_name_, __LINE__)[] PROGMEM = #NAME; \
  static const char NECK_CONCAT(__neck_tk_ctx_, __LINE__)[] PROGMEM = CTX; \
  static const char NECK_CONCAT(__neck_tk_plan_, __LINE__)[] PROGMEM = PLAN; \
  __attribute__((used, section("neck_tacit"))) \
  static const NECK::TacitEntry NECK_CONCAT(__neck_tk_entry_, __LINE__) = { \
    NECK_CONCAT(__neck_tk_app_, __LINE__), \
    NECK_CONCAT(__neck_tk_name_, __LINE__), \
    NECK_CONCAT(__neck_tk_ctx_, __LINE__), \
    NECK_CONCAT(__neck_tk_plan_, __LINE__) \
  }

// Macro overloading by argument count
#define NECK_GET_5TH_ARG(_1,_2,_3,_4,_5,...) _5
#define TacitKnowledge(...) NECK_GET_5TH_ARG(__VA_ARGS__, NECK_TK4, NECK_TK3, _NA, _NA)(__VA_ARGS__)

