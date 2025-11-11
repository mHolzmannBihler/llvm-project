//===- BihlerUnsafeAllocationCheck.cpp - clang-tidy ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "BihlerUnsafeAllocationCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace bihler {

void BihlerUnsafeAllocationCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  // Match 'new' expressions that are NOT inside a try statement
  Finder->addMatcher(
    cxxNewExpr(
      unless(hasAncestor(cxxTryStmt()))
    ).bind("unsafe_new"), this);

  // Match STL container methods that may allocate
  Finder->addMatcher(
    cxxMemberCallExpr(
      unless(hasAncestor(cxxTryStmt())),
      callee(cxxMethodDecl(
        anyOf(
          // std::vector, std::deque methods
          hasName("push_back"),
          hasName("push_front"), 
          hasName("emplace_back"),
          hasName("emplace_front"),
          hasName("insert"),
          hasName("emplace"),
          hasName("emplace_hint"),
          hasName("reserve"),
          hasName("resize"),
          hasName("assign"),
          hasName("shrink_to_fit"),
          
          // std::string methods
          hasName("append"),
          hasName("replace"),
          hasName("operator+="),
          
          // std::map, std::set, std::unordered_map, std::unordered_set
          hasName("rehash"),
          
          // std::list, std::forward_list additional methods
          hasName("splice"),
          hasName("merge"),
          hasName("sort"),
          hasName("unique"),
          hasName("remove"),
          hasName("remove_if")
        )
      ))
    ).bind("unsafe_stl"), this);

  // Match map/unordered_map operator[] which may allocate
  Finder->addMatcher(
    cxxOperatorCallExpr(
      unless(hasAncestor(cxxTryStmt())),
      hasOverloadedOperatorName("[]"),
      callee(cxxMethodDecl(
        ofClass(anyOf(
          hasName("std::map"),
          hasName("std::unordered_map"),
          hasName("std::multimap"),
          hasName("std::unordered_multimap")
        ))
      ))
    ).bind("unsafe_map_subscript"), this);

  // Match smart pointer factory functions
  Finder->addMatcher(
    callExpr(
      unless(hasAncestor(cxxTryStmt())),
      callee(functionDecl(anyOf(
        hasName("make_unique"),
        hasName("make_shared"),
        hasName("allocate_shared")
      )))
    ).bind("unsafe_smart_ptr"), this);
}

void BihlerUnsafeAllocationCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *NewExpr = Result.Nodes.getNodeAs<CXXNewExpr>("unsafe_new")) {
    // Check if this new expression uses nothrow
    if (isNothrowNew(NewExpr)) {
      return; // No warning for nothrow new
    }
    
    diag(NewExpr->getBeginLoc(), 
         "new expression is not protected by try-catch block and may throw std::bad_alloc");
  } else if (const auto *STLCall = Result.Nodes.getNodeAs<CXXMemberCallExpr>("unsafe_stl")) {
    std::string MethodName = "STL container method";
    if (const auto *Method = STLCall->getMethodDecl()) {
      MethodName = Method->getNameAsString();
    }
    diag(STLCall->getBeginLoc(), 
         "STL method '%0' is not protected by try-catch block and may throw std::bad_alloc")
        << MethodName;
  } else if (const auto *MapSubscript = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("unsafe_map_subscript")) {
    diag(MapSubscript->getBeginLoc(),
         "map subscript operator[] is not protected by try-catch block and may throw std::bad_alloc");
  } else if (const auto *SmartPtrCall = Result.Nodes.getNodeAs<CallExpr>("unsafe_smart_ptr")) {
    std::string FuncName = "smart pointer factory";
    if (const auto *Func = SmartPtrCall->getDirectCallee()) {
      FuncName = Func->getNameAsString();
    }
    diag(SmartPtrCall->getBeginLoc(), 
         "smart pointer factory '%0' is not protected by try-catch block and may throw std::bad_alloc")
        << FuncName;
  }
}

bool BihlerUnsafeAllocationCheck::isNothrowNew(const CXXNewExpr *NewExpr) {
  // Check if the new expression has placement arguments
  if (NewExpr->getNumPlacementArgs() == 0) {
    return false; // No placement args, so not nothrow
  }

  // Check each placement argument for std::nothrow
  for (unsigned i = 0; i < NewExpr->getNumPlacementArgs(); ++i) {
    const Expr *PlacementArg = NewExpr->getPlacementArg(i);
    
    // Look for DeclRefExpr that references std::nothrow
    if (const auto *DRE = dyn_cast<DeclRefExpr>(PlacementArg->IgnoreImplicit())) {
      if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
        if (VD->getName() == "nothrow") {
          // Additional check: ensure it's in std namespace
          if (const auto *NS = dyn_cast<NamespaceDecl>(VD->getDeclContext())) {
            if (NS->getName() == "std") {
              return true;
            }
          }
        }
      }
    }
  }
  
  return false;
}

} // namespace bihler
} // namespace tidy
} // namespace clang