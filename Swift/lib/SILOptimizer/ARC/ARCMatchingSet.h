//===--- ARCMatchingSet.h ---------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_SILOPTIMIZER_PASSMANAGER_GLOBALARCPAIRINGANALYSIS_H
#define SWIFT_SILOPTIMIZER_PASSMANAGER_GLOBALARCPAIRINGANALYSIS_H

#include "GlobalARCSequenceDataflow.h"
#include "GlobalLoopARCSequenceDataflow.h"
#include "swift/SIL/SILValue.h"
#include "swift/SILOptimizer/Utils/LoopUtils.h"
#include "swift/SILOptimizer/Analysis/RCIdentityAnalysis.h"
#include "llvm/ADT/SetVector.h"

namespace swift {

class SILInstruction;
class SILFunction;
class AliasAnalysis;
class PostOrderAnalysis;
class LoopRegionFunctionInfo;
class SILLoopInfo;
class RCIdentityFunctionInfo;

/// A set of matching reference count increments, decrements, increment
/// insertion pts, and decrement insertion pts.
struct ARCMatchingSet {

  /// The pointer that this ARCMatchingSet is providing matching increment and
  /// decrement sets for.
  ///
  /// TODO: This should really be called RCIdentity.
  SILValue Ptr;

  /// The set of reference count increments that were paired.
  llvm::SetVector<SILInstruction *> Increments;

  /// An insertion point for an increment means the earliest point in the
  /// program after the increment has occurred that the increment can be moved
  /// to
  /// without moving the increment over an instruction that may decrement a
  /// reference count.
  llvm::SetVector<SILInstruction *> IncrementInsertPts;

  /// The set of reference count decrements that were paired.
  llvm::SetVector<SILInstruction *> Decrements;

  /// An insertion point for a decrement means the latest point in the program
  /// before the decrement that the optimizer conservatively assumes that a
  /// reference counted value could be used.
  llvm::SetVector<SILInstruction *> DecrementInsertPts;

  // This is a data structure that cannot be moved or copied.
  ARCMatchingSet() = default;
  ARCMatchingSet(const ARCMatchingSet &) = delete;
  ARCMatchingSet(ARCMatchingSet &&) = delete;
  ARCMatchingSet &operator=(const ARCMatchingSet &) = delete;
  ARCMatchingSet &operator=(ARCMatchingSet &&) = delete;

  void clear() {
    Ptr = SILValue();
    Increments.clear();
    IncrementInsertPts.clear();
    Decrements.clear();
    DecrementInsertPts.clear();
  }
};

struct MatchingSetFlags {
  bool KnownSafe;
  bool Partial;
};
static_assert(std::is_pod<MatchingSetFlags>::value,
              "MatchingSetFlags should be a pod.");

struct ARCMatchingSetBuilder {
  using TDMapTy = BlotMapVector<SILInstruction *, TopDownRefCountState>;
  using BUMapTy = BlotMapVector<SILInstruction *, BottomUpRefCountState>;

  TDMapTy &TDMap;
  BUMapTy &BUMap;

  llvm::SmallVector<SILInstruction *, 8> NewIncrements;
  llvm::SmallVector<SILInstruction *, 8> NewDecrements;
  bool MatchedPair;
  ARCMatchingSet MatchSet;
  bool PtrIsGuaranteedArg;

  RCIdentityFunctionInfo *RCIA;

public:
  ARCMatchingSetBuilder(TDMapTy &TDMap, BUMapTy &BUMap,
                        RCIdentityFunctionInfo *RCIA)
      : TDMap(TDMap), BUMap(BUMap), MatchedPair(false),
        PtrIsGuaranteedArg(false), RCIA(RCIA) {}

  void init(SILInstruction *Inst) {
    clear();
    MatchSet.Ptr = RCIA->getRCIdentityRoot(Inst->getOperand(0));

    // If we have a function argument that is guaranteed, set the guaranteed
    // flag so we know that it is always known safe.
    if (auto *A = dyn_cast<SILArgument>(MatchSet.Ptr)) {
      if (A->isFunctionArg()) {
        auto C = A->getArgumentConvention();
        PtrIsGuaranteedArg = C == SILArgumentConvention::Direct_Guaranteed;
      }
    }
    NewIncrements.push_back(Inst);
  }

  void clear() {
    MatchSet.clear();
    MatchedPair = false;
    NewIncrements.clear();
    NewDecrements.clear();
  }

  bool matchUpIncDecSetsForPtr();

  // We only allow for get result when this object is invalidated via a move.
  ARCMatchingSet &getResult() { return MatchSet; }

  bool matchedPair() const { return MatchedPair; }

private:
  /// Returns .Some(MatchingSetFlags) on success and .None on failure.
  Optional<MatchingSetFlags> matchIncrementsToDecrements();
  Optional<MatchingSetFlags> matchDecrementsToIncrements();
};

} // end swift namespace

#endif
