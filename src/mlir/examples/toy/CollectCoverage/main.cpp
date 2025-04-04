// #include "smith/DiversityCriteria.h"
// #include "smith/ExpSetting.h"
// #include "smith/config.h"
// #include "toy/Dialect.h"
// #include "toy/MLIRGen.h"
// #include "toy/Parser.h"
// #include "toy/Passes.h"

// #include "mlir/Dialect/Affine/Passes.h"
// #include "mlir/IR/AsmState.h"
// #include "mlir/IR/BuiltinOps.h"


#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

// #include "src/Dialect/ONNX/ONNXOps.hpp"

#include <iostream>
#include <ctime>

#include "MutationUtil.h"
#include "Traverse.h"
#include "Debug.h"

using namespace mlir;

namespace cl = llvm::cl;
static cl::opt<std::string> inputFilename(cl::Positional,
                                          cl::desc("<input mlir file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));
static cl::opt<unsigned> depth("depth",
                                       cl::desc("<Depth of program dependency graph>"),
                                       cl::init(1),
                                       cl::value_desc("unsigned integer value"));
static cl::opt<std::string> outputFilename(cl::Positional,
                                          cl::desc("<output mlir file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));

// Usage: /data/src/mlirsmith-dev/build/bin/mutator --operation-prob=0.1 test.mlir 1.mlir
int main(int argc, char **argv) {
  // regist the options
  cl::ParseCommandLineOptions(argc, argv, "mlir mutator\n");
  llvm::InitLLVM(argc, argv);

  // Load the mlir file
  MLIRContext context;
  registerAllDialects(context);
  // context.getOrLoadDialect<ONNXDialect>();
  context.allowUnregisteredDialects();
  OwningOpRef<ModuleOp> module;
  llvm::SourceMgr sourceMgr;

  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr = llvm::MemoryBuffer::getFileOrSTDIN(inputFilename);
  sourceMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());
  module = parseSourceFile<ModuleOp>(sourceMgr, &context);
  if (!module) {
    llvm::errs() << "[main] Error can't load file " << inputFilename << "\n";
    return 3;
  }
  #ifdef DEBUG
  llvm::errs() << "[main] Load file " << inputFilename << " success!\n";
  #endif

  srand(time(0));
  Operation* op = module.get();
  MutationParser mp = MutationParser(depth, outputFilename, op);
  mp.writeCoverage();
  
  return 0;
}
