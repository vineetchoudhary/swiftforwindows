//===--- BCReadingExtras.h - Useful helpers for bitcode reading -*- C++ -*-===//
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

#ifndef SWIFT_SERIALIZATION_BCREADINGEXTRAS_H
#define SWIFT_SERIALIZATION_BCREADINGEXTRAS_H

#include "llvm/Bitcode/BitstreamReader.h"

namespace swift {
namespace serialization {

/// Saves and restores a BitstreamCursor's bit offset in its stream.
class BCOffsetRAII {
  llvm::BitstreamCursor *Cursor;
  decltype(Cursor->GetCurrentBitNo()) Offset;

public:
  explicit BCOffsetRAII(llvm::BitstreamCursor &cursor)
    : Cursor(&cursor), Offset(cursor.GetCurrentBitNo()) {}

  void reset() {
    if (Cursor)
      Offset = Cursor->GetCurrentBitNo();
  }

  void cancel() {
    Cursor = nullptr;
  }

  ~BCOffsetRAII() {
    if (Cursor)
      Cursor->JumpToBit(Offset);
  }
};

} // end namespace serialization
} // end namespace swift

static constexpr const auto AF_DontPopBlockAtEnd =
  llvm::BitstreamCursor::AF_DontPopBlockAtEnd;

#endif
