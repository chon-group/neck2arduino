/**
 * ============================================================================
 * JSON_SLP.hpp
 * ============================================================================
 *
 * Lightweight framing protocol for exchanging sequences of JSON objects over
 * an Arduino Stream.
 *
 * JSON_SLP was designed as the transport/framing mechanism used by NECK for
 * communication between an embedded body and external software.
 *
 * The protocol was inspired by two Internet standards:
 *
 *   RFC 1055 - "Nonstandard for transmission of IP datagrams over serial lines:
 *               SLIP"
 *
 *   RFC 7464 - "JavaScript Object Notation (JSON) Text Sequences"
 *
 * JSON_SLP does not implement either RFC in its entirety. Instead, it combines
 * framing concepts from both specifications to provide a small protocol suited
 * to resource-constrained embedded devices communicating through serial
 * streams.
 *
 * --------------------------------------------------------------------------
 * Inspiration from RFC 1055 (SLIP)
 * --------------------------------------------------------------------------
 *
 * SLIP defines the END byte:
 *
 *     END = 0xC0
 *
 * JSON_SLP adopts this byte as the outer delimiter of a transmission. A
 * transmission may contain one or more JSON records:
 *
 *     0xC0
 *         JSON record
 *         JSON record
 *         ...
 *     0xC0
 *
 * Therefore, 0xC0 marks the transition into and out of a JSON_SLP
 * transmission.
 *
 * IMPORTANT:
 * JSON_SLP is not a complete implementation of SLIP. In particular, the
 * SLIP escaping mechanism is not implemented here. RFC 1055 is used as the
 * inspiration for the outer transmission delimiter.
 *
 * --------------------------------------------------------------------------
 * Inspiration from RFC 7464 (JSON Text Sequences)
 * --------------------------------------------------------------------------
 *
 * RFC 7464 defines a sequence of JSON texts using:
 *
 *     RS = 0x1E    Record Separator
 *     LF = 0x0A    Line Feed
 *
 * JSON_SLP adopts these bytes to delimit each JSON document inside an open
 * transmission:
 *
 *     0x1E <JSON> 0x0A
 *
 * Consequently, a transmission containing multiple JSON documents has the
 * following structure:
 *
 *     0xC0
 *       0x1E {"msg":"first"}  0x0A
 *       0x1E {"msg":"second"} 0x0A
 *       0x1E {"msg":"third"}  0x0A
 *     0xC0
 *
 * Or, symbolically:
 *
 *     END
 *       RS JSON LF
 *       RS JSON LF
 *       ...
 *     END
 *
 * This two-level framing allows JSON_SLP to distinguish:
 *
 *   - a transmission, delimited by END (0xC0); and
 *   - individual JSON records, delimited by RS (0x1E) and LF (0x0A).
 *
 *
 *  RFC 1055                    RFC 7464
 *    │                           │
 *    │ END = 0xC0                │ RS = 0x1E / LF = 0x0A
 *    │                           │
 *    └──────────┐       ┌────────┘
 *               ▼       ▼
 *                JSON_SLP
 *                   │
 *           END ........ END
 *              │
 *              ├─ RS JSON LF
 *              ├─ RS JSON LF
 *              └─ RS JSON LF
 *
 * --------------------------------------------------------------------------
 * Design goal
 * --------------------------------------------------------------------------
 *
 * The purpose of JSON_SLP is not to provide a general-purpose replacement
 * for SLIP or JSON Text Sequences. It provides a compact framing mechanism
 * for streaming one or more JSON messages through Arduino Stream interfaces
 * while keeping parsing and memory requirements small.
 *
 * The implementation operates directly over Stream and therefore can be used
 * with Serial and other Arduino communication classes derived from Stream.
 *
 * Author: Nilson Lazarin
 * Revision: September 2026
 * Cognitive Hardware on Networks Research Group - https://chon.group
 *
 * References:
 *   RFC 1055 - https://www.rfc-editor.org/rfc/rfc1055
 *   RFC 7464 - https://www.rfc-editor.org/rfc/rfc7464
 *
 * ============================================================================
 */

#pragma once

#include "Arduino.h"
#include "../lib/ArduinoJson-7.4.2/src/ArduinoJson.h"


class JSON_SLP {

    public:

        /*
         * Creates a JSON_SLP endpoint over an Arduino Stream.
         *
         * Using Stream instead of HardwareSerial keeps the framing mechanism
         * independent of a specific serial implementation.
         */
        JSON_SLP(Stream &serialPort) {
            _serial = &serialPort;
        }


        /*
         * Checks whether the endpoint is currently receiving a transmission.
         *
         * When outside a frame, the first END byte (0xC0) starts a new
         * transmission.
         *
         * Once inside a frame, reception remains active until another END
         * byte is found or the frame timeout expires.
         */
        bool incoming() {

            // No transmission is currently open: look for the initial END.
            if (!_inFrame) {
                if (testConsumption(TRANSMISSION)) {
                    _inFrame = true;
                    _timeSTARTED = millis();
                    return true;
                }

                return false;
            }

            // A transmission is open: END or timeout closes it.
            if (timeout() || test(TRANSMISSION)) {
                _inFrame = false;
                return false;
            }

            return true;
        }


        /*
         * Opens an outgoing transmission.
         *
         * The END byte (0xC0), inspired by RFC 1055, is written only when
         * there is no transmission already open.
         */
        void startTransmission() {

            if (!_transmitting) {
                _serial->write(TRANSMISSION);
                _transmitting = true;
            }
        }


        /*
         * Closes the current outgoing transmission by writing END (0xC0).
         *
         * flush() ensures that queued output bytes have been transmitted
         * before the transmission is considered finished.
         */
        void endTransmission() {

            if (_transmitting) {
                _serial->write(TRANSMISSION);
                _serial->flush();
                _transmitting = false;
            }
        }


        /*
         * Writes one JSON record inside an already opened transmission.
         *
         * Following the framing adopted from RFC 7464:
         *
         *     RS <JSON> LF
         *
         * where:
         *
         *     RS = 0x1E
         *     LF = 0x0A
         */
        void transmit(const JsonDocument& doc) {

            _serial->write(JSONSTART);
            serializeJson(doc, *_serial);
            _serial->write(JSONEND);
        }


        /*
         * Sends a complete transmission containing one JSON document.
         *
         * Resulting wire format:
         *
         *     END RS <JSON> LF END
         */
        void sendMsg(const JsonDocument& doc) {

            if (!_transmitting)
                startTransmission();

            transmit(doc);
            endTransmission();
        }


        /*
         * Attempts to read one JSON record from the current transmission.
         *
         * A document is read only when:
         *
         *   1. a transmission is currently open; and
         *   2. the next consumed byte is RS (0x1E).
         *
         * The JSON text is then read until LF (0x0A) and deserialized directly
         * into targetDoc.
         *
         * Receiving a JSON record also refreshes the frame timeout.
         *
         * Returns true when ArduinoJson successfully deserializes the record;
         * otherwise returns false.
         */
        bool updateDoc(JsonDocument& targetDoc) {

            if (!_inFrame)
                return false;

            if (testConsumption(JSONSTART)) {

                targetDoc.clear();

                // Receiving a record indicates activity in the current frame.
                _timeSTARTED = millis();

                _err = deserializeJson(
                    targetDoc,
                    _serial->readStringUntil(JSONEND)
                );

                return !_err;
            }

            return false;
        }


    private:

        /*
         * Framing bytes.
         *
         * TRANSMISSION (0xC0)
         *   Inspired by the END byte defined by RFC 1055 (SLIP).
         *
         * JSONSTART (0x1E)
         *   Record Separator (RS) defined by RFC 7464.
         *
         * JSONEND (0x0A)
         *   Line Feed (LF) used by RFC 7464 after a JSON text.
         */
        static const uint8_t TRANSMISSION = 0xC0;
        static const uint8_t JSONSTART    = 0x1E;
        static const uint8_t JSONEND      = 0x0A;


        // Communication stream used by the protocol.
        Stream* _serial;

        // Result of the most recent ArduinoJson deserialization.
        DeserializationError _err;


        /*
         * Reception state.
         *
         * _maxFRAMEtime
         *   Maximum inactivity period allowed for an open incoming frame.
         *
         * _timeSTARTED
         *   Timestamp of the most recent frame start or received JSON record.
         *
         * _inFrame
         *   Indicates that an incoming transmission is currently open.
         *
         * _transmitting
         *   Indicates that an outgoing transmission is currently open.
         */
        unsigned long _maxFRAMEtime = 2200;
        unsigned long _timeSTARTED  = 0;

        bool _inFrame      = false;
        bool _transmitting = false;


        /*
         * Tests whether the next available byte equals t.
         *
         * Unlike testConsumption(), this function first peeks at the byte and
         * consumes it only when it matches.
         */
        bool test(const uint8_t t) {

            if (_serial->available() > 0) {
                if (_serial->peek() != t)
                    return false;

                if (_serial->read() == t)
                    return true;
            }

            return false;
        }


        /*
         * Consumes the next available byte and tests whether it equals t.
         *
         * Notice that a non-matching byte is consumed as well. This behavior
         * is intentional in the current parser and allows it to advance while
         * searching for framing bytes.
         */
        bool testConsumption(const uint8_t t) {

            if (_serial->available() > 0) {
                if (_serial->read() == t)
                    return true;
            }

            return false;
        }


        /*
         * Returns true when the maximum allowed frame time has elapsed.
         *
         * The timer starts when an incoming transmission is opened and is
         * refreshed whenever a JSON record starts being processed.
         */
        bool timeout() {

            if (millis() < (_timeSTARTED + _maxFRAMEtime))
                return false;

            return true;
        }
};