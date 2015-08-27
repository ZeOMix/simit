#ifndef SIMIT_LLVM_CODEGEN_H
#define SIMIT_LLVM_CODEGEN_H

#include <string>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"

#include "ir.h"
#include "llvm_defines.h"

namespace simit {
namespace backend {

llvm::ConstantInt* llvmInt(long long int val, unsigned bits=32);
llvm::ConstantInt* llvmUInt(long long unsigned int val, unsigned bits=32);
llvm::Constant*    llvmFP(double val, unsigned bits=64);
llvm::Constant*    llvmBool(bool val);

// Simit-specific utilities

/// The number of index struct elements that are compiled into an edge struct.
extern const int NUM_EDGE_INDEX_ELEMENTS;

llvm::Constant *llvmPtr(llvm::Type *type, const void *data);
llvm::Constant *llvmPtr(const ir::Type &type, const void *data,
                        unsigned addrspace=0);
llvm::Constant *llvmPtr(const ir::Literal& literal);

llvm::Constant *llvmVal(const ir::Type &type, const void *data);
llvm::Constant *llvmVal(const ir::Literal *literal);

/// Creates an llvm function prototype
llvm::Function *createPrototype(const std::string &name,
                                const std::vector<ir::Var> &arguments,
                                const std::vector<ir::Var> &results,
                                llvm::Module *module,
                                bool externalLinkage,
                                bool doesNotThrow=true,
                                bool scalarsByValue=true,
                                unsigned addrspace=0);

}}
#endif
