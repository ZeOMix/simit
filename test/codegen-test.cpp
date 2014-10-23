#include "gtest/gtest.h"

#include <memory>
#include <cmath>

#include "ir.h"
#include "ir_printer.h"
#include "backend.h"
#include "llvm_backend.h"

using namespace std;
using namespace testing;
using namespace simit;
using namespace simit::ir;
using namespace simit::internal;

template <typename T>
std::vector<T> toVectorOf(Expr expr) {
  std::vector<T> vec;
  const Literal *lit = dynamic_cast<const Literal*>(expr.expr());
  assert(lit);

  assert(lit->type.isTensor());
  T *vals = static_cast<T*>(lit->data);
  vec.assign(vals, vals + lit->type.toTensor()->size());
  return vec;
}

TEST(Codegen, add0) {
  Var a("a", Float());
  Var b("b", Float());
  Var c("c", Float());

  Expr axb = Add::make(a,b);
  Stmt body = AssignStmt::make(c, axb);

  Func func = Func("add0", {a,b}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aArg = 2.0;
  Expr bArg = 4.1;
  Expr cRes = Literal::make(Float());

  function->bind("a", &aArg);
  function->bind("b", &bArg);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], 6.1);
}

TEST(Codegen, sin) {
  Var a("a", Float());
  Var c("c", Float());

  Expr sin_a = Call::make(Intrinsics::sin, {a});
  Stmt body = AssignStmt::make(c, sin_a);

  Func func = Func("testsin", {a}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aArg = 2.0;
  Expr cRes = 0.0;

  function->bind("a", &aArg);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], sin(2.0));

}

TEST(Codegen, cos) {
  Var a("a", Float());
  Var c("c", Float());

  Expr cos_a = Call::make(Intrinsics::cos, {a});
  Stmt body = AssignStmt::make(c, cos_a);

  Func func = Func("testcos", {a}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aVar = 2.0;
  Expr cRes = 0.0;

  function->bind("a", &aVar);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], cos(2.0));

}

TEST(Codegen, sqrt) {
  Var a("a", Float());
  Var c("c", Float());

  Expr sqrt_a = Call::make(Intrinsics::sqrt, {a});
  Stmt body = AssignStmt::make(c, sqrt_a);

  Func func = Func("testsqrt", {a}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aVar = 5.0;
  Expr cRes = 0.0;

  function->bind("a", &aVar);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], sqrt(5.0));

}

TEST(Codegen, log) {
  Var a("a", Float());
  Var c("c", Float());

  Expr log_a = Call::make(Intrinsics::log, {a});
  Stmt body = AssignStmt::make(c, log_a);

  Func func = Func("testlog", {a}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aVar = 5.0;
  Expr cRes = 0.0;

  function->bind("a", &aVar);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], log(5.0));

}

TEST(Codegen, exp) {
  Var a("a", Float());
  Var c("c", Float());

  Expr exp_a = Call::make(Intrinsics::exp, {a});
  Stmt body = AssignStmt::make(c, exp_a);

  Func func = Func("testexp", {a}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aVar = 5.0;
  Expr cRes = 0.0;

  function->bind("a", &aVar);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], exp(5.0));

}

TEST(Codegen, atan2) {
  Var a("a", Float());
  Var b("b", Float());
  Var c("c", Float());

  Expr atan2_ab = Call::make(Intrinsics::atan2, {a,b});
  Stmt body = AssignStmt::make(c, atan2_ab);

  Func func = Func("testatan2", {a,b}, {c}, body);

  LLVMBackend backend;
  unique_ptr<Function> function(backend.compile(func));

  Expr aVar = 1.0;
  Expr bVar = 2.0;
  Expr cRes = 0.0;

  function->bind("a", &aVar);
  function->bind("b", &bVar);
  function->bind("c", &cRes);

  function->run();

  vector<double> results = toVectorOf<double>(cRes);
  ASSERT_DOUBLE_EQ(results[0], atan2(1.0,2.0));

}