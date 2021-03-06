set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyReadabilityModule
  BracesAroundStatementsCheck.cpp
  ContainerSizeEmptyCheck.cpp
  ElseAfterReturnCheck.cpp
  FunctionSizeCheck.cpp
  IdentifierNamingCheck.cpp
  ImplicitBoolCastCheck.cpp
  InconsistentDeclarationParameterNameCheck.cpp
  NamedParameterCheck.cpp
  NamespaceCommentCheck.cpp
  ReadabilityTidyModule.cpp
  RedundantStringCStrCheck.cpp
  RedundantSmartptrGetCheck.cpp
  SimplifyBooleanExprCheck.cpp
  UniqueptrDeleteReleaseCheck.cpp

  LINK_LIBS
  clangAPINotes
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTooling
  )
