//===- BihlerUnsafeAllocationCheck.h - clang-tidy ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BIHLER_UNSAFEALLOCATIONCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BIHLER_UNSAFEALLOCATIONCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
class CXXMethodDecl;
class CXXMemberCallExpr;
class CXXNewExpr;

namespace tidy {
namespace bihler {

/// Detects dynamic memory allocations that are not protected by try-catch blocks.
class BihlerUnsafeAllocationCheck : public ClangTidyCheck {
public:
  BihlerUnsafeAllocationCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.CPlusPlus;
  }

private:
  bool isNothrowNew(const CXXNewExpr *NewExpr);
  bool isBihlListMethod(const CXXMethodDecl *Method);
  bool isResultCheckedForNullptr(const CXXMemberCallExpr *CallExpr,
                                  const ast_matchers::MatchFinder::MatchResult &Result);
};

} // namespace bihler
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BIHLER_UNSAFEALLOCATIONCHECK_H