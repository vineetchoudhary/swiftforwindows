//===--- RuntimeShims.h - Access to runtime facilities for the core -------===//
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
//
//  Runtime functions and structures needed by the core stdlib are
//  declared here.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_STDLIB_SHIMS_RUNTIMESHIMS_H
#define SWIFT_STDLIB_SHIMS_RUNTIMESHIMS_H

#include "SwiftStddef.h"
#include "SwiftStdint.h"
#include "Visibility.h"

#ifdef __cplusplus
namespace swift {

template <unsigned PointerSize> struct RuntimeTarget;
using HostTarget = RuntimeTarget<sizeof(uintptr_t)>;

template <typename Target> struct TargetMetadata;
using Metadata = TargetMetadata<InProcess>;
  
extern "C" {
#else
typedef struct Metadata Metadata;
#define bool _Bool
#endif

SWIFT_RUNTIME_EXPORT
bool swift_objc_class_usesNativeSwiftReferenceCounting(const void *);

/// Return an NSString to be used as the Mirror summary of the object
SWIFT_RUNTIME_STDLIB_INTERFACE
void *_swift_objCMirrorSummary(const void * nsObject);

/// Call strtold_l with the C locale, swapping argument and return
/// types so we can operate on Float80.  Return NULL on overflow.
SWIFT_RUNTIME_STDLIB_INTERFACE
const char *_swift_stdlib_strtold_clocale(const char *nptr, void *outResult);
/// Call strtod_l with the C locale, swapping argument and return
/// types so we can operate consistently on Float80.  Return NULL on
/// overflow.
SWIFT_RUNTIME_STDLIB_INTERFACE
const char *_swift_stdlib_strtod_clocale(const char *nptr, double *outResult);
/// Call strtof_l with the C locale, swapping argument and return
/// types so we can operate consistently on Float80.  Return NULL on
/// overflow.
SWIFT_RUNTIME_STDLIB_INTERFACE
const char *_swift_stdlib_strtof_clocale(const char *nptr, float *outResult);
  
/// Return the superclass, if any.  The result is nullptr for root
/// classes and class protocol types.
SWIFT_RUNTIME_EXPORT
const Metadata *swift_class_getSuperclass(const Metadata *);
  
SWIFT_RUNTIME_STDLIB_INTERFACE
void _swift_stdlib_flockfile_stdout(void);
SWIFT_RUNTIME_STDLIB_INTERFACE
void _swift_stdlib_funlockfile_stdout(void);

SWIFT_RUNTIME_STDLIB_INTERFACE
int _swift_stdlib_putc_stderr(int C);

SWIFT_RUNTIME_STDLIB_INTERFACE
__swift_size_t _swift_stdlib_getHardwareConcurrency();

#ifdef __cplusplus
}} // extern "C", namespace swift
#endif

#endif // SWIFT_STDLIB_SHIMS_RUNTIMESHIMS_H

