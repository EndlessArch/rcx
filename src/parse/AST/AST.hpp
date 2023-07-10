#ifndef RCX_1_AST_AST_HPP
#define RCX_1_AST_AST_HPP

#include <conv/Modernizer.hpp>

#include <map>
#include <vector>

#include <llvm/IR/IRBuilder.h>

NSRCXBGN

namespace ast {

using codegen_t = void*;

template <typename T>
class AST {
public:
  codegen_t&& generateCode() noexcept;
};

class Type : AST<Type> {
  std::string name_;
public:

  Type(const std::string& name) : name_(name) {}

  // class type constructor
  // Type(const std::string& name, const std::vector<Type>& mem_ty)

  inline
  const std::string&
  getTypeName(void) noexcept { return name_; }

  llvm::Type*
  generateCode(llvm::LLVMContext& ctx) noexcept {
    static std::map<std::string, std::function<llvm::Type*(llvm::LLVMContext&)>> primitive_type_genmap_ = {
      { "void",   llvm::Type::getVoidTy },

      { "int8",   llvm::Type::getInt8Ty },
      { "int16",  llvm::Type::getInt16Ty },
      { "int32",  llvm::Type::getInt32Ty },
      { "int64",  llvm::Type::getInt64Ty },
      { "int128", llvm::Type::getInt128Ty },

      { "int8*",  std::bind(llvm::Type::getInt8PtrTy, std::placeholders::_1, 0) },
      { "int16*", std::bind(llvm::Type::getInt16PtrTy, std::placeholders::_1, 0) }
      // TODO
    };

    if(auto it = primitive_type_genmap_.find(name_);
      it != primitive_type_genmap_.end()) return it->second(ctx);
    else if(name_ == "char") return primitive_type_genmap_["int8"](ctx);

    return nullptr;
  }
};

template <typename A, typename B>
class BOp : AST<BOp<A, B>> {
public:
  enum class OpList {
    // Binary Operators
    BLT, BLE, BEQ, BGE, BGT,
    BNE
  };

  BOp(OpList opTy, A&& lhs, B&& rhs)
  : op_ty_(opTy), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  llvm::Value*
  generateCode(llvm::IRBuilder<>& builder) noexcept {
    using v_t = llvm::Value*;
    static std::map<OpList, std::function<v_t(v_t, v_t)>>
    // TODO: Optimization for [S?? vs U??] [ICmp vs FCmp]
    opMap = {
      { OpList::BLT, [&builder](v_t l, v_t r){ return builder.CreateICmpSLT(l, r); }},
      { OpList::BLE, [&builder](v_t l, v_t r){ return builder.CreateICmpSLE(l, r); }},
      { OpList::BEQ, [&builder](v_t l, v_t r){ return builder.CreateICmpEQ(l, r); }},
      { OpList::BGE, [&builder](v_t l, v_t r){ return builder.CreateICmpSGE(l, r); }},
      { OpList::BGT, [&builder](v_t l, v_t r){ return builder.CreateICmpSGT(l, r); }},
      { OpList::BNE, [&builder](v_t l, v_t r){ return builder.CreateICmpNE(l, r); }}
    };

    if(auto it = opMap.find(op_ty_); it != opMap.end())
      return it->second(lhs_.generateCode(builder, lhs_), rhs_.generateCode(builder));
    return nullptr;
  }

private:
  OpList op_ty_;
  A lhs_;
  B rhs_;
};

class Function : AST<Function> {
  Type ret_;
  std::vector<Type> arg_list_;

public:

  Function() : ret_("void"), arg_list_{} {}
  ~Function() = default;

  Function(Type ret, std::vector<Type>&& args)
  : ret_(ret), arg_list_(std::move(args)) {}

  llvm::Function*
  generateCode(llvm::IRBuilder<>& builder) noexcept {
    std::vector<llvm::Type*> arr(2);
    std::for_each(arg_list_.begin(), arg_list_.end(),
    [&](Type& arg) -> void { arr.push_back(arg.generateCode(builder.getContext())); });

    auto f_ty = llvm::FunctionType::get(
      ret_.generateCode(builder.getContext()), arr, false);
    return llvm::Function::Create(f_ty, llvm::Function::LinkageTypes::CommonLinkage);
  }
};

class Call : AST<Call> {
  std::variant<Call> fn_;
public:

  Call() = default;
  ~Call() = default;

  template <typename T>
  Call(T&& a) : fn_(std::move(a)) {}

  llvm::CallInst*
  generateCode(llvm::IRBuilder<>& builder) noexcept {
    // NOTE: builder.CreateConstrainedFPCall method exists
    return builder.CreateCall(
      llvm::FunctionCallee(
        std::visit([&builder](auto&& fn) -> llvm::Function* {
          if constexpr (
            std::is_same_v<Call,
              std::remove_reference_t<decltype(fn)>
            >) {
              // TODO: Return alloca invovling block
              llvm::CallInst* c = fn.generateCode(builder);
              return (llvm::Function*)builder.CreateAlloca(c->getType(), nullptr, "call_tmp_fn");
          } else
            return fn.generateCode(builder);
        }, fn_)));
  }
};

} // ns ast

NSRCXEND

#endif