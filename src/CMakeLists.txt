set(LLVM_LINK_COMPONENTS
  Analysis
  Core
  ExecutionEngine
  Interpreter
  InstCombine
  MC
  ScalarOpts
  Support
  TransformUtils
  nativecodegen
  )

set(LLVM_REQUIRES_RTTI 1)

add_llvm_example(Mila
  mila.cpp
  )
