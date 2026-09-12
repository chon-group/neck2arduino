#!/bin/bash

ARDUINO_CLI="/usr/bin/arduino-cli"
JQ="/usr/bin/jq"
HERE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SKETCH="$HERE/../examples/allInOne/"
FQBN="arduino:avr:uno"

cd "$HERE"
clear

echo "========================================"
echo "       NECK - Compilation Test"
echo "========================================"
echo ""

#
# 1. Arduino CLI
#
if [[ ! -f "$ARDUINO_CLI" || ! -f "$JQ" ]]
then
    echo "Arduino CLI not found."
    echo "Installing Arduino CLI..."
    echo ""
    echo "deb [trusted=yes] http://packages.chon.group/ chonos main" | sudo tee /etc/apt/sources.list.d/chonos.list

    sudo apt update
    sudo apt install -y arduino-cli jq
else
    echo "Arduino CLI already installed."
fi

echo ""

#
# 2. Arduino hardware
#
echo "Checking Arduino connection..."

ARDUINO_PORT=""
BOARD_LIST="$(arduino-cli board list)"

if echo "$BOARD_LIST" | grep -q "^/dev/ttyUSB0"
then
    ARDUINO_PORT="/dev/ttyUSB0"

elif echo "$BOARD_LIST" | grep -q "^/dev/ttyACM0"
then
    ARDUINO_PORT="/dev/ttyACM0"

else
    echo ""
    echo "========================================"
    echo "NECK TEST: FAIL"
    echo "========================================"
    echo "No Arduino was detected by arduino-cli."
    echo ""
    echo "Expected:"
    echo "  /dev/ttyUSB0"
    echo "or"
    echo "  /dev/ttyACM0"
    echo ""
    echo "$BOARD_LIST"
    echo ""
    exit 1
fi

echo "Arduino detected at $ARDUINO_PORT"
echo ""

#
# 3. Arduino AVR core
#
if ! arduino-cli core list | grep -q "^arduino:avr "
then
    echo "Arduino AVR core not found."
    echo "Updating Arduino indexes..."

    arduino-cli core update-index

    echo "Installing Arduino AVR core..."
    arduino-cli core install arduino:avr
else
    echo "Arduino AVR core already installed."
fi

echo ""

#
# 6. Compile
#
echo "========================================"
echo "Compiling NECK validation sketch..."
echo "Board: $FQBN"
echo "========================================"
echo ""

arduino-cli compile \
    --fqbn "$FQBN" \
    "$SKETCH"

RESULT=$?

echo ""

if [[ $RESULT -eq 0 ]]
then
    echo "========================================"
    echo "NECK COMPILATION TEST: PASS"
    echo "========================================"
else
    echo "========================================"
    echo "NECK COMPILATION TEST: FAIL"
    echo "========================================"
    exit 1
fi

#
# 7. Upload
#
echo ""
echo "========================================"
echo "Uploading NECK validation sketch..."
echo "Board: $FQBN"
echo "Port:  $ARDUINO_PORT"
echo "========================================"
echo ""

arduino-cli upload \
    --port "$ARDUINO_PORT" \
    --fqbn "$FQBN" \
    "$SKETCH"

UPLOAD_RESULT=$?

echo ""

if [[ $UPLOAD_RESULT -eq 0 ]]
then
    echo "========================================"
    echo "NECK DEPLOY TEST: PASS"
    echo "========================================"
else
    echo "========================================"
    echo "NECK DEPLOY TEST: FAIL"
    echo "========================================"
    exit 1
fi

#
# 6. Python environment
#
echo ""
echo "Checking Python virtual environment..."

cd "$HERE"

if [[ ! -d ".venv" ]]
then
    echo "Creating Python virtual environment..."
    python3 -m venv .venv
fi

source .venv/bin/activate

if ! python3 -c "import serial" 2>/dev/null
then
    echo "Installing pyserial..."
    pip install pyserial
else
    echo "pyserial already installed."
fi

echo ""

#
# 7. NECK percepts test
#
echo ""
echo "========================================"
echo "Testing NECK percepts..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"getPercepts"}')

#echo "RAW RESPONSE: $RAW_RESPONSE"

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

echo ""
echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')
echo ""
#echo "JSON Array:"
#echo "$JSON_ARRAY" | "$JQ" -c .

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[]; 
        .apparatus == "validationBody" and
        .bodyResponse == "executed" and
        .request == "getPercepts" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "getPercepts response header: PASS"
else
    echo "getPercepts response header: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "cycleDelta" and
        .element == "sensor" and
        .type == "proprioception" and
        .status == "percepted" and
        .args == [1]
    )
' >/dev/null
then
    echo "cycleDelta: PASS"
else
    echo "cycleDelta: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "unchangedValue" and
        .element == "sensor" and
        .type == "interoception" and
        .status == "unchanged" and
        (has("args") | not)
    )
' >/dev/null
then
    echo "unchangedValue: PASS"
else
    echo "unchangedValue: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "unavailableValue" and
        .element == "sensor" and
        .type == "exteroception" and
        .status == "unavailable" and
        (has("args") | not)
    )
' >/dev/null
then
    echo "unavailableValue: PASS"
else
    echo "unavailableValue: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "mixedValues" and
        .element == "sensor" and
        .type == "interoception" and
        .status == "percepted" and
        .args == [true, -7, 4000000000, 1.25, "neck"]
    )
' >/dev/null
then
    echo "mixedValues: PASS"
else
    echo "mixedValues: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "movementSteps" and
        .element == "actuator" and
        .type == "proprioception" and
        .status == "percepted" and
        .args == [0]
    )
' >/dev/null
then
    echo "movementSteps: PASS"
else
    echo "movementSteps: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "internalLoad" and
        .element == "sensor" and
        .type == "interoception" and
        .status == "percepted" and
        .args == [0.5]
    )
' >/dev/null
then
    echo "internalLoad: PASS"
else
    echo "internalLoad: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .percept == "externalValue" and
        .element == "sensor" and
        .type == "exteroception" and
        .status == "percepted" and
        .args == [23.5]
    )
' >/dev/null
then
    echo "externalValue: PASS"
else
    echo "externalValue: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .trieb == "perceptObserved" and
        .element == "sensor" and
        .drang == 0.25 and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "perceptObserved trieb: PASS"
else
    echo "perceptObserved trieb: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK PERCEPTS TEST: PASS"
echo "========================================"


echo ""
echo "========================================"
echo "Testing NECK actions..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"getActions"}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')


if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "executed" and
        .request == "getActions" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "getActions response header: PASS"
else
    echo "getActions response header: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    (
        [.[] | select(.action != null) | .action] | sort
    ) == (
        ["returnRejected", "returnUnable", "setRunning", "testTrieb", "testTypes"] | sort
    )
' >/dev/null
then
    echo "getActions list: PASS"
else
    echo "getActions list: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    all(
        .[] | select(.action != null);
        .element == "actuator"
    )
' >/dev/null
then
    echo "getActions elements: PASS"
else
    echo "getActions elements: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK ACTIONS TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK know-how..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"getKnowHow"}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "executed" and
        .request == "getKnowHow" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "getKnowHow response header: PASS"
else
    echo "getKnowHow response header: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .skill == "startMovement" and
        .context == null and
        .plan == "+!start <- .myBody.act(setRunning(true))."
    )
' >/dev/null
then
    echo "startMovement: PASS"
else
    echo "startMovement: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .skill == "safeMovement" and
        .context == "battery(Level) & Level > 20" and
        .plan == "+!start <- .myBody.act(setRunning(true))."
    )
' >/dev/null
then
    echo "safeMovement: PASS"
else
    echo "safeMovement: FAIL"
    exit 1
fi

if echo "$JSON_ARRAY" | "$JQ" -e '
    (
        [.[] | select(.skill != null) | .skill] | sort
    ) == (
        ["safeMovement", "startMovement"] | sort
    )
' >/dev/null
then
    echo "getKnowHow list: PASS"
else
    echo "getKnowHow list: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK KNOW-HOW TEST: PASS"
echo "========================================"


echo ""
echo "========================================"
echo "Testing NECK action execution..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"setRunning","args":[true]} | {"msg":"setRunning","args":[true]}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

if echo "$JSON_ARRAY" | "$JQ" -e '
    [
        .[]
        | select(
            .request == "setRunning" and
            .element == "actuator"
        )
        | .bodyResponse
    ] == ["executed", "already"]
' >/dev/null
then
    echo "setRunning executed/already: PASS"
else
    echo "setRunning executed/already: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK ACTION EXECUTION TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK action arguments..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"setRunning","args":[true]} | {"msg":"setRunning","args":["true"]} | {"msg":"setRunning","args":[false]}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

if echo "$JSON_ARRAY" | "$JQ" -e '
    [
        .[]
        | select(
            .request == "setRunning" and
            .element == "actuator"
        )
        | .bodyResponse
    ] == ["executed", "invalid", "executed"]
' >/dev/null
then
    echo "setRunning type/state validation: PASS"
else
    echo "setRunning type/state validation: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK ACTION ARGUMENTS TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK action argument types..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"testTypes","args":[true,-7,4000000000,1.25,"neck"]}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

#
# Validate typedImpulse trieb
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .trieb == "typedImpulse" and
        .element == "actuator" and
        .drang == 0.5 and
        .args == [true, -7, 4000000000, 1.25, "neck"] and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "testTypes typedImpulse trieb: PASS"
else
    echo "testTypes typedImpulse trieb: FAIL"
    exit 1
fi

#
# Validate action response
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "executed" and
        .request == "testTypes" and
        .element == "actuator" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "testTypes executed: PASS"
else
    echo "testTypes executed: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK ACTION TYPES TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK strict argument types..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"testTypes","args":[true,-7,1000,1.25,"neck"]}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

#
# 1000 is parsed as INT, not UINT.
# Therefore ActionArgs.isUInt(2) must fail.
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "invalid" and
        .request == "testTypes" and
        .element == "actuator" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "testTypes strict uint validation: PASS"
else
    echo "testTypes strict uint validation: FAIL"
    exit 1
fi

#
# An invalid action must not emit typedImpulse.
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .trieb == "typedImpulse"
    ) | not
' >/dev/null
then
    echo "testTypes invalid emits no trieb: PASS"
else
    echo "testTypes invalid emits no trieb: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK STRICT TYPES TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK trieb normalization..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"testTrieb"}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

#
# Validate drang sequence:
# omitted -> 1
# 0.111   -> 0.111
# 0       -> 0
# -0.7    -> 0
# 5       -> 1
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    [
        .[]
        | select(
            .trieb == "attention" and
            .element == "actuator"
        )
        | .drang
    ] == [1, 0.111, 0, 0, 1]
' >/dev/null
then
    echo "testTrieb drang normalization: PASS"
else
    echo "testTrieb drang normalization: FAIL"
    exit 1
fi

#
# Validate action response
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "executed" and
        .request == "testTrieb" and
        .element == "actuator" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "testTrieb executed: PASS"
else
    echo "testTrieb executed: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK TRIEB TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK action responses..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"returnUnable"} | {"msg":"returnRejected"} | {"msg":"actionThatDoesNotExist"}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

#
# Validate UNABLE
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "unable" and
        .request == "returnUnable" and
        .element == "actuator" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "returnUnable: PASS"
else
    echo "returnUnable: FAIL"
    exit 1
fi

#
# Validate REJECTED
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "rejected" and
        .request == "returnRejected" and
        .element == "actuator" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "returnRejected: PASS"
else
    echo "returnRejected: FAIL"
    exit 1
fi

#
# Validate UNKNOWN action
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    any(.[];
        .apparatus == "validationBody" and
        .bodyResponse == "unknown" and
        .request == "actionThatDoesNotExist" and
        (.apparatusID | type) == "number"
    )
' >/dev/null
then
    echo "unknown action: PASS"
else
    echo "unknown action: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK ACTION RESPONSES TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK body cycle regression..."
echo "========================================"
echo ""

RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"getPercepts"} | {"msg":"getPercepts"} | {"msg":"getPercepts"}')

JSON_RESPONSE=$(printf '%s\n' "$RAW_RESPONSE" \
    | grep '^<< ' \
    | sed 's/^<< //' \
    | sed 's/\[END\]//g' \
    | sed 's/\[LF\]/\n/g' \
    | sed 's/\[RS\]//g' \
    | sed '/^[[:space:]]*$/d')

JSON_RESPONSE=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -c .)

echo "Parsed JSON:"
echo "$JSON_RESPONSE" | "$JQ" -c .
echo ""

JSON_ARRAY=$(printf '%s\n' "$JSON_RESPONSE" | "$JQ" -s -c '.')

#
# Validate that all three getPercepts requests were processed.
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    [
        .[]
        | select(
            .request == "getPercepts" and
            .bodyResponse == "executed"
        )
    ]
    | length == 3
' >/dev/null
then
    echo "getPercepts repeated requests: PASS"
else
    echo "getPercepts repeated requests: FAIL"
    exit 1
fi

#
# Sensing runs before protocol processing and Behaving runs afterward.
#
# Therefore each getPercepts request must observe:
#
#   sensingCycles - behavingCycles == 1
#
# If Behaving were skipped after getPercepts, the sequence would grow
# as 1, 2, 3 instead of remaining 1, 1, 1.
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    [
        .[]
        | select(
            .percept == "cycleDelta" and
            .element == "sensor" and
            .type == "proprioception" and
            .status == "percepted"
        )
        | .args[0]
    ] == [1, 1, 1]
' >/dev/null
then
    echo "cycleDelta regression: PASS"
else
    echo "cycleDelta regression: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK BODY CYCLE TEST: PASS"
echo "========================================"