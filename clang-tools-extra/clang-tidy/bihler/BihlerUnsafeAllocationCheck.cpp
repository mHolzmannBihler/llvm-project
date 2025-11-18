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
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
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

  // Match BihlList::List::emplace_back specifically
  Finder->addMatcher(
    cxxMemberCallExpr(
      callee(cxxMethodDecl(
        hasName("emplace_back"),
        ofClass(hasName("BihlList::List"))
      ))
    ).bind("bihllist_emplace"), this);

  // Match BihlOptional::emplace and ::create specifically
  Finder->addMatcher(
    cxxMemberCallExpr(
      callee(cxxMethodDecl(
        anyOf(hasName("emplace"), hasName("create")),
        hasAncestor(namespaceDecl(hasName("BihlOptional")))
      ))
    ).bind("bihloptional_call"), this);

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
  } else if (const auto *BihlListCall = Result.Nodes.getNodeAs<CXXMemberCallExpr>("bihllist_emplace")) {
    // BihlList::List::emplace_back uses std::nothrow internally
    // Check if the return value is checked for nullptr
    if (!isResultCheckedForNullptr(BihlListCall, Result)) {
      diag(BihlListCall->getBeginLoc(),
           "BihlList::List::emplace_back return value must be checked for nullptr");
    }
  } else if (const auto *BihlOptionalCall = Result.Nodes.getNodeAs<CXXMemberCallExpr>("bihloptional_call")) {
    // BihlOptional::emplace and ::create use std::nothrow internally
    // Check if the return value is checked for nullptr
    if (!isResultCheckedForNullptr(BihlOptionalCall, Result)) {
      std::string MethodName = "unknown";
      if (const auto *Method = BihlOptionalCall->getMethodDecl()) {
        MethodName = Method->getNameAsString();
      }
      diag(BihlOptionalCall->getBeginLoc(),
           "BihlOptional::" + MethodName + " return value must be checked for nullptr");
    }
  } else if (const auto *STLCall = Result.Nodes.getNodeAs<CXXMemberCallExpr>("unsafe_stl")) {
    // Skip BihlList methods - they use nothrow internally
    if (const auto *Method = STLCall->getMethodDecl()) {
      if (isBihlListMethod(Method)) {
        return;
      }
      // Skip BihlOptional methods - they use nothrow internally
      if (isBihlOptionalMethod(Method)) {
        return;
      }
      std::string MethodName = Method->getNameAsString();
      diag(STLCall->getBeginLoc(), 
           "STL method '%0' is not protected by try-catch block and may throw std::bad_alloc")
          << MethodName;
    }
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

bool BihlerUnsafeAllocationCheck::isBihlListMethod(const CXXMethodDecl *Method) {
  if (!Method)
    return false;

  const auto *ParentClass = Method->getParent();
  if (!ParentClass)
    return false;

  // Check if the class is BihlList::List
  std::string ClassName = ParentClass->getQualifiedNameAsString();
  return ClassName.find("BihlList::List") != std::string::npos;
}

bool BihlerUnsafeAllocationCheck::isBihlOptionalMethod(const CXXMethodDecl *Method) {
  if (!Method)
    return false;

  const auto *ParentClass = Method->getParent();
  if (!ParentClass)
    return false;

  // Check if the class is BihlOptional
  std::string ClassName = ParentClass->getQualifiedNameAsString();
  if (ClassName.find("BihlOptional") == std::string::npos)
    return false;

  // Check if method is emplace or create
  StringRef MethodName = Method->getName();
  return MethodName == "emplace" || MethodName == "create";
}

bool BihlerUnsafeAllocationCheck::isResultCheckedForNullptr(
    const CXXMemberCallExpr *CallExpr,
    const MatchFinder::MatchResult &Result) {
  
  ASTContext *Context = Result.Context;
  
  // Helper to recursively check parent nodes
  std::function<bool(const Stmt *)> checkParentForNullptrCheck;
  checkParentForNullptrCheck = [&](const Stmt *S) -> bool {
    auto Parents = Context->getParents(*S);
    for (const auto &Parent : Parents) {
      // Direct parent is VarDecl - the result is assigned
      if (Parent.get<clang::VarDecl>()) {
        // If assigned to variable, assume it will be checked
        return true;
      }
      
      // Parent is IfStmt condition
      if (Parent.get<clang::IfStmt>()) {
        return true;
      }
      
      // Parent is binary operator with nullptr comparison
      if (const auto *BinOp = Parent.get<BinaryOperator>()) {
        if (BinOp->getOpcode() == BO_NE || BinOp->getOpcode() == BO_EQ) {
          const Expr *LHS = BinOp->getLHS()->IgnoreImpCasts();
          const Expr *RHS = BinOp->getRHS()->IgnoreImpCasts();
          if (isa<CXXNullPtrLiteralExpr>(LHS) || isa<CXXNullPtrLiteralExpr>(RHS)) {
            return true;
          }
        }
        // Continue checking parent of BinOp
        if (checkParentForNullptrCheck(BinOp))
          return true;
      }
      
      // Parent is unary NOT operator - continue checking
      if (const auto *UnaryOp = Parent.get<UnaryOperator>()) {
        if (UnaryOp->getOpcode() == UO_LNot) {
          // Recursively check parent of UnaryOp
          if (checkParentForNullptrCheck(UnaryOp))
            return true;
        }
      }
      
      // Parent is ImplicitCastExpr - check its parent recursively
      if (const auto *Cast = Parent.get<ImplicitCastExpr>()) {
        if (checkParentForNullptrCheck(Cast))
          return true;
      }
      
      // Parent is MaterializeTemporaryExpr - check its parent recursively
      if (const auto *Materialize = Parent.get<MaterializeTemporaryExpr>()) {
        if (checkParentForNullptrCheck(Materialize))
          return true;
      }
      
      // Parent is ExprWithCleanups - check its parent recursively
      if (const auto *Cleanup = Parent.get<ExprWithCleanups>()) {
        if (checkParentForNullptrCheck(Cleanup))
          return true;
      }
    }
    return false;
  };
  
  return checkParentForNullptrCheck(CallExpr);
}

} // namespace bihler
} // namespace tidy
} // namespace clang