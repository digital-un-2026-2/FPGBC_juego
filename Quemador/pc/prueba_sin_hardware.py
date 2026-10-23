#!/usr/bin/env python3
"""Prueba fpgbc_cart.py sin consola: reemplaza el puerto serial por un puente
falso que imita el protocolo, el MBC5 y la flash.

    python prueba_sin_hardware.py ../../ejemplo.gb
"""
import os
import runpy
import sys
import types


class PuenteFalso:
    def __init__(self):
        self.rom = bytearray(b"\xff" * 0x80000)
        self.bank = 1
        self.st = "R"
        self.ident = False
        self.entrada = bytearray()
        self.salida = bytearray()

    def reset_input_buffer(self):
        pass

    def _phys(self, a):
        return self.bank * 0x4000 + (a & 0x3FFF) if a & 0x4000 else a & 0x3FFF

    def _escribir_bus(self, a, d):
        if a >= 0x8000:
            return
        if 0x2000 <= a < 0x3000:
            self.bank = (self.bank & 0x100) | d
        elif 0x3000 <= a < 0x4000:
            self.bank = (self.bank & 0xFF) | ((d & 1) << 8)
        lo = a & 0xFFF
        if d == 0xF0 and self.st != "P":         # en programacion, F0 es un dato
            self.st, self.ident = "R", False
            return
        st = self.st
        if st == "R":
            self.st = "C1" if (lo, d) == (0xAAA, 0xAA) else "R"
        elif st == "C1":
            self.st = "C2" if (lo, d) == (0x555, 0x55) else "R"
        elif st == "C2":
            self.st = {0xA0: "P", 0x80: "E3"}.get(d, "R") if lo == 0xAAA else "R"
            self.ident = lo == 0xAAA and d == 0x90
        elif st == "P":
            self.rom[self._phys(a)] &= d
            self.st = "R"
        elif st == "E3":
            self.st = "E4" if d == 0xAA else "R"
        elif st == "E4":
            self.st = "E5" if d == 0x55 else "R"
        elif st == "E5":
            if d == 0x10:
                self.rom[:] = b"\xff" * len(self.rom)
            elif d == 0x30:
                p = self._phys(a)
                tam = 0x10000 if p >= 0x10000 else 0x2000
                base = p - p % tam
                self.rom[base:base + tam] = b"\xff" * tam
            self.st = "R"

    def _leer_bus(self, a):
        if self.ident:
            return {0: 0x01, 2: 0x7E}.get(a & 0xFF, 0)
        return self.rom[self._phys(a)]

    def write(self, datos):
        self.entrada += datos
        while self.entrada:
            c = chr(self.entrada[0])
            if c == "V":
                self.salida += b"FPGBC1"
                del self.entrada[:1]
            elif c == "R" and len(self.entrada) >= 5:
                a = int.from_bytes(self.entrada[1:3], "big")
                n = int.from_bytes(self.entrada[3:5], "big") or 65536
                self.salida += bytes(self._leer_bus((a + i) & 0xFFFF) for i in range(n))
                del self.entrada[:5]
            elif c in "WP" and len(self.entrada) >= 4:
                a = int.from_bytes(self.entrada[1:3], "big")
                d = self.entrada[3]
                if c == "P":
                    for ca, cd in ((0xAAA, 0xAA), (0x555, 0x55), (0xAAA, 0xA0)):
                        self._escribir_bus(ca, cd)
                self._escribir_bus(a, d)
                self.salida += b"K"
                del self.entrada[:4]
            else:
                break

    def read(self, n):
        r = bytes(self.salida[:n])
        del self.salida[:n]
        return r


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    rom = sys.argv[1]
    puente = PuenteFalso()
    sys.modules["serial"] = types.SimpleNamespace(Serial=lambda *a, **k: puente)
    import time
    time.sleep = lambda s: None
    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fpgbc_cart.py")
    for args in (["info"], ["burn", rom], ["header"], ["verify", rom]):
        print(f"\n=== fpgbc_cart.py {' '.join(args)}")
        sys.argv = ["fpgbc_cart.py", "--port", "falso"] + args
        try:
            runpy.run_path(script, run_name="__main__")
        except SystemExit as e:
            if e.code:
                print(e.code)
                sys.exit(1)


if __name__ == "__main__":
    main()
