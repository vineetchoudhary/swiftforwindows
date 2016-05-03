// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/err_type_complete.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/for_incomplete1.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_complete.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete1.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete2.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete3.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete4.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete5.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete6.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete7.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete8.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete9.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete10.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete11.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/func_incomplete12.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/nominal_complete.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/nominal_incomplete.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/nominal_incomplete2.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/nominal_incomplete3.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/switch_incomplete.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/switch_incomplete2.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/switch_incomplete3.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/toplevel_complete.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/toplevel_incomplete.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/toplevel_incomplete2.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/toplevel_incomplete3.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/toplevel_incomplete4.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/toplevel_incomplete5.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/type_incomplete1.swift | FileCheck %s -check-prefix=COMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/type_incomplete2.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/type_incomplete3.swift | FileCheck %s -check-prefix=INCOMPLETE
// RUN: %swift-ide-test -test-input-complete -source-filename %S/Inputs/type_incomplete4.swift | FileCheck %s -check-prefix=INCOMPLETE

// INCOMPLETE: IS_INCOMPLETE
// COMPLETE: IS_COMPLETE