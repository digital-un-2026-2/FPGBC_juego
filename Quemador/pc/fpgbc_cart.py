#!/usr/bin/env python3
"""Herramienta del PC para leer y quemar el cartucho EverSave (MBC5 + S29GL064N)
a traves del puente cart_bridge cargado en la FPGA de la consola FPGBC.

Uso:
    python fpgbc_cart.py --port /dev/ttyUSB0 info          # enlace + ID de la flash
    python fpgbc_cart.py --port /dev/ttyUSB0 header        # lee y decodifica la cabecera
    python fpgbc_cart.py --port /dev/ttyUSB0 dump salida.gb --size 32768
    python fpgbc_cart.py --port /dev/ttyUSB0 erase         # borra la flash completa
    python fpgbc_cart.py --port /dev/ttyUSB0 burn juego.gb # borra, programa y verifica
    python fpgbc_cart.py --port /dev/ttyUSB0 verify juego.gb

Protocolo (ver rtl/cart_bridge.v):
    'V'                 -> "FPGBC1"
    'R' aH aL nH nL     -> n bytes leidos
    'W' aH aL d         -> 'K'
    'P' aH aL d         -> 'K' (programado) o 'X' (fallo)
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("Falta pyserial: pip install pyserial")

BANK_SIZE = 0x4000
NINTENDO_LOGO = bytes.fromhex(
    "CEED6666CC0D000B03730083000C000D0008111F8889000E"
    "DCCC6EE6DDDDD999BBBB67636E0EECCCDDDC999FBBB9333E"
)
CART_TYPES = {0x00: "ROM ONLY", 0x01: "MBC1", 0x13: "MBC3+RAM+BATTERY",
              0x19: "MBC5", 0x1A: "MBC5+RAM", 0x1B: "MBC5+RAM+BATTERY"}
FLASH_MANUFACTURER = 0x01    # Spansion / Cypress / Infineon
FLASH_DEVICE = 0x7E          # S29GL064N


class CartBridge:
    def __init__(self, port, baud):
        self.ser = serial.Serial(port, baud, timeout=2)
        time.sleep(0.1)
        self.ser.reset_input_buffer()

    # ---------- comandos basicos ----------
    def _expect(self, n, what):
        data = self.ser.read(n)
        if len(data) != n:
            raise IOError(f"{what}: esperaba {n} bytes y llegaron {len(data)}")
        return data

    def version(self):
        self.ser.write(b"V")
        return self._expect(6, "version").decode(errors="replace")

    def read(self, addr, length):
        """Lee length bytes (max 65536) del bus de la consola desde addr."""
        out = bytearray()
        while length > 0:
            n = min(length, 4096)
            self.ser.write(b"R" + addr.to_bytes(2, "big") + (n & 0xFFFF).to_bytes(2, "big"))
            out += self._expect(n, f"lectura en {addr:#06x}")
            addr += n
            length -= n
        return bytes(out)

    def write(self, addr, value):
        self.ser.write(b"W" + addr.to_bytes(2, "big") + bytes([value]))
        if self._expect(1, "ack W") != b"K":
            raise IOError(f"escritura en {addr:#06x} sin confirmacion")

    def program_many(self, items):
        """items: lista de (direccion_bus, dato). Se envian en rafagas para
        aprovechar la FIFO de 512 bytes del puente (4 bytes por comando)."""
        burst = 64
        for i in range(0, len(items), burst):
            chunk = items[i:i + burst]
            self.ser.write(b"".join(b"P" + a.to_bytes(2, "big") + bytes([d]) for a, d in chunk))
            acks = self._expect(len(chunk), "acks P")
            if acks != b"K" * len(chunk):
                bad = chunk[acks.index(b"X")]
                self.flash_reset()
                raise IOError(f"fallo programando {bad[1]:#04x} en {bad[0]:#06x}")

    # ---------- MBC5 ----------
    def set_bank(self, bank):
        self.write(0x2000, bank & 0xFF)
        self.write(0x3000, (bank >> 8) & 0x01)

    def read_physical(self, start, length, progress=None):
        """Lee la ROM por direccion fisica usando la ventana 0x4000-0x7FFF."""
        out = bytearray()
        pos = start
        while pos < start + length:
            bank, off = divmod(pos, BANK_SIZE)
            n = min(BANK_SIZE - off, start + length - pos)
            self.set_bank(bank)
            out += self.read(0x4000 + off, n)
            pos += n
            if progress:
                progress(pos - start, length)
        return bytes(out)

    # ---------- flash S29GL064N (modo byte) ----------
    def unlock(self):
        self.write(0x0AAA, 0xAA)
        self.write(0x0555, 0x55)

    def flash_reset(self):
        self.write(0x0000, 0xF0)

    def flash_id(self):
        self.unlock()
        self.write(0x0AAA, 0x90)
        ident = self.read(0x0000, 4)
        self.flash_reset()
        return ident[0], ident[2]

    def _wait_erased(self, bus_addr, timeout_s):
        t0 = time.time()
        while time.time() - t0 < timeout_s:
            if self.read(bus_addr, 1)[0] == 0xFF:
                return
            time.sleep(0.05)
        self.flash_reset()
        raise IOError("el borrado no termino a tiempo")

    def erase_chip(self, timeout_s=300):
        self.unlock()
        self.write(0x0AAA, 0x80)
        self.unlock()
        self.write(0x0AAA, 0x10)
        self._wait_erased(0x0000, timeout_s)

    def erase_sector(self, phys):
        self.set_bank(phys // BANK_SIZE)
        bus = 0x4000 + phys % BANK_SIZE
        self.unlock()
        self.write(0x0AAA, 0x80)
        self.unlock()
        self.write(bus, 0x30)
        self._wait_erased(bus, 10)

    def erase_range(self, size):
        """Borra los sectores que cubren [0, size). Primeros 64 KB en pasos de
        8 KB (por si la variante tiene sectores pequenos abajo), luego de 64 KB."""
        addrs = list(range(0, min(size, 0x10000), 0x2000))
        addrs += list(range(0x10000, size, 0x10000))
        for i, a in enumerate(addrs):
            print(f"\r  borrando sector {i + 1}/{len(addrs)} ({a:#08x})", end="", flush=True)
            self.erase_sector(a)
        print()

    def program_physical(self, start, data, progress=None):
        pos = 0
        while pos < len(data):
            phys = start + pos
            bank, off = divmod(phys, BANK_SIZE)
            n = min(BANK_SIZE - off, len(data) - pos)
            self.set_bank(bank)
            items = [(0x4000 + off + i, b) for i, b in enumerate(data[pos:pos + n]) if b != 0xFF]
            self.program_many(items)
            pos += n
            if progress:
                progress(pos, len(data))


def bar(done, total):
    pct = 100 * done // max(total, 1)
    print(f"\r  [{'#' * (pct // 4):<25}] {pct:3d}%  {done}/{total} bytes", end="", flush=True)


def describe_header(h):
    """h = bytes 0x0100..0x014F"""
    title = h[0x34:0x44].split(b"\x00")[0].decode("ascii", errors="replace")
    logo_ok = h[0x04:0x34] == NINTENDO_LOGO
    ctype, rom_code, ram_code = h[0x47], h[0x48], h[0x49]
    chk = 0
    for b in h[0x34:0x4D]:
        chk = (chk - b - 1) & 0xFF
    print(f"  Logo de Nintendo : {'correcto' if logo_ok else 'NO coincide'}")
    print(f"  Titulo           : {title!r}")
    print(f"  Tipo de cartucho : {ctype:#04x} ({CART_TYPES.get(ctype, 'otro')})")
    print(f"  Tamano de ROM    : {32 << rom_code} KB (codigo {rom_code:#04x})")
    print(f"  Codigo de RAM    : {ram_code:#04x}")
    print(f"  Checksum cabecera: {'correcto' if chk == h[0x4D] else 'NO coincide'}")
    return logo_ok


def main():
    ap = argparse.ArgumentParser(description="Lectura y quemado del cartucho FPGBC")
    ap.add_argument("--port", required=True, help="puerto serial, p. ej. /dev/ttyUSB0 o COM3")
    ap.add_argument("--baud", type=int, default=115200)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("info")
    sub.add_parser("header")
    p = sub.add_parser("dump"); p.add_argument("archivo"); p.add_argument("--size", type=int, default=32768)
    sub.add_parser("erase")
    p = sub.add_parser("burn"); p.add_argument("archivo"); p.add_argument("--chip-erase", action="store_true",
                                                                          help="borrar el chip completo en vez de solo los sectores necesarios")
    p = sub.add_parser("verify"); p.add_argument("archivo")
    args = ap.parse_args()

    cart = CartBridge(args.port, args.baud)
    ver = cart.version()
    if ver != "FPGBC1":
        sys.exit(f"El puente no respondio bien ({ver!r}). Revisa el cable, el puerto y que el bitstream este cargado.")

    if args.cmd == "info":
        print(f"Puente: {ver}")
        man, dev = cart.flash_id()
        print(f"Flash: fabricante {man:#04x}, dispositivo {dev:#04x}")
        if (man, dev) == (FLASH_MANUFACTURER, FLASH_DEVICE):
            print("  -> S29GL064N reconocida: las escrituras llegan a la flash.")
        else:
            print("  -> No es el ID esperado (0x01, 0x7e). Si ves bytes de la ROM normal,")
            print("     las escrituras no llegan a la flash: revisa DIR del level shifter.")

    elif args.cmd == "header":
        h = cart.read(0x0100, 0x50)
        print("Cabecera (0x0100-0x014F):")
        describe_header(h)

    elif args.cmd == "dump":
        data = cart.read_physical(0, args.size, bar)
        print()
        open(args.archivo, "wb").write(data)
        print(f"Guardado {len(data)} bytes en {args.archivo}")

    elif args.cmd == "erase":
        print("Borrando el chip completo (puede tardar mas de un minuto)...")
        cart.erase_chip()
        print("Listo.")

    elif args.cmd in ("burn", "verify"):
        rom = open(args.archivo, "rb").read()
        print(f"{args.archivo}: {len(rom)} bytes")
        describe_header(rom[0x100:0x150])
        if args.cmd == "burn":
            t0 = time.time()
            if args.chip_erase:
                print("Borrando el chip completo...")
                cart.erase_chip()
            else:
                print("Borrando sectores...")
                cart.erase_range(len(rom))
            print("Programando...")
            cart.program_physical(0, rom, bar)
            print(f"\n  programado en {time.time() - t0:.1f} s")
        print("Verificando...")
        back = cart.read_physical(0, len(rom), bar)
        print()
        diffs = [i for i in range(len(rom)) if rom[i] != back[i]]
        if diffs:
            i = diffs[0]
            sys.exit(f"FALLO: {len(diffs)} bytes distintos; el primero en {i:#08x} "
                     f"(esperado {rom[i]:#04x}, leido {back[i]:#04x})")
        print("OK: el cartucho coincide con el archivo.")


if __name__ == "__main__":
    main()
