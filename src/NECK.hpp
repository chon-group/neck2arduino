/*
 * ============================================================================
 * NECK.hpp
 * ============================================================================
 *
 * Main entry point of the NECK library.
 *
 * NECK (ageNt Embodied Cognition development Kit) provides the computational
 * mechanisms required to explicitly represent and operate the physical body
 * of a BDI agent.
 *
 * NECK was conceived as part of the MAOP+b model, which extends the
 * Multi-Agent Oriented Programming (MAOP) paradigm by explicitly
 * representing the bodies of embodied agents. Instead of treating sensors,
 * actuators and embedded devices merely as external resources of the
 * environment, NECK allows them to constitute the agent's physical body.
 *
 * In this model, the agent's cognitive and physical processes are distinct
 * but integrated as parts of the same embodied entity.
 * The agent performs cognitive processes such as deliberation,
 * while its body performs physical processes responsible for sensing and
 * behaving in the world. NECK provides the interface through which these
 * two dimensions become integrated.
 *
 * A body is composed of one or more Apparatus, each representing a physical
 * subsystem that may contain one or more Elements. Elements constitute the
 * points through which the body senses and acts upon both its internal state
 * and the external world.
 *
 * This organization allows the physical constitution of an agent to be
 * explicitly represented and potentially changed during execution by
 * attaching or detaching Apparatus, while preserving the distinction between
 * the agent's cognitive identity and its current physical embodiment.
 *
 * At the embedded-device level, this library provides the mechanisms used to
 * describe and execute an Apparatus and its Elements, establishing the
 * connection between physical processes and the BDI agent:
 *
 *                    ┌────────── Body ──────────┐
 *                    │                          │
 *                    │          Mind            │
 *                    │         (BDI)            │
 *                    │           │              │
 *                    │  Percept ↑│↓ Act         │
 *                    │           │              │
 *                    │      ┌─ Apparatus ─┐     │
 *                    │      │             │     │
 *                    │      │   Element   │     │
 *                    │      └──────┬──────┘     │
 *                    │             │            │
 *                    └─────────────┼────────────┘
 *                       Sensing ↑  │  ↓ Behaving
 *                                  │
 *                           Physical World
 *
 * Percept
 *   Exposes information produced by the body to the BDI agent, allowing
 *   bodily and environmental state to participate in its deliberative cycle.
 *
 * Act
 *   Represents an intentional action requested by the BDI agent and
 *   performed through its body.
 *
 * Sensing
 *   Represents a body-side sensing process performed continuously by an
 *   Element. It allows the body to acquire and maintain information about
 *   itself or the physical world independently of an explicit percept
 *   request from the agent.
 *
 * Behaving
 *   Represents a body-side behavioral process performed continuously by an
 *   Element. It allows physical behavior initiated by the agent, or arising
 *   from the body's own operation, to persist and evolve independently of
 *   new deliberative commands.
 *
 * The body execution cycle is implemented by Apparatus::embody():
 *
 *     sensing();
 *     inhabitance();
 *     behaving();
 *
 * This cycle deliberately places inhabitance between sensing and behaving:
 * the body first senses its current physical condition, then interacts with
 * the agent that inhabits it, and finally continues its bodily behavior in
 * the physical world.
 *
 * The inhabitance() process therefore represents the operational connection
 * between body and mind: percepts may flow from body to agent, while acts may
 * flow from agent to body.
 *
 * --------------------------------------------------------------------------
 * Internal organization
 * --------------------------------------------------------------------------
 *
 * NECKCore.hpp
 *   Fundamental types and contracts shared by the library, including
 *   response types, argument representation, percept return values and
 *   tacit-knowledge metadata.
 *
 * NECKBody.hpp
 *   Structural representation of the embedded body, including Elements,
 *   Trieb, sensing/behaving functions and the internal registries used to
 *   associate bodily processes with Elements.
 *
 * NECKApparatus.hpp
 *   Runtime implementation of an Apparatus. It coordinates the body execution
 *   cycle, communication with the BDI agent, action dispatch, percept
 *   transmission, tacit knowledge and Trieb messages.
 *
 * NECKDSL.hpp
 *   User-facing DSL for describing the physical constitution and bodily
 *   processes of an Apparatus. It provides Apparatus, Element, Percept, Act,
 *   Sensing, Behaving and TacitKnowledge declarations for embedded
 *   applications.
 *
 * Dependency order:
 *
 *     NECKCore.hpp
 *          ↓
 *     NECKBody.hpp
 *          ↓
 *     NECKApparatus.hpp
 *          ↓
 *     NECKDSL.hpp
 *
 * Users normally need to include only this file:
 *
 *     #include <NECK.hpp>
 *
 *
 * ============================================================================
 * Authorship and Reference
 * ============================================================================
 *
 * Author: Nilson Lazarin
 * Research Group: Cognitive Hardware on Networks (CHON) - https://chon.group
 *
 * Revision: September 2026
 *
 * The conceptual foundations underlying NECK and the introduction of the
 * body as an explicit dimension of BDI-agent-based embedded systems are
 * presented in:
 *
 * Nilson Lazarin, Carlos Pantoja, and Jose Viterbo. 2026.
 * "My Body, My Perceptions: A Shift from Computationalism to Embodied
 * Cognition in BDI-agent-based Embedded Systems."
 * In Proceedings of the 25th International Conference on Autonomous Agents
 * and Multiagent Systems (AAMAS 2026), Paphos, Cyprus, May 25–29, 2026.
 * IFAAMAS, 10 pages.
 *
 * https://doi.org/10.65109/QIVX3835
 *
 * ============================================================================
 */


/*
 * ============================================================================
 * Platform and standard dependencies
 * ============================================================================
 */

#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "JSON_SLP.hpp"


/*
 * ============================================================================
 * Tacit Knowledge linker section
 * ============================================================================
 *
 * TacitKnowledge declarations are stored in the "neck_tacit" linker section.
 *
 * The linker provides the symbols below marking the beginning and the end of
 * that section. Apparatus uses these boundaries to enumerate the registered
 * tacit knowledge at runtime.
 *
 * They are declared as raw bytes and outside namespace NECK to match the
 * global symbols generated by the linker and to avoid C++ name mangling.
 * ============================================================================
 */

extern "C" {
    extern const uint8_t __start_neck_tacit[];
    extern const uint8_t __stop_neck_tacit[];
}


/*
 * ============================================================================
 * Program-memory compatibility
 * ============================================================================
 *
 * On AVR architectures, constant data such as TacitKnowledge strings may be
 * stored in program memory (Flash) using PROGMEM.
 *
 * Other Arduino architectures may not provide the AVR-specific PROGMEM and
 * pgm_read_* facilities. The definitions below therefore provide compatible
 * fallbacks so the remaining NECK code can use the same interface regardless
 * of the target architecture.
 * ============================================================================
 */

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
 * ============================================================================
 * NECK implementation
 * ============================================================================
 *
 * These headers are intentionally included inside namespace NECK.
 *
 * Their order reflects their dependencies:
 *
 *   Core      -> fundamental contracts
 *   Body      -> body structures built upon Core
 *   Apparatus -> runtime built upon Core and Body
 *
 * The DSL is included afterwards because it exposes NECK types and macros at
 * the global scope for use directly in Arduino sketches.
 * ============================================================================
 */

namespace NECK {

    #include "NECKCore.hpp"
    #include "NECKBody.hpp"
    #include "NECKApparatus.hpp"

} 

/*
 * ============================================================================
 * User-facing Domain-Specific Language
 * ============================================================================
 *
 */

#include "NECKDSL.hpp"