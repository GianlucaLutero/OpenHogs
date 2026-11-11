# export_decompiled.py - Ghidra headless script (Jython).
#
# Decompiles every function of the analysed program into one C-like text
# file, with the strings each function refers to listed above it, so the
# game's logic can be searched with ordinary tools (grep). Used to study
# warhogs_.exe for OpenHogs; see docs/formats/executable.md.
#
# The output is derived from the game's executable: keep it on your own
# computer, never commit or publish it.
#
# Usage (see tools/ghidra/README.md):
#   analyzeHeadless <project-dir> hogs -process warhogs_.exe -noanalysis \
#       -scriptPath tools/ghidra -postScript export_decompiled.py <output.c>
#
# @category OpenHogs

from ghidra.app.decompiler import DecompInterface, DecompileOptions
from ghidra.util.task import ConsoleTaskMonitor

args = getScriptArgs()
output_path = args[0] if len(args) > 0 else "decompiled.c"

program = currentProgram
listing = program.getListing()
reference_manager = program.getReferenceManager()

decompiler = DecompInterface()
decompiler.setOptions(DecompileOptions())
decompiler.openProgram(program)
monitor = ConsoleTaskMonitor()


def strings_used_by(function):
    """The string literals a function's instructions refer to."""
    found = []
    for instruction in listing.getInstructions(function.getBody(), True):
        for reference in instruction.getReferencesFrom():
            data = listing.getDataAt(reference.getToAddress())
            if data is not None and data.hasStringValue():
                found.append(str(data.getValue()))
    return found


out = open(output_path, "w")
count = 0
for function in program.getFunctionManager().getFunctions(True):
    result = decompiler.decompileFunction(function, 60, monitor)
    out.write("/* ==== %s @ %s ==== */\n" % (function.getName(), function.getEntryPoint()))
    for text in strings_used_by(function):
        out.write("/* string: %s */\n" % text.replace("*/", "* /").encode("ascii", "replace"))
    if result is not None and result.decompileCompleted():
        out.write(result.getDecompiledFunction().getC().encode("ascii", "replace"))
    else:
        out.write("/* decompilation failed */\n")
    out.write("\n")
    count += 1
out.close()
print("Decompiled %d functions to %s" % (count, output_path))
