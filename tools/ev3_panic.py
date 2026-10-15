#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Decodes an EV3 panic dump printed by ev3_panic_handler() on the debug UART.

Paste the dump on stdin (or pass a file) and this prints:

- What the exception was and which instruction caused it.
- The symbol and source location of the program counter and link register.
- The decoded fault status registers (DFSR/IFSR/FAR) and the saved CPSR.
- Each register annotated with the memory region and symbol it points at.
- Which mode stack the stack pointer was on and how much of it was used.
- A disassembly window around the faulting instruction.

Symbols come from bricks/ev3/build/firmware.elf, so it must be the same build
that produced the dump. Source locations require a DEBUG=1 build.

Usage::

    ./tools/ev3_panic.py            # then paste the dump
    ./tools/ev3_panic.py panic.txt
"""

import argparse
import bisect
import os
import re
import select
import shutil
import subprocess
import sys
import time
from pathlib import Path

TOP = Path(__file__).resolve().parent.parent
DEFAULT_ELF = TOP / "bricks" / "ev3" / "build" / "firmware.elf"
SOC_HEADER = TOP / "lib" / "tiam1808" / "tiam1808" / "hw" / "soc_AM1808.h"
START_S = TOP / "lib" / "pbio" / "platform" / "ev3" / "start.S"

# Must match PBDRV_CONFIG_CACHE_UNCACHED_OFFSET in the ev3 pbdrvconfig.h.
UNCACHED_OFFSET = 0x10000000

DDR_START = 0xC0000000
DDR_SIZE = 64 * 1024 * 1024

# Regions that are not covered by ELF sections.
STATIC_REGIONS = [
    (0x00000000, 0x00001000, "null page (unmapped)"),
    (0x01800000, 0x02000000, "SoC peripheral registers"),
    (0x80000000, 0x80020000, "on-chip shared RAM (PRU)"),
    (0xFFFD0000, 0xFFFE0000, "ARM ROM"),
    (0xFFFEE000, 0xFFFF0000, "AINTC registers"),
    (0xFFFF0000, 0xFFFF2000, "ARM local RAM (vectors)"),
]

# ARM926EJ-S fault status encoding: fault name and whether the domain field is
# meaningful, indexed by FSR[3:0].
FSR_CODES = {
    0b0001: ("alignment fault", False),
    0b0011: ("alignment fault", False),
    0b0010: ("terminal exception", True),
    0b0100: ("external abort on linefetch (section)", True),
    0b0101: ("translation fault (section)", False),
    0b0110: ("external abort on linefetch (page)", True),
    0b0111: ("translation fault (page)", True),
    0b1000: ("external abort on non-linefetch (section)", True),
    0b1001: ("domain fault (section)", True),
    0b1010: ("external abort on non-linefetch (page)", True),
    0b1011: ("domain fault (page)", True),
    0b1100: ("external abort on translation (first level)", False),
    0b1110: ("external abort on translation (second level)", True),
    0b1101: ("permission fault (section)", True),
    0b1111: ("permission fault (page)", True),
}

FSR_HINTS = {
    "translation": "address is not mapped in the MMU tables, see mmu_init() in platform.c",
    "alignment": "unaligned access, e.g. a misaligned pointer cast or packed struct member",
    "permission": "access not permitted by the MMU tables",
    "domain": "access not permitted by the MMU domain configuration",
    "external abort": "bus error, e.g. a peripheral that is not powered (PSC) or a bad DMA address",
}

ARM_MODES = {
    0x10: "User",
    0x11: "FIQ",
    0x12: "IRQ",
    0x13: "Supervisor",
    0x17: "Abort",
    0x1B: "Undefined",
    0x1F: "System",
}

# Mode stacks are carved out of the top of the stack region by start.S, in this
# order. Sizes are read from start.S so they stay in sync.
MODE_STACK_ORDER = ["UND", "ABT", "FIQ", "IRQ", "SVC"]
MODE_STACK_NAMES = {
    "UND": "Undefined",
    "ABT": "Abort",
    "FIQ": "FIQ",
    "IRQ": "IRQ",
    "SVC": "Supervisor",
    "SYS": "System/User",
}


def tool(name):
    """Gets the path of a cross toolchain program, or None if not installed."""
    prefix = os.environ.get("CROSS_COMPILE", "arm-none-eabi-")
    return shutil.which(prefix + name)


def run(program, *args):
    return subprocess.run(
        [program, *args], check=True, capture_output=True, text=True
    ).stdout


def read_pasted_dump():
    """Reads a dump pasted into the terminal, ending when the paste stops."""
    print("Paste the panic dump (Ctrl-C to abort):\n", file=sys.stderr)

    lines = []
    while True:
        # The first line is waited for indefinitely. After that the rest of the
        # paste arrives at once, so a short quiet period means it is done.
        if lines and not select.select([sys.stdin], [], [], 1)[0]:
            break
        line = sys.stdin.readline()
        if not line:
            break
        lines.append(line)

    return "".join(lines)


REGISTER_RE = re.compile(r"^(R\d{1,2}|SPSR|DFSR|IFSR|FAR)\s*:\s*(?:0x)?([0-9A-Fa-f]+)$")


def parse_dump(text):
    """Extracts the register values and exception type from a panic dump.

    Anything before the exception type and anything after the last register
    value is ignored, so a rough selection of terminal output can be pasted.
    """
    dump = {}

    match = re.search(r"Exception type:\s*(.+)", text)
    if match:
        dump["type"] = match.group(1).strip()
        text = text[match.end() :]

    registers_seen = False
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        match = REGISTER_RE.match(line)
        if match:
            dump[match.group(1).upper()] = int(match.group(2), 16)
            registers_seen = True
        elif registers_seen:
            break

    if "R15" not in dump:
        raise ValueError("no registers found; is this an EV3 panic dump?")

    return dump


class Symbols:
    """Symbol table of the firmware ELF file."""

    def __init__(self, elf):
        self.addrs = []
        self.entries = []
        self.by_name = {}

        nm = tool("nm")
        if not nm:
            return

        for line in run(nm, "-S", "-n", "--defined-only", str(elf)).splitlines():
            fields = line.split()
            if len(fields) == 4:
                addr, size, kind, name = fields
                size = int(size, 16)
            elif len(fields) == 3:
                addr, kind, name = fields
                size = 0
            else:
                continue
            addr = int(addr, 16)
            self.addrs.append(addr)
            self.entries.append((addr, size, kind, name))
            self.by_name.setdefault(name, addr)

    def lookup(self, addr):
        """Gets (name, offset, is_code) for the symbol containing addr, or None."""
        index = bisect.bisect_right(self.addrs, addr) - 1
        if index < 0:
            return None

        start, size, kind, name = self.entries[index]
        limit = (
            start + size
            if size
            else self.addrs[index + 1]
            if index + 1 < len(self.addrs)
            else start + 1
        )
        if addr >= limit:
            return None

        return name, addr - start, kind in "TtWw"

    def value(self, name):
        return self.by_name.get(name)


def get_sections(elf):
    """Gets a list of (start, end, name) for the allocated ELF sections."""
    readelf = tool("readelf")
    if not readelf:
        return []

    sections = []
    pattern = re.compile(
        r"\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)"
    )
    for line in run(readelf, "-S", "-W", str(elf)).splitlines():
        match = pattern.search(line)
        if not match:
            continue
        name, addr, size = (
            match.group(1),
            int(match.group(2), 16),
            int(match.group(3), 16),
        )
        if addr and size:
            sections.append((addr, addr + size, name))

    return sorted(sections)


def get_soc_registers():
    """Gets a list of (address, name) of the AM1808 peripheral register bases."""
    if not SOC_HEADER.exists():
        return []

    registers = re.findall(
        r"#define\s+SOC_(\w+)_REGS\s+\((0x[0-9A-Fa-f]+)\)", SOC_HEADER.read_text()
    )
    return sorted((int(addr, 16), name) for name, addr in registers)


def get_mode_stack_sizes():
    """Gets the per-mode stack sizes defined in start.S."""
    if not START_S.exists():
        return {}

    sizes = re.findall(
        r"\.set\s+(\w+)_STACK_SIZE,\s*(0x[0-9A-Fa-f]+)", START_S.read_text()
    )
    return {name: int(size, 16) for name, size in sizes}


class Firmware:
    """Everything known about the firmware that crashed."""

    def __init__(self, elf):
        self.elf = elf
        self.symbols = Symbols(elf)
        self.sections = get_sections(elf)
        self.section_starts = [start for start, _, _ in self.sections]
        self.soc_registers = get_soc_registers()
        self.soc_starts = [addr for addr, _ in self.soc_registers]
        self.mode_stack_sizes = get_mode_stack_sizes()
        self._lines = {}

    def region(self, addr):
        """Gets a human readable name of the memory region containing addr."""
        index = bisect.bisect_right(self.section_starts, addr) - 1
        if index >= 0:
            start, end, name = self.sections[index]
            if addr < end:
                return name

        for start, end, name in STATIC_REGIONS:
            if start <= addr < end:
                if name == "SoC peripheral registers":
                    index = bisect.bisect_right(self.soc_starts, addr) - 1
                    if index >= 0:
                        base, register = self.soc_registers[index]
                        if addr - base < 0x1000:
                            return f"SOC_{register}_REGS + 0x{addr - base:x}"
                return name

        if DDR_START <= addr < DDR_START + DDR_SIZE:
            return "DDR RAM (outside any section)"

        return None

    def source(self, addr):
        """Gets the source locations (including inlined frames) for addr."""
        if addr not in self._lines:
            self._lines[addr] = self._addr2line(addr)
        return self._lines[addr]

    def _addr2line(self, addr):
        addr2line = tool("addr2line")
        if not addr2line:
            return []

        locations = []
        output = run(
            addr2line, "-f", "-i", "-p", "-C", "-e", str(self.elf), f"0x{addr:x}"
        )
        for line in output.splitlines():
            line = line.strip()
            if line.startswith("(inlined by)"):
                line = line[len("(inlined by)") :].strip()
            if "??" in line and ":0" in line:
                continue
            try:
                relative = (
                    Path(line.split(" at ")[-1].split(":")[0])
                    .resolve()
                    .relative_to(TOP)
                )
                line = line.replace(line.split(" at ")[-1].split(":")[0], str(relative))
            except (ValueError, IndexError):
                pass
            locations.append(line)

        return locations

    def describe(self, addr):
        """Gets a one line description of what addr points at."""
        parts = []

        target = addr
        if DDR_START + UNCACHED_OFFSET <= addr < DDR_START + UNCACHED_OFFSET + DDR_SIZE:
            target = addr - UNCACHED_OFFSET
            parts.append(f"uncached alias of 0x{target:08X}")

        region = self.region(target)
        if region:
            parts.append(region)

        symbol = self.symbols.lookup(target)
        if symbol:
            name, offset, _ = symbol
            parts.append(f"{name}+0x{offset:x}" if offset else name)

        if not parts:
            parts.append("unmapped")

        return ", ".join(parts)

    def code_location(self, addr):
        """Gets a description of a code address, or None if it is not code."""
        symbol = self.symbols.lookup(addr)
        if not symbol or not symbol[2]:
            return None

        name, offset, _ = symbol
        text = f"{name}+0x{offset:x}" if offset else name
        source = self.source(addr)
        if source:
            text += "\n" + "\n".join(f"    {line}" for line in source)

        return text

    def disassemble(self, addr, before=6, after=4):
        """Gets the disassembly around addr, marking the instruction at addr."""
        objdump = tool("objdump")
        if not objdump or not self.symbols.lookup(addr):
            return []

        start = addr - before * 4
        stop = addr + after * 4
        output = run(
            objdump,
            "-d",
            "-C",
            f"--start-address=0x{start:x}",
            f"--stop-address=0x{stop:x}",
            str(self.elf),
        )

        lines = []
        for line in output.splitlines():
            match = re.match(r"^\s*([0-9a-f]+):\t", line)
            if match:
                marker = "=>" if int(match.group(1), 16) == addr else "  "
                lines.append(f"{marker} {line.strip()}")
            elif re.match(r"^[0-9a-f]+ <.*>:$", line):
                lines.append(f"   {line}")

        return lines


def decode_fsr(value):
    """Describes a fault status register value."""
    status = value & 0xF
    domain = (value >> 4) & 0xF

    name, domain_valid = FSR_CODES.get(status, ("unknown fault status", False))
    text = f"0x{value:08X}: {name}"
    if domain_valid:
        text += f", domain {domain}"

    hint = next((h for key, h in FSR_HINTS.items() if key in name), None)

    return text, hint


def decode_psr(value):
    """Describes a program status register value."""
    flags = "".join(
        letter if value & (1 << bit) else "-"
        for letter, bit in (("N", 31), ("Z", 30), ("C", 29), ("V", 28), ("Q", 27))
    )
    mode = ARM_MODES.get(value & 0x1F, f"unknown (0x{value & 0x1F:02X})")
    state = "Thumb" if value & (1 << 5) else "ARM"
    irq = "disabled" if value & (1 << 7) else "enabled"
    fiq = "disabled" if value & (1 << 6) else "enabled"

    return f"{mode} mode, {state} state, IRQ {irq}, FIQ {fiq}, flags {flags}"


def describe_stack(firmware, sp):
    """Describes which mode stack the stack pointer is on and its usage."""
    end = firmware.symbols.value("pbdrv_stack_end")
    start = firmware.symbols.value("pbdrv_stack_start")
    sizes = firmware.mode_stack_sizes
    if end is None or start is None or not sizes:
        return []

    top = end
    stacks = []
    for mode_name in MODE_STACK_ORDER:
        size = sizes.get(mode_name, 0)
        stacks.append((top - size, top, MODE_STACK_NAMES[mode_name]))
        top -= size
    stacks.append((start, top, MODE_STACK_NAMES["SYS"]))

    lines = []
    for bottom, stack_top, name in stacks:
        if bottom <= sp < stack_top:
            used = stack_top - sp
            free = sp - bottom
            lines.append(
                f"  on the {name} mode stack (0x{bottom:08X}-0x{stack_top:08X})"
            )
            lines.append(f"  {used} bytes used, {free} bytes free")
            if free < 4096:
                lines.append(
                    "  WARNING: stack is nearly exhausted, this may be a stack overflow"
                )
            break
    else:
        lines.append(
            "  WARNING: not on any known stack, the stack pointer is corrupt or overflowed"
        )

    return lines


def analyze(dump, firmware, disassemble=True):
    """Prints the decoded panic dump."""
    out = print
    pc = dump["R15"]
    exception = dump.get("type", "unknown")
    is_data_abort = "Data Abort" in exception
    is_prefetch_abort = "Prefetch Abort" in exception

    out(f"ELF:  {firmware.elf}")
    out(
        f"      built {time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(firmware.elf.stat().st_mtime))}"
    )
    out("      (make sure this is the build that was running on the hub)")
    out("")

    out("=== Crash ===")
    out(f"{exception} at 0x{pc:08X}")
    location = firmware.code_location(pc)
    if location:
        out(f"  in {location}")
    else:
        out(f"  in {firmware.describe(pc)}")
        out("  the program counter is not in any function: a corrupt function")
        out("  pointer or return address was used, check LR (R14) for the caller")

    lr = dump.get("R14")
    if lr:
        out("")
        out(f"Called from LR 0x{lr:08X}")
        caller = firmware.code_location(lr - 4)
        out(f"  {caller if caller else firmware.describe(lr)}")

    out("")
    out("=== Fault status ===")
    if is_data_abort:
        text, hint = decode_fsr(dump.get("DFSR", 0))
        out(f"DFSR {text}")
        far = dump.get("FAR", 0)
        out(f"FAR  0x{far:08X}: {firmware.describe(far)}")
        if far < 0x1000:
            out(f"  NULL pointer dereference (member/index offset 0x{far:x})")
        if hint:
            out(f"  {hint}")
        out("  (the faulting instruction read or wrote this address)")
    elif is_prefetch_abort:
        text, hint = decode_fsr(dump.get("IFSR", 0))
        out(f"IFSR {text}")
        if hint:
            out(f"  {hint}")
        out("  (the CPU could not fetch the instruction at the program counter)")
    else:
        out("Not an abort, DFSR/IFSR/FAR are stale values from an earlier fault.")

    out("")
    out("=== CPU state before the exception ===")
    spsr = dump.get("SPSR", 0)
    out(f"SPSR 0x{spsr:08X}: {decode_psr(spsr)}")

    sp = dump.get("R13")
    if sp is not None:
        out("")
        out("=== Stack ===")
        out(f"SP   0x{sp:08X}")
        for line in describe_stack(firmware, sp):
            out(line)

    out("")
    out("=== Registers ===")
    for i in range(16):
        value = dump.get(f"R{i}")
        if value is None:
            continue
        name = {13: "R13/SP", 14: "R14/LR", 15: "R15/PC"}.get(i, f"R{i}")
        out(f"{name:<7} 0x{value:08X}  {firmware.describe(value)}")

    if disassemble:
        lines = firmware.disassemble(pc)
        if lines:
            out("")
            out("=== Disassembly ===")
            for line in lines:
                out(line)


def main():
    parser = argparse.ArgumentParser(
        description="Decode an EV3 panic dump.",
        epilog="With no file, paste the dump into the terminal when prompted.",
    )
    parser.add_argument(
        "dump",
        nargs="?",
        type=argparse.FileType("r"),
        help="file containing the panic dump (default: paste it or pipe it in)",
    )
    parser.add_argument(
        "--elf",
        type=Path,
        default=DEFAULT_ELF,
        help=f"firmware ELF file (default: {DEFAULT_ELF.relative_to(TOP)})",
    )
    parser.add_argument(
        "--no-disassembly", action="store_true", help="skip the disassembly window"
    )
    args = parser.parse_args()

    if not args.elf.exists():
        sys.exit(f"{args.elf} not found, build the ev3 firmware first")

    if not tool("nm"):
        print(
            "warning: cross toolchain not found, only the fault bits are decoded",
            file=sys.stderr,
        )

    try:
        if args.dump:
            text = args.dump.read()
        elif sys.stdin.isatty():
            text = read_pasted_dump()
        else:
            text = sys.stdin.read()
        dump = parse_dump(text)
    except ValueError as err:
        sys.exit(str(err))
    except KeyboardInterrupt:
        sys.exit(1)

    analyze(dump, Firmware(args.elf), not args.no_disassembly)


if __name__ == "__main__":
    main()
