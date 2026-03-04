//===- BihlerTidyModule.cpp - clang-tidy ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "BihlerUnsafeAllocationCheck.h"

namespace clang {
namespace tidy {
namespace bihler {

class BihlerModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<BihlerUnsafeAllocationCheck>(
        "bihler-unsafe-allocation");
  }
};

// Register the BihlerModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<BihlerModule>
    X("bihler-module", "Adds Bihler custom lint checks.");

} // namespace bihler
// This anchor is used to force the linker to link in the generated object file
// and thus register the BihlerModule.
volatile int BihlerModuleAnchorSource = 0; // NOLINT(misc-use-internal-linkage)
} // namespace tidy
} // namespace clang
