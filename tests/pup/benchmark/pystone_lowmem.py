"""
Hardware Module: All hubs.

Description: MicroPython performance benchmark for LEGO Powered Up hubs.

Based on: https://github.com/micropython/micropython-lib/blob/master/python-stdlib/pystone_lowmem/pystone_lowmem.py

Author history:

Reinhold P. Weicker,  CACM Vol 27, No 10, 10/84 pg. 1013.
Translated from ADA to C by Rick Richardson.
Translated from C to Python by Guido van Rossum.
Adapted by the micropython-lib contributors for MicroPython compatibility.
Adapted by the Pybricks Authors for LEGO Powered Up compatibility:
    - Cross platform clock definition for comparison with other firmwares.
    - Dropped command-line options, with LOOPS just hardcoded.

"""

LOOPS = 500

try:
    from pybricks.tools import StopWatch

    ticks_ms = StopWatch().time
except ImportError:
    from utime import ticks_ms


__version__ = "1.2"

[Ident1, Ident2, Ident3, Ident4, Ident5] = range(1, 6)


class Record:
    def __init__(self, PtrComp=None, Discr=0, EnumComp=0, IntComp=0, StringComp=0):
        self.PtrComp = PtrComp
        self.Discr = Discr
        self.EnumComp = EnumComp
        self.IntComp = IntComp
        self.StringComp = StringComp

    def copy(self):
        return Record(
            self.PtrComp, self.Discr, self.EnumComp, self.IntComp, self.StringComp
        )


TRUE = 1
FALSE = 0


def main(loops=LOOPS):
    benchtime, stones = pystones(loops)
    print("Pystone(%s) time for %d passes = %dms" % (__version__, loops, benchtime))
    print("This machine benchmarks at %d pystones/second" % stones)


def pystones(loops=LOOPS):
    return Proc0(loops)


IntGlob = 0
BoolGlob = FALSE
Char1Glob = "\0"
Char2Glob = "\0"
Array1Glob = [0] * (51 // 2)
Array2Glob = [x for x in [Array1Glob] * (51 // 2)]
PtrGlb = None
PtrGlbNext = None


def Proc0(loops=LOOPS):
    global IntGlob
    global BoolGlob
    global Char1Glob
    global Char2Glob
    global Array1Glob
    global Array2Glob
    global PtrGlb
    global PtrGlbNext

    starttime = ticks_ms()
    for i in range(loops):
        pass
    nulltime = ticks_ms() - starttime

    PtrGlbNext = Record()
    PtrGlb = Record()
    PtrGlb.PtrComp = PtrGlbNext
    PtrGlb.Discr = Ident1
    PtrGlb.EnumComp = Ident3
    PtrGlb.IntComp = 40
    PtrGlb.StringComp = "DHRYSTONE PROGRAM, SOME STRING"
    String1Loc = "DHRYSTONE PROGRAM, 1'ST STRING"
    Array2Glob[8 // 2][7 // 2] = 10

    starttime = ticks_ms()

    for i in range(loops):
        Proc5()
        Proc4()
        IntLoc1 = 2
        IntLoc2 = 3
        String2Loc = "DHRYSTONE PROGRAM, 2'ND STRING"
        EnumLoc = Ident2
        BoolGlob = not Func2(String1Loc, String2Loc)
        while IntLoc1 < IntLoc2:
            IntLoc3 = 5 * IntLoc1 - IntLoc2
            IntLoc3 = Proc7(IntLoc1, IntLoc2)
            IntLoc1 = IntLoc1 + 1
        Proc8(Array1Glob, Array2Glob, IntLoc1, IntLoc3)
        PtrGlb = Proc1(PtrGlb)
        CharIndex = "A"
        while CharIndex <= Char2Glob:
            if EnumLoc == Func1(CharIndex, "C"):
                EnumLoc = Proc6(Ident1)
            CharIndex = chr(ord(CharIndex) + 1)
        IntLoc3 = IntLoc2 * IntLoc1
        IntLoc2 = IntLoc3 // IntLoc1
        IntLoc2 = 7 * (IntLoc3 - IntLoc2) - IntLoc1
        IntLoc1 = Proc2(IntLoc1)

    benchtime = ticks_ms() - starttime - nulltime
    if benchtime == 0:
        loopsPerBenchtime = 0
    else:
        loopsPerBenchtime = loops * 1000 // benchtime
    return benchtime, loopsPerBenchtime


def Proc1(PtrParIn):
    PtrParIn.PtrComp = NextRecord = PtrGlb.copy()
    PtrParIn.IntComp = 5
    NextRecord.IntComp = PtrParIn.IntComp
    NextRecord.PtrComp = PtrParIn.PtrComp
    NextRecord.PtrComp = Proc3(NextRecord.PtrComp)
    if NextRecord.Discr == Ident1:
        NextRecord.IntComp = 6
        NextRecord.EnumComp = Proc6(PtrParIn.EnumComp)
        NextRecord.PtrComp = PtrGlb.PtrComp
        NextRecord.IntComp = Proc7(NextRecord.IntComp, 10)
    else:
        PtrParIn = NextRecord.copy()
    NextRecord.PtrComp = None
    return PtrParIn


def Proc2(IntParIO):
    IntLoc = IntParIO + 10
    while 1:
        if Char1Glob == "A":
            IntLoc = IntLoc - 1
            IntParIO = IntLoc - IntGlob
            EnumLoc = Ident1
        if EnumLoc == Ident1:
            break
    return IntParIO


def Proc3(PtrParOut):
    global IntGlob

    if PtrGlb is not None:
        PtrParOut = PtrGlb.PtrComp
    else:
        IntGlob = 100
    PtrGlb.IntComp = Proc7(10, IntGlob)
    return PtrParOut


def Proc4():
    global Char2Glob

    BoolLoc = Char1Glob == "A"
    BoolLoc = BoolLoc or BoolGlob
    Char2Glob = "B"


def Proc5():
    global Char1Glob
    global BoolGlob

    Char1Glob = "A"
    BoolGlob = FALSE


def Proc6(EnumParIn):
    EnumParOut = EnumParIn
    if not Func3(EnumParIn):
        EnumParOut = Ident4
    if EnumParIn == Ident1:
        EnumParOut = Ident1
    elif EnumParIn == Ident2:
        if IntGlob > 100:
            EnumParOut = Ident1
        else:
            EnumParOut = Ident4
    elif EnumParIn == Ident3:
        EnumParOut = Ident2
    elif EnumParIn == Ident4:
        pass
    elif EnumParIn == Ident5:
        EnumParOut = Ident3
    return EnumParOut


def Proc7(IntParI1, IntParI2):
    IntLoc = IntParI1 + 2
    IntParOut = IntParI2 + IntLoc
    return IntParOut


def Proc8(Array1Par, Array2Par, IntParI1, IntParI2):
    global IntGlob

    IntLoc = IntParI1 + 5
    Array1Par[IntLoc // 2] = IntParI2
    Array1Par[(IntLoc + 1) // 2] = Array1Par[IntLoc // 2]
    Array1Par[(IntLoc + 30) // 2] = IntLoc
    for IntIndex in range(IntLoc, IntLoc + 2):
        Array2Par[IntLoc // 2][IntIndex // 2] = IntLoc
    Array2Par[IntLoc // 2][(IntLoc - 1) // 2] = (
        Array2Par[IntLoc // 2][(IntLoc - 1) // 2] + 1
    )
    Array2Par[(IntLoc + 20) // 2][IntLoc // 2] = Array1Par[IntLoc // 2]
    IntGlob = 5


def Func1(CharPar1, CharPar2):
    CharLoc1 = CharPar1
    CharLoc2 = CharLoc1
    if CharLoc2 != CharPar2:
        return Ident1
    else:
        return Ident2


def Func2(StrParI1, StrParI2):
    IntLoc = 1
    while IntLoc <= 1:
        if Func1(StrParI1[IntLoc], StrParI2[IntLoc + 1]) == Ident1:
            CharLoc = "A"
            IntLoc = IntLoc + 1
    if CharLoc >= "W" and CharLoc <= "Z":
        IntLoc = 7
    if CharLoc == "X":
        return TRUE
    else:
        if StrParI1 > StrParI2:
            IntLoc = IntLoc + 7
            return TRUE
        else:
            return FALSE


def Func3(EnumParIn):
    EnumLoc = EnumParIn
    if EnumLoc == Ident3:
        return TRUE
    return FALSE


main(LOOPS)
