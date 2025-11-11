set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyBihlerModule STATIC
  BihlerUnsafeAllocationCheck.cpp
  BihlerTidyModule.cpp

  LINK_LIBS
  clangTidy

  DEPENDS
  omp_gen
  ClangDriverOptions
  )

clang_target_link_libraries(clangTidyBihlerModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  )
