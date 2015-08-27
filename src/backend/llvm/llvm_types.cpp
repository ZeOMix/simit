#include "llvm_types.h"

#include <vector>
#include "types.h"
#include "llvm/IR/Type.h"

using namespace std;
using namespace simit::ir;

#include "llvm_defines.h"

namespace simit {
namespace backend {

llvm::Type* const LLVM_VOID              = llvm::Type::getVoidTy(LLVM_CTX);

llvm::IntegerType* const LLVM_BOOL       = llvm::Type::getInt1Ty(LLVM_CTX);
llvm::IntegerType* const LLVM_INT        = llvm::Type::getInt32Ty(LLVM_CTX);
llvm::IntegerType* const LLVM_INT8       = llvm::Type::getInt8Ty(LLVM_CTX);
llvm::IntegerType* const LLVM_INT32      = llvm::Type::getInt32Ty(LLVM_CTX);
llvm::IntegerType* const LLVM_INT64      = llvm::Type::getInt64Ty(LLVM_CTX);

llvm::Type* const LLVM_FLOAT             = llvm::Type::getFloatTy(LLVM_CTX);
llvm::Type* const LLVM_DOUBLE            = llvm::Type::getDoubleTy(LLVM_CTX);

llvm::PointerType* const LLVM_FLOAT_PTR  = llvm::Type::getFloatPtrTy(LLVM_CTX);
llvm::PointerType* const LLVM_DOUBLE_PTR = llvm::Type::getDoublePtrTy(LLVM_CTX);

llvm::PointerType* const LLVM_BOOL_PTR   = llvm::Type::getInt1PtrTy(LLVM_CTX);
llvm::PointerType* const LLVM_INT_PTR    = llvm::Type::getInt32PtrTy(LLVM_CTX);
llvm::PointerType* const LLVM_INT8_PTR   = llvm::Type::getInt8PtrTy(LLVM_CTX);
llvm::PointerType* const LLVM_INT32_PTR  = llvm::Type::getInt32PtrTy(LLVM_CTX);
llvm::PointerType* const LLVM_INT64_PTR  = llvm::Type::getInt64PtrTy(LLVM_CTX);

llvm::Type* llvmType(const Type& type, unsigned addrspace) {
  switch (type.kind()) {
    case Type::Tensor:
      return llvmType(*type.toTensor(), addrspace);
    case Type::Element:
      not_supported_yet;
      break;
    case Type::Set:
      return llvmType(*type.toSet(), addrspace);
    case Type::Tuple:
      not_supported_yet;
      break;
  }
  unreachable;
  return nullptr;
}

// TODO: replace anonymous struct with one struct per element and set type
llvm::StructType *llvmType(const ir::SetType& setType, unsigned addrspace) {
  const ElementType *elemType = setType.elementType.toElement();
  vector<llvm::Type*> llvmFieldTypes;

  // Set size
  llvmFieldTypes.push_back(LLVM_INT);

  // Edge indices (if the set is an edge set)
  if (setType.endpointSets.size() > 0) {
    // Endpoints
    llvmFieldTypes.push_back(
        llvm::Type::getInt32PtrTy(LLVM_CTX, addrspace));

    // Neighbor Index
    // row starts (block row)
    llvmFieldTypes.push_back(
        llvm::Type::getInt32PtrTy(LLVM_CTX, addrspace));
    // col indexes (block column)
    llvmFieldTypes.push_back(
        llvm::Type::getInt32PtrTy(LLVM_CTX, addrspace));
  }

  // Fields
  for (const Field &field : elemType->fields) {
    llvmFieldTypes.push_back(llvmType(field.type, addrspace));
  }
  return llvm::StructType::get(LLVM_CTX, llvmFieldTypes);
}

llvm::PointerType* llvmType(const TensorType& type, unsigned addrspace) {
  return llvmPtrType(type.componentType, addrspace);
}

llvm::Type* llvmType(ScalarType stype) {
  switch (stype.kind) {
    case ScalarType::Int:
      return LLVM_INT;
    case ScalarType::Float:
      return llvmFloatType();
    case ScalarType::Boolean:
      return LLVM_BOOL;
  }
  unreachable;
  return nullptr;
}

llvm::Type *llvmFloatType() {
  if (ScalarType::singleFloat()) {
    return LLVM_FLOAT;
  }
  else {
    return LLVM_DOUBLE;
  }
}

llvm::PointerType *llvmFloatPtrType(unsigned addrspace) {
  if (ScalarType::singleFloat()) {
    return llvm::Type::getFloatPtrTy(LLVM_CTX, addrspace);
  }
  else {
    return llvm::Type::getDoublePtrTy(LLVM_CTX, addrspace);
  }
}

}}
