//===-- MSP430Subtarget.h - Define Subtarget for the MSP430 ----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the MSP430 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MSP430_MSP430SUBTARGET_H
#define LLVM_LIB_TARGET_MSP430_MSP430SUBTARGET_H

#include "MSP430FrameLowering.h"
#include "MSP430InstrInfo.h"
#include "MSP430ISelLowering.h"
#include "MSP430RegisterInfo.h"
#include "MSP430SelectionDAGInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "MSP430GenSubtargetInfo.inc"

namespace llvm {
class StringRef;

class MSP430Subtarget : public MSP430GenSubtargetInfo {
  virtual void anchor();
  bool ExtendedInsts;
  const DataLayout DL; // Calculates type size & alignment
  MSP430FrameLowering FrameLowering;
  MSP430InstrInfo InstrInfo;
  MSP430TargetLowering TLInfo;
  MSP430SelectionDAGInfo TSInfo;

public:
  /// This constructor initializes the data members to match that
  /// of the specified triple.
  ///
  MSP430Subtarget(const std::string &TT, const std::string &CPU,
                  const std::string &FS, const TargetMachine &TM);

  MSP430Subtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef FS);

  const TargetFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const MSP430InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const DataLayout *getDataLayout() const override { return &DL; }
  const TargetRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const MSP430TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const MSP430SelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
};
} // End llvm namespace

#endif  // LLVM_TARGET_MSP430_SUBTARGET_H
