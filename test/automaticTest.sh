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
        .response == "executed" and
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
        .trieb == "perceptObserved" and
        .element == "sensor" and
        .drang == 0.25 and
        (has("args") | not)
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
        .response == "executed" and
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
        ["resetTriebs", "returnRejected", "returnUnable", "setDrang", "setRunning", "testTrieb", "testTypes"] | sort
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
        .response == "executed" and
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
        | .response
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
        | .response
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
    --request '{"msg":"testTypes","args":[true,-7,4000000000,1.25,"neck"]} | {"msg":"getPercepts"}')

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
# Validate standalone typedImpulse on the following getPercepts request.
# The Trieb has no args and is not emitted directly by the action.
# Both requests are sent in one serial session to preserve the device state.
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    ([.[] | select(.request == "getPercepts" and .response == "executed")] | length) == 1 and
    ([.[] | select(.trieb == "typedImpulse" and .element == "actuator" and
                    .drang == 0.5 and (has("args") | not))] | length) == 1
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
        .response == "executed" and
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
    --request '{"msg":"testTypes","args":[true,-7,1000,1.25,"neck"]} | {"msg":"getPercepts"}')

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
        .response == "invalid" and
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
# An invalid action must not activate typedImpulse in the subsequent getPercepts.
#
if echo "$JSON_ARRAY" | "$JQ" -e '
    ([.[] | select(.request == "getPercepts" and .response == "executed")] | length) == 1 and
    ([.[] | select(.trieb == "typedImpulse")] | length) == 0
' >/dev/null
then
    echo "testTypes invalid activates no trieb: PASS"
else
    echo "testTypes invalid activates no trieb: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK STRICT TYPES TEST: PASS"
echo "========================================"

echo ""
echo "========================================"
echo "Testing NECK Trieb intensity and transmission..."
echo "========================================"
echo ""

# Each request is handled in one serial session. Action responses must not
# contain Triebs; only the subsequent getPercepts may include them.
# Boundary checks: 0.111 (active), 0 (silent), -0.7 (silent),
# 5 (silent), 0.001 (active), 1 (active).
RAW_RESPONSE=$(python3 neckClient.py \
    --port "$ARDUINO_PORT" \
    --request '{"msg":"testTrieb"} | {"msg":"getPercepts"} | {"msg":"setDrang","args":[0.0]} | {"msg":"getPercepts"}')

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

# Check the ordered responses, including that each active Trieb belongs
# to its own getPercepts transaction and no Trieb is emitted by an action.
# Every getPercepts response includes seven percepts plus the header;
# identify the boundaries using response headers rather than message counts.
if echo "$JSON_ARRAY" | "$JQ" -e '
    (
        [.[] | select(.request == "testTrieb" or .request == "setDrang") | .response]
        == ["executed", "executed"]
    ) and
    (
        [.[] | select(.request == "getPercepts" and .response == "executed")]
        | length == 2
    ) and
    (
        [.[] | select(.trieb == "attention") | .drang]
        == [0.111]
    ) and
    (
        [.[] | select(.trieb == "attention")]
        | all(.[]; .element == "actuator" and (has("args") | not))
    )
' >/dev/null
then
    echo "Trieb intensity and suppression: PASS"
else
    echo "Trieb intensity and suppression: FAIL"
    exit 1
fi

# Check exact placement in the response stream: each Trieb must occur after
# the corresponding getPercepts header and before the next response header.
if echo "$JSON_ARRAY" | "$JQ" -e '
    reduce .[] as $item ({groups: [], current: null};
        if $item.request != null then
            if .current == null then
                .current = {request: $item.request, triebs: []}
            else
                .groups += [.current]
                | .current = {request: $item.request, triebs: []}
            end
        elif $item.trieb != null and .current != null then
            .current.triebs += [$item]
        else
            .
        end
    )
    | .groups += (if .current == null then [] else [.current] end)
    | [.groups[]
        | select(.request == "getPercepts")
        | [.triebs[]
            | select(.trieb == "attention")
            | .drang
          ]
      ]
    | . == [[0.111], []]
' >/dev/null
then
    echo "Trieb request-scoped transmission: PASS"
else
    echo "Trieb request-scoped transmission: FAIL"
    exit 1
fi

echo ""
echo "========================================"
echo "NECK TRIEB TEST: PASS"
echo "========================================"

# --------------------------------------------------------------------------
# Trieb Drang boundary tests
# --------------------------------------------------------------------------

echo ""
echo "========================================"
echo "Testing NECK Trieb Drang boundaries..."
echo "========================================"
echo ""

test_drang_boundary() {
    local value="$1"
    local expected="$2"
    local raw_response
    local json_response
    local json_array

    raw_response=$(python3 neckClient.py \
        --port "$ARDUINO_PORT" \
        --request "{\"msg\":\"setDrang\",\"args\":[$value]} | {\"msg\":\"getPercepts\"}")

    json_response=$(printf '%s\n' "$raw_response" \
        | grep '^<< ' \
        | sed 's/^<< //' \
        | sed 's/\[END\]//g' \
        | sed 's/\[LF\]/\n/g' \
        | sed 's/\[RS\]//g' \
        | sed '/^[[:space:]]*$/d')

    json_array=$(printf '%s\n' "$json_response" | "$JQ" -s -c '.')

    if ! printf '%s\n' "$json_array" | "$JQ" -e \
        --argjson value "$value" \
        --arg expected "$expected" '
        (
            [.[] | select(.request == "setDrang") | .response]
            == ["executed"]
        ) and
        (
            [.[] | select(.request == "getPercepts" and .response == "executed")]
            | length == 1
        ) and
        (
            [.[] | select(.trieb == "perceptObserved" and
                          .element == "sensor" and
                          .drang == 0.25)]
            | length == 1
        ) and
        (
            if $expected == "transmitted" then
                [.[] | select(.trieb == "attention")]
                | length == 1 and
                  .[0].element == "actuator" and
                  .[0].drang == $value and
                  (.[0] | has("args") | not)
            else
                [.[] | select(.trieb == "attention")]
                | length == 0
            end
        )
    ' >/dev/null
    then
        echo "Drang $value ($expected): FAIL"
        echo "$json_response"
        exit 1
    fi

    echo "Drang $value ($expected): PASS"
}

test_drang_boundary -0.7  suppressed
test_drang_boundary 5.0   suppressed
test_drang_boundary 0.001 transmitted
test_drang_boundary 1.0   transmitted

echo ""
echo "========================================"
echo "NECK TRIEB BOUNDARY TEST: PASS"
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
        .response == "unable" and
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
        .response == "rejected" and
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
        .response == "unknown" and
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
            .response == "executed"
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