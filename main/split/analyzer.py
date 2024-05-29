#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
from parse import Parse
from z3helper import Z3Helper

z3f = Z3Helper()
def getMatchConditionsForTest(beforeList, afterList):
    keys = set(beforeList.getKeys())
    keys.intersection(afterList.getKeys())

    # Only care intersection for now.
    beforeCc = beforeList.getUnrolledConditionsByKey(k)
    afterCc = afterList.getUnrolledConditionsByKey(k)

    # If one of conditions are empty, no point of doing analysis.
    if len(beforeCc) == 0 or len(afterCc) == 0:
        if len(beforeCc) != len(afterCc):
            # CHECKME : looks like there's a bug.
            pass
        continue

    proofResult = z3f.isEqual(beforeCc, afterCc)
    if proofResult:
        return 1
    else:
        return 0

def TestImp(beforeList, afterList):
    keys = set(beforeList.getKeys())
    keys.intersection(afterList.getKeys())

    # Only care intersection for now.
    beforeCc = beforeList.getUnrolledConditionsByKey(k)
    afterCc = afterList.getUnrolledConditionsByKey(k)

    # If one of conditions are empty, no point of doing analysis.
    if len(beforeCc) == 0 or len(afterCc) == 0:
        if len(beforeCc) != len(afterCc):
            # CHECKME : looks like there's a bug.
            pass
        continue

    proofResult = z3f.isImply(beforeCc, afterCc)
    if proofResult:
        return 1
    else:
        return 0
        


def test_analyzer(a, b):
    beforeList = Parse(a).parse()
    afterList = Parse(b).parse()
    return getMatchConditionsForTest(beforeList, afterList)

def test_analyzer_plus(a, b):
    beforeList = Parse(a).parse()
    afterList = Parse(b).parse()
    return TestImp(beforeList, afterList)

def analyze_kernel(fssOutputDir):
    import ops
    # rcContainer = Parse(fssOutputDir).parse()
    matchGen = ops.matchFunctionGenerator()

if __name__ == "__main__":
    # test_analyzer()
    analyze_kernel("asdf")