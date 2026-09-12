#!/usr/bin/env bash

set -e

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "${1:-}" ]]
then
    echo "Usage: $0 <serial-port>"
    echo "Example: $0 /dev/ttyACM0"
    exit 1
fi

PORT="$1"

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
fi

python3 neckClient.py --port "$PORT"