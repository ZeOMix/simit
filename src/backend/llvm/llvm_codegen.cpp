#include "llvm_codegen.h"

#include <utility>
#include "llvm_types.h"

using namespace std;
using namespace simit::ir;

namespace simit {
namespace backend {

llvm::ConstantInt *llvmInt(long long int val, unsigned bits) {
  return llvm::ConstantInt::get(LLVM_CTX, llvm::APInt(bits, val, true));
}

llvm::ConstantInt *llvmUInt(long long unsigned int val, unsigned bits) {
  return llvm::ConstantInt::get(LLVM_CTX, llvm::APInt(bits, val, false));
}

llvm::Constant *llvmFP(double val, unsigned bits) {
  return llvm::ConstantFP::get(llvmFloatType(), val);
}

llvm::Constant* llvmBool(bool val) {
  int intVal = (val) ? 1 : 0;
  return llvm::ConstantInt::get(LLVM_CTX, llvm::APInt(1, intVal, false));
}

llvm::PointerType *llvmPtrType(ScalarType stype, unsigned addrspace) {
  switch (stype.kind) {
    case ScalarType::Int:
      return llvm::Type::getInt32PtrTy(LLVM_CTX, addrspace);
    case ScalarType::Float:
      return llvmFloatPtrType(addrspace);
    case ScalarType::Boolean:
      return llvm::Type::getInt1PtrTy(LLVM_CTX, addrspace);
  }
  unreachable;
  return nullptr;
}

llvm::Constant *llvmPtr(llvm::Type *type, const void *data) {
  llvm::Constant *c = (sizeof(void*) == 4)
      ? llvm::ConstantInt::get(llvm::Type::getInt32Ty(LLVM_CTX),
                               (int)(intptr_t)data)
      : llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLVM_CTX),
                               (intptr_t)data);
  return llvm::ConstantExpr::getIntToPtr(c, type);
}

llvm::Constant *llvmPtr(const Type &type, const void *data,
                        unsigned addrspace) {
  return llvmPtr(llvmType(type, addrspace), data);
}

llvm::Constant *llvmPtr(const Literal& literal) {
  iassert(literal.type.isTensor());
  return llvmPtr(literal.type, literal.data);
}

llvm::Constant *llvmVal(const Type &type, const void *data) {
  ScalarType componentType = type.toTensor()->componentType;
  switch (componentType.kind) {
    case ScalarType::Int:
      return llvmInt(static_cast<const int*>(data)[0]);
    case ScalarType::Float:
      return llvmFP(static_cast<const double*>(data)[0], componentType.bytes());
    case ScalarType::Boolean:
      return llvmBool(static_cast<const bool*>(data)[0]);
  }
  ierror;
  return nullptr;
}

llvm::Constant *llvmVal(const Literal *literal) {
  return llvmVal(literal->type, literal->data);
}

/// One for endpoints, two for neighbor index
extern const int NUM_EDGE_INDEX_ELEMENTS = 3;

static llvm::Function *createPrototype(const std::string &name,
                                       const vector<string> &argNames,
                                       const vector<llvm::Type*> &argTypes,
                                       llvm::Module *module,
                                       bool externalLinkage,
                                       bool doesNotThrow) {
  llvm::FunctionType *ft = llvm::FunctionType::get(LLVM_VOID, argTypes, false);
  auto linkage = externalLinkage ? llvm::Function::ExternalLinkage
                                 : llvm::Function::InternalLinkage;
  llvm::Function *f= llvm::Function::Create(ft, linkage, name, module);
  if (doesNotThrow) {
    f->setDoesNotThrow();
  }
  unsigned i = 0;
  for (llvm::Argument &arg : f->getArgumentList()) {
    arg.setName(argNames[i]);

    // TODO(gkanwar): Move noalias code here from GPU implementation
    if (arg.getType()->isPointerTy()) {
      f->setDoesNotCapture(i+1);  //  setDoesNotCapture(0) is the return value
    }
    ++i;
  }

  return f;
}

llvm::Function *createPrototype(const std::string &name,
                                const vector<Var> &arguments,
                                const vector<Var> &results,
                                llvm::Module *module,
                                bool externalLinkage,
                                bool doesNotThrow,
                                bool scalarsByValue,
                                unsigned addrspace) {
  vector<string>      llvmArgNames;
  vector<llvm::Type*> llvmArgTypes;

  // We don't need two llvm arguments for aliased simit argument/results
  std::set<std::string> argNames;
  
  for (auto &arg : arguments) {
    argNames.insert(arg.getName());
    llvmArgNames.push_back(arg.getName());

    // Our convention is that scalars are passed to functions by value,
    // while everything else is passed through a pointer
    llvm::Type *type = (isScalar(arg.getType()) && scalarsByValue)
        ? llvmType(arg.getType().toTensor()->componentType)
        : llvmType(arg.getType(), addrspace);
    llvmArgTypes.push_back(type);
  }

  for (auto &res : results) {
    if (argNames.find(res.getName()) != argNames.end()) {
      continue;
    }
    llvmArgNames.push_back(res.getName());
    llvmArgTypes.push_back(llvmType(res.getType(), addrspace));
  }

  assert(llvmArgNames.size() == llvmArgTypes.size());

  return createPrototype(name, llvmArgNames, llvmArgTypes,
                         module, externalLinkage, doesNotThrow);
}

}}
