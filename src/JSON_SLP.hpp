/**
 * JSON/SLP: JSON-seq over SLIP over Serial
 * Like a Transport protocol for streaming of JSON objects
 * Inspired by RFCs 1055 and 7464
 * Author: Nilson Lazarin 
 * January 2026
 */

#pragma once

#include "Arduino.h"
#include "../lib/ArduinoJson-7.4.2/src/ArduinoJson.h" // Dependence: https://arduinojson.org/ version 7.4.2*/

class JSON_SLP {
    public:
        // The constructor, receving the Serial instantiation from user scketch
        JSON_SLP(Stream &serialPort) {
            _serial = &serialPort;
        }

        // Return true if a new message is comming in the serial
        bool incoming() {
            // Se não estamos em um frame, tenta detectar o byte 0xC0
            if (!_inFrame) {
            if (testConsumption(TRANSMISSION)) {
                _inFrame = true;
                _timeSTARTED = millis();
                return true;
            }
            return false;
            }

            // Se já estamos no frame, verifica se ele deve fechar (Timeout ou fim 0xC0)
            if (timeout() || test(TRANSMISSION)) {
                _inFrame = false;
                return false;
            }
            return true; 
        }

        //  Sends the END (RFC 1055), indicating that a new transmission is soon
        void startTransmission() {
            if(!_transmitting){
            _serial->write(TRANSMISSION);
            _transmitting = true;
            }
        }
        
        //  Sends the END (RFC 1055), indicating that the transmission over
        void endTransmission() {
            if(_transmitting){
            _serial->write(TRANSMISSION);
            _serial->flush();
            _transmitting = false;
            }
        }

        // Sends an JSON document in a openned transmission
        void transmit(const JsonDocument& doc) {
            _serial->write(JSONSTART);
            serializeJson(doc, *_serial);
            _serial->write(JSONEND);
        }

        // Sends only one JSON. Open, transmitt, and close.
        void sendMsg(const JsonDocument& doc) {
            if(!_transmitting) startTransmission();
            transmit(doc);
            endTransmission();
        }

       // Get the transmitted JSON and put in the defined JsonDocument
        bool updateDoc(JsonDocument& targetDoc) {
            if (!_inFrame) return false;         // Ajustado de transmission para _inFrame

            if (testConsumption(JSONSTART)) {
            targetDoc.clear();
            _timeSTARTED = millis();        // Reset do timeout ao receber dados
            _err = deserializeJson(targetDoc, _serial->readStringUntil(JSONEND));
            return !_err;
            }
            return false;
        }

    private:
        
        static const uint8_t TRANSMISSION = 0xC0;   // END - RFC 1055
        static const uint8_t JSONSTART    = 0x1E;   // RECORD SEPARATOR - RFC 7464
        static const uint8_t JSONEND      = 0x0A;   // LINE FEED - RFC 7464
        
        Stream* _serial;
        DeserializationError _err;

        unsigned long   _maxFRAMEtime   = 2200;
        unsigned long   _timeSTARTED    = 0;
        bool            _inFrame        = false;
        bool            _transmitting   = false;
        

        /* Helpers */
        // Return true is the 
        bool test(const uint8_t t){
        if(_serial->available() > 0){
            if(_serial->peek() != t) return false;
            if(_serial->read() == t) return true;
        }
        return false;
        }

        bool testConsumption(const uint8_t t){
        if(_serial->available() > 0){
            if(_serial->read() == t) return true;
        }
        return false;
        }


        bool timeout(){
        if(millis() < (_timeSTARTED+_maxFRAMEtime)) return false;
        return true;
        }

};