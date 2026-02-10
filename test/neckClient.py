#!/usr/bin/env python3
# pip install pyserial

import sys
import time
import argparse
import serial

# Framing OFICIAL
SLIP_END   = 0xC0  # RFC 1055 (Fim/Início do enquadramento)
JSON_START = 0x1E  # RFC 7464 (RS - Record Separator)
JSON_END   = 0x0A  # RFC 7464 (LF - Line Feed)

def bytes_debug_view(data: bytes) -> str:
    out = []
    for b in data:
        if b == SLIP_END:          # 0xC0
            out.append("[END]")
        elif b == JSON_START:      # 0x1E
            out.append("[RS]")
        elif b == 0x0A:
            out.append("[LF]")
        elif b == 0x0D:
            out.append("[CR]")
        elif 32 <= b <= 126:       # ASCII imprimível
            out.append(chr(b))
        else:
            out.append(f"[0x{b:02X}]")
    return "".join(out)

def read_available_lines_debug(ser) -> None:
    while True:
        line = ser.readline()  # inclui o \n (LF) se ele vier
        if not line:
            break
        print("<< " + bytes_debug_view(line))


def frame_multiple_messages(json_list: list) -> bytes:
    """
    Monta uma única transmissão SLIP contendo múltiplos registros JSON-seq.
    Estrutura: [SLIP_END] + [RS + JSON1 + LF] + [RS + JSON2 + LF] + [SLIP_END]
    """
    # Início do frame SLIP
    buffer = bytearray([SLIP_END])
    
    for json_text in json_list:
        payload = json_text.strip().encode("utf-8")
        # Cada registro JSON-seq deve começar com RS e terminar com LF
        buffer.append(JSON_START)
        buffer.extend(payload)
        buffer.append(JSON_END)
    
    # Fim do frame SLIP
    buffer.append(SLIP_END)
    return bytes(buffer)

def open_serial(port: str, baud: int, timeout: float) -> serial.Serial:
    return serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=timeout,
        write_timeout=None,
    )

def read_available_lines(ser: serial.Serial) -> None:
    while True:
        line = ser.readline()
        if not line:
            break

        # Remove bytes de framing do protocolo do texto exibido
        line = line.replace(bytes([SLIP_END]), b"")
        if line.startswith(bytes([JSON_START])):
            line = line[1:]  # remove RS no começo

        # Opcional: remove LF/CR finais para imprimir padronizado
        line = line.rstrip(b"\r\n")

        if line:
            print("<< " + line.decode("utf-8", errors="replace"))


def interactive(ser: serial.Serial) -> None:
    print('Digite um ou mais JSONs separados por "|" para enviar na mesma transmissão.')
    print('Exemplo: {"msg":"A"} | {"msg":"B"}')
    print('Comandos: /quit  /raw <texto>  /ping')
    print()

    while True:
        try:
            user_in = input(">> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nSaindo.")
            return

        if not user_in:
            #read_available_lines(ser)
            read_available_lines_debug(ser)
            continue

        if user_in == "/quit":
            return

        if user_in.startswith("/raw "):
            raw = user_in[len("/raw "):]
            ser.write(raw.encode("utf-8"))
            ser.flush()
            time.sleep(0.05)
            #read_available_lines(ser)
            read_available_lines_debug(ser)
            continue

        # Possibilita múltiplos JSONs na mesma linha usando o separador |
        messages = [msg.strip() for msg in user_in.split('|')]
        
        # Envia todos no mesmo envelope SLIP
        msg_packet = frame_multiple_messages(messages)
        ser.write(msg_packet)
        ser.flush()

        time.sleep(0.05)
        read_available_lines_debug(ser)

def main():
    ap = argparse.ArgumentParser(description="UART JSON-seq over SLIP Client")
    ap.add_argument("--port", required=True, help="Ex: /dev/ttyUSB0 ou COM3")
    ap.add_argument("--baud", type=int, default=115200, help="Baudrate")
    ap.add_argument("--timeout", type=float, default=1.0, help="Timeout de leitura")
    args = ap.parse_args()

    try:
        ser = open_serial(args.port, args.baud, args.timeout)
    except Exception as e:
        print(f"Erro abrindo porta {args.port}: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Conectado em {ser.port} @ {ser.baudrate} baud.")
    try:
        time.sleep(1.5) # Aguarda reset da placa
        ser.reset_input_buffer()
        interactive(ser)
    finally:
        ser.close()

if __name__ == "__main__":
    main()