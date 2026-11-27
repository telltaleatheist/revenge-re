#!/usr/bin/env python3
"""tools/check_headers.py fails a declaration that isn't beside its definition.

Builds a small tree in a temporary folder: a domain header and a data
header, the files that define what they declare, and a file that includes
them. As built it passes; each case then breaks one thing and expects
check_headers to name it.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
CHECK = os.path.join(os.path.dirname(HERE), "tools", "check_headers.py")

BUILT = {
    "src/Original/Tactical/Tactical.h":
        "#include <stdint.h>\n\nvoid ApplyDamage(int amount);\nuint16_t CountUnits(\n    uint16_t side);\n",
    "src/Original/Tactical/UnitDamage.cpp":
        '#include "Tactical/Tactical.h"\n\n// 2D1B:0010\nvoid ApplyDamage(int amount)\n{\n    (void) amount;\n}\n',
    "src/Original/Tactical/UnitCount.cpp":
        '#include "Tactical/Tactical.h"\n\nuint16_t CountUnits(\n    uint16_t side)\n{\n    return side;\n}\n',
    "src/Original/Data/TacticalData.h":
        "#include <stdint.h>\n\n/* DS:1234 */\nextern uint16_t MapWidth;\nextern uint8_t UnitFlags[0x18];\n",
    "src/Original/Data/TacticalData.cpp":
        '#include "Data/TacticalData.h"\n\nuint16_t MapWidth;\nuint8_t\n    UnitFlags[0x18];\n',
    "src/Original/Data/CampaignData.cpp": "#include <stdint.h>\n\nuint16_t CampaignDay;\n",
    "src/Original/Units/Units.h": "void ResetUnits(void);\n",
    "src/Original/Units/Reset.cpp": '#include "Units/Units.h"\n\nvoid ResetUnits(void)\n{\n}\n',
}


def run(changes):
    with tempfile.TemporaryDirectory() as root:
        files = dict(BUILT)
        files.update(changes)
        for relative, text in files.items():
            if text is None:
                continue
            path = os.path.join(root, relative)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w") as f:
                f.write(text)
        checked = subprocess.run([sys.executable, "-I", CHECK, "--root", root], capture_output=True, text=True)
        return checked.returncode, checked.stdout + checked.stderr


def main():
    failures = 0
    cases = [
        ("as built", {}, 0, "valid: 5 declarations in 3 headers"),
        ("a procedure defined in another domain",
         {"src/Original/Tactical/UnitDamage.cpp": None,
          "src/Original/Units/Damage.cpp": "void ApplyDamage(int amount)\n{\n    (void) amount;\n}\n"},
         1, "the function ApplyDamage isn't defined in src/Original/Tactical/*.cpp"),
        ("a variable defined in another data file",
         {"src/Original/Data/TacticalData.h": "#include <stdint.h>\n\nextern uint16_t MapWidth;\n"
                                              "extern uint8_t UnitFlags[0x18];\nextern uint16_t CampaignDay;\n"},
         1, "the variable CampaignDay isn't defined in src/Original/Data/TacticalData.cpp"),
        ("a declaration in two headers",
         {"src/Original/Units/Units.h": "void ResetUnits(void);\nvoid ApplyDamage(int amount);\n"},
         1, "ApplyDamage is declared in src/Original/Tactical/Tactical.h too"),
        ("a header included by its bare name",
         {"src/Original/Tactical/UnitDamage.cpp":
          '#include "Tactical.h"\n\nvoid ApplyDamage(int amount)\n{\n    (void) amount;\n}\n'},
         1, 'includes "Tactical.h"; spell it "Tactical/Tactical.h"'),
        ("a header that includes an umbrella",
         {"src/Original/Units/Units.h": '#include "revenge/original_data.h"\n\nvoid ResetUnits(void);\n'},
         1, 'src/Original/Units/Units.h: includes "revenge/original_data.h"'),
        ("a header that includes an umbrella by a relative path",
         {"src/Original/Units/Units.h": '#include "../REVENGE.h"\n\nvoid ResetUnits(void);\n'},
         1, 'src/Original/Units/Units.h: includes "../REVENGE.h"'),
    ]
    for what, changes, code, expected in cases:
        returncode, output = run(changes)
        if returncode != code or expected not in output:
            print(f"FAIL: {what}: exit {returncode}, expected {code} and {expected!r}:\n{output}")
            failures += 1
    print("check_headers rules: ok" if not failures else f"check_headers rules: {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
