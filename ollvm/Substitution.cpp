//===- Substitution.cpp - Substitution Obfuscation
// pass-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements operators substitution's pass
//
//===----------------------------------------------------------------------===//

#include "Transforms/Obfuscation/Substitution.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Intrinsics.h"

#define DEBUG_TYPE "substitution"

#define NUMBER_ADD_SUBST 4
#define NUMBER_SUB_SUBST 3
#define NUMBER_AND_SUBST 2
#define NUMBER_OR_SUBST 2
#define NUMBER_XOR_SUBST 2

static cl::opt<int>
ObfTimes("sub_loop",
         cl::desc("Choose how many time the -sub pass loops on a function"),
         cl::value_desc("number of times"), cl::init(1), cl::Optional);


// Stats
STATISTIC(Add, "Add substitued");
STATISTIC(Sub, "Sub substitued");
// STATISTIC(Mul,  "Mul substitued");
// STATISTIC(Div,  "Div substitued");
// STATISTIC(Rem,  "Rem substitued");
// STATISTIC(Shi,  "Shift substitued");
STATISTIC(And, "And substitued");
STATISTIC(Or, "Or substitued");
STATISTIC(Xor, "Xor substitued");

namespace {

struct Substitution : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  static void (*funcAdd[NUMBER_ADD_SUBST])(BinaryOperator *bo);
  static void (*funcSub[NUMBER_SUB_SUBST])(BinaryOperator *bo);
  static void (*funcAnd[NUMBER_AND_SUBST])(BinaryOperator *bo);
  static void (*funcOr[NUMBER_OR_SUBST])(BinaryOperator *bo);
  static void (*funcXor[NUMBER_XOR_SUBST])(BinaryOperator *bo);
  bool flag;

  Substitution() : FunctionPass(ID) {
    // this is really called
  }

  Substitution(bool flag) : FunctionPass(ID) {
    this->flag = flag;
  }

  bool runOnFunction(Function &F);
  bool substitute(Function *f);

  static void addNeg(BinaryOperator *bo);
  static void addDoubleNeg(BinaryOperator *bo);
  static void addRand(BinaryOperator *bo);
  static void addRand2(BinaryOperator *bo);

  static void subNeg(BinaryOperator *bo);
  static void subRand(BinaryOperator *bo);
  static void subRand2(BinaryOperator *bo);

  static void andSubstitution(BinaryOperator *bo);
  static void andSubstitutionRand(BinaryOperator *bo);

  static void orSubstitution(BinaryOperator *bo);
  static void orSubstitutionRand(BinaryOperator *bo);

  static void xorSubstitution(BinaryOperator *bo);
  static void xorSubstitutionRand(BinaryOperator *bo);
};

void (*Substitution::funcAdd[NUMBER_ADD_SUBST])(BinaryOperator *bo) = {&Substitution::addNeg, &Substitution::addDoubleNeg, &Substitution::addRand, &Substitution::addRand2};
void (*Substitution::funcSub[NUMBER_SUB_SUBST])(BinaryOperator *bo) = {&Substitution::subNeg, &Substitution::subRand, &Substitution::subRand2};
void (*Substitution::funcAnd[NUMBER_AND_SUBST])(BinaryOperator *bo) = {&Substitution::andSubstitution, &Substitution::andSubstitutionRand};
void (*Substitution::funcOr[NUMBER_OR_SUBST])(BinaryOperator *bo) = {&Substitution::orSubstitution, &Substitution::orSubstitutionRand};
void (*Substitution::funcXor[NUMBER_XOR_SUBST])(BinaryOperator *bo) = {&Substitution::xorSubstitution, &Substitution::xorSubstitutionRand};

}

char Substitution::ID = 0;
static RegisterPass<Substitution> X("substitution", "operators substitution");
Pass *llvm::createSubstitution(bool flag) { return new Substitution(flag); }

bool Substitution::runOnFunction(Function &F) {
   // Check if the percentage is correct
   if (ObfTimes <= 0) {
     errs()<<"Substitution application number -sub_loop=x must be x > 0";
	 return false;
   }

  Function *tmp = &F;
  // Do we obfuscate
  // if (toObfuscate(flag, tmp, "sub")) {
  //   substitute(tmp);
	//   return true;
  // }
  // auto name = F.getName();
  // if (name.compare("file_chmod") == 0) {
  //   substitute(tmp);
  //   return true;
  // }
  // just obfuscate
  substitute(tmp);
  return true;

  return false;
}

bool Substitution::substitute(Function *f) {
  Function *tmp = f;

  // Loop for the number of time we run the pass on the function
  int times = ObfTimes;
  do {
    for (Function::iterator bb = tmp->begin(); bb != tmp->end(); ++bb) {
      for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); ++inst) {
        if (inst->isBinaryOp()) {
          void (Substitution::*func)(BinaryOperator *bo);
          switch (inst->getOpcode()) {
          case BinaryOperator::Add:
            // case BinaryOperator::FAdd:
            // Substitute with random add operation
            (*funcAdd[llvm::cryptoutils->get_range(NUMBER_ADD_SUBST)])(
                cast<BinaryOperator>(inst));
            ++Add;
            break;
          case BinaryOperator::Sub:
            // case BinaryOperator::FSub:
            // Substitute with random sub operation
            (*funcSub[llvm::cryptoutils->get_range(NUMBER_SUB_SUBST)])(
                cast<BinaryOperator>(inst));
            ++Sub;
            break;
          case BinaryOperator::Mul:
          case BinaryOperator::FMul:
            //++Mul;
            break;
          case BinaryOperator::UDiv:
          case BinaryOperator::SDiv:
          case BinaryOperator::FDiv:
            //++Div;
            break;
          case BinaryOperator::URem:
          case BinaryOperator::SRem:
          case BinaryOperator::FRem:
            //++Rem;
            break;
          case Instruction::Shl:
            //++Shi;
            break;
          case Instruction::LShr:
            //++Shi;
            break;
          case Instruction::AShr:
            //++Shi;
            break;
          case Instruction::And:
            (*funcAnd[llvm::cryptoutils->get_range(2)])(cast<BinaryOperator>(inst));
            ++And;
            break;
          case Instruction::Or:
            this->orSubstitution(cast<BinaryOperator>(inst));
            // func = this->funcOr[llvm::cryptoutils->get_range(2)];
            // (this->*func)(cast<BinaryOperator>(inst));
            ++Or;
            break;
          case Instruction::Xor:
            (*funcXor[llvm::cryptoutils->get_range(2)])(cast<BinaryOperator>(inst));
            ++Xor;
            break;
          default:
            break;
          }              // End switch
        }                // End isBinaryOp
      }                  // End for basickblock
    }                    // End for Function
  } while (--times > 0); // for times
  return false;
}

// Implementation of a = b - (-c)
void Substitution::addNeg(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Create sub
  if (bo->getOpcode() == Instruction::Add) {
    op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
    op =
        BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), op, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }/* else {
    op = BinaryOperator::CreateFNeg(bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::FSub, bo->getOperand(0), op, "",
                                bo);
  }*/
}

// Implementation of a = -(-b + (-c))
void Substitution::addDoubleNeg(BinaryOperator *bo) {
  BinaryOperator *op, *op2 = NULL;
  UnaryOperator *opu, *opu2 = NULL;

  if (bo->getOpcode() == Instruction::Add) {
    op = BinaryOperator::CreateNeg(bo->getOperand(0), "", bo);
    op2 = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Add, op, op2, "", bo);
    op = BinaryOperator::CreateNeg(op, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
  } else {
    opu = UnaryOperator::CreateFNeg(bo->getOperand(0), "", bo);
    opu2 = UnaryOperator::CreateFNeg(bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::FAdd, opu, opu2, "", bo);
    opu = UnaryOperator::CreateFNeg(op, "", bo);
    bo->replaceAllUsesWith(opu);
    return;
  }

  bo->replaceAllUsesWith(op);
}

// Implementation of  r = rand (); a = b + r; a = a + c; a = a - r
void Substitution::addRand(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Add) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Add, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Add, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Sub, op, co, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FAdd,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
  } */
}

// Implementation of r = rand (); a = b - r; a = a + b; a = a + r
void Substitution::addRand2(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Add) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Add, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Add, op, co, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FAdd,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
  } */
}

// Implementation of a = b + (-c)
void Substitution::subNeg(BinaryOperator *bo) {
  BinaryOperator *op = NULL;
  UnaryOperator *opu = NULL;

  if (bo->getOpcode() == Instruction::Sub) {
    op = BinaryOperator::CreateNeg(bo->getOperand(1), "", bo);
    op =
        BinaryOperator::Create(Instruction::Add, bo->getOperand(0), op, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());
  } else {
    opu = UnaryOperator::CreateFNeg(bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::FAdd, bo->getOperand(0), opu, "",
                                bo);
  }

  bo->replaceAllUsesWith(op);
}

// Implementation of  r = rand (); a = b + r; a = a - c; a = a - r
void Substitution::subRand(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Sub) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Add, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Sub, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Sub, op, co, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FAdd,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,co,"",bo);
  } */
}

// Implementation of  r = rand (); a = b - r; a = a - c; a = a + r
void Substitution::subRand2(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  if (bo->getOpcode() == Instruction::Sub) {
    Type *ty = bo->getType();
    ConstantInt *co =
        (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());
    op =
        BinaryOperator::Create(Instruction::Sub, bo->getOperand(0), co, "", bo);
    op =
        BinaryOperator::Create(Instruction::Sub, op, bo->getOperand(1), "", bo);
    op = BinaryOperator::Create(Instruction::Add, op, co, "", bo);

    // Check signed wrap
    //op->setHasNoSignedWrap(bo->hasNoSignedWrap());
    //op->setHasNoUnsignedWrap(bo->hasNoUnsignedWrap());

    bo->replaceAllUsesWith(op);
  }
  /* else {
      Type *ty = bo->getType();
      ConstantFP *co =
  (ConstantFP*)ConstantFP::get(ty,(float)llvm::cryptoutils->get_uint64_t());
      op = BinaryOperator::Create(Instruction::FSub,bo->getOperand(0),co,"",bo);
      op = BinaryOperator::Create(Instruction::FSub,op,bo->getOperand(1),"",bo);
      op = BinaryOperator::Create(Instruction::FAdd,op,co,"",bo);
  } */
}

// Implementation of a = b & c => a = (b^~c)& b
void Substitution::andSubstitution(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Create NOT on second operand => ~c
  op = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // Create XOR => (b^~c)
  BinaryOperator *op1 =
      BinaryOperator::Create(Instruction::Xor, bo->getOperand(0), op, "", bo);

  // Create AND => (b^~c) & b
  op = BinaryOperator::Create(Instruction::And, op1, bo->getOperand(0), "", bo);
  bo->replaceAllUsesWith(op);
}

// Implementation of a = a && b <=> !(!a | !b) && (r | !r)
void Substitution::andSubstitutionRand(BinaryOperator *bo) {
  // Copy of the BinaryOperator type to create the random number with the
  // same type of the operands
  Type *ty = bo->getType();

  // r (Random number)
  ConstantInt *co =
      (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());

  // !a
  BinaryOperator *op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

  // !b
  BinaryOperator *op1 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // !r
  BinaryOperator *opr = BinaryOperator::CreateNot(co, "", bo);

  // (!a | !b)
  BinaryOperator *opa =
      BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

  // (r | !r)
  opr = BinaryOperator::Create(Instruction::Or, co, opr, "", bo);

  // !(!a | !b)
  op = BinaryOperator::CreateNot(opa, "", bo);

  // !(!a | !b) && (r | !r)
  op = BinaryOperator::Create(Instruction::And, op, opr, "", bo);

  // We replace all the old AND operators with the new one transformed
  bo->replaceAllUsesWith(op);
}

// Implementation of a = b | c => a = (b & c) | (b ^ c)
void Substitution::orSubstitutionRand(BinaryOperator *bo) {

  Type *ty = bo->getType();
  ConstantInt *co =
      (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());

  // !a
  BinaryOperator *op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

  // !b
  BinaryOperator *op1 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // !r
  BinaryOperator *op2 = BinaryOperator::CreateNot(co, "", bo);

  // !a && r
  BinaryOperator *op3 =
      BinaryOperator::Create(Instruction::And, op, co, "", bo);

  // a && !r
  BinaryOperator *op4 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(0), op2, "", bo);

  // !b && r
  BinaryOperator *op5 =
      BinaryOperator::Create(Instruction::And, op1, co, "", bo);

  // b && !r
  BinaryOperator *op6 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(1), op2, "", bo);

  // (!a && r) || (a && !r)
  op3 = BinaryOperator::Create(Instruction::Or, op3, op4, "", bo);

  // (!b && r) ||(b && !r)
  op4 = BinaryOperator::Create(Instruction::Or, op5, op6, "", bo);

  // (!a && r) || (a && !r) ^ (!b && r) ||(b && !r)
  op5 = BinaryOperator::Create(Instruction::Xor, op3, op4, "", bo);

  // !a || !b
  op3 = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

  // !(!a || !b)
  op3 = BinaryOperator::CreateNot(op3, "", bo);

  // r || !r
  op4 = BinaryOperator::Create(Instruction::Or, co, op2, "", bo);

  // !(!a || !b) && (r || !r)
  op4 = BinaryOperator::Create(Instruction::And, op3, op4, "", bo);

  // [(!a && r) || (a && !r) ^ (!b && r) ||(b && !r) ] || [!(!a || !b) && (r ||
  // !r)]
  op = BinaryOperator::Create(Instruction::Or, op5, op4, "", bo);
  bo->replaceAllUsesWith(op);
}

void Substitution::orSubstitution(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Creating first operand (b & c)
  op = BinaryOperator::Create(Instruction::And, bo->getOperand(0),
                              bo->getOperand(1), "", bo);

  // Creating second operand (b ^ c)
  BinaryOperator *op1 = BinaryOperator::Create(
      Instruction::Xor, bo->getOperand(0), bo->getOperand(1), "", bo);

  // final op
  op = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);
  bo->replaceAllUsesWith(op);
}

// Implementation of a = a ~ b => a = (!a && b) || (a && !b)
void Substitution::xorSubstitution(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  // Create NOT on first operand
  op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo); // !a

  // Create AND
  op = BinaryOperator::Create(Instruction::And, bo->getOperand(1), op, "",
                              bo); // !a && b

  // Create NOT on second operand
  BinaryOperator *op1 =
      BinaryOperator::CreateNot(bo->getOperand(1), "", bo); // !b

  // Create AND
  op1 = BinaryOperator::Create(Instruction::And, bo->getOperand(0), op1, "",
                               bo); // a && !b

  // Create OR
  op = BinaryOperator::Create(Instruction::Or, op, op1, "",
                              bo); // (!a && b) || (a && !b)
  bo->replaceAllUsesWith(op);
}

// implementation of a = a ^ b <=> (a ^ r) ^ (b ^ r) <=> (!a && r || a && !r) ^
// (!b && r || b && !r)
// note : r is a random number
void Substitution::xorSubstitutionRand(BinaryOperator *bo) {
  BinaryOperator *op = NULL;

  Type *ty = bo->getType();
  ConstantInt *co =
      (ConstantInt *)ConstantInt::get(ty, llvm::cryptoutils->get_uint64_t());

  // !a
  op = BinaryOperator::CreateNot(bo->getOperand(0), "", bo);

  // !a && r
  op = BinaryOperator::Create(Instruction::And, co, op, "", bo);

  // !r
  BinaryOperator *opr = BinaryOperator::CreateNot(co, "", bo);

  // a && !r
  BinaryOperator *op1 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(0), opr, "", bo);

  // !b
  BinaryOperator *op2 = BinaryOperator::CreateNot(bo->getOperand(1), "", bo);

  // !b && r
  op2 = BinaryOperator::Create(Instruction::And, op2, co, "", bo);

  // b && !r
  BinaryOperator *op3 =
      BinaryOperator::Create(Instruction::And, bo->getOperand(1), opr, "", bo);

  // (!a && r) || (a && !r)
  op = BinaryOperator::Create(Instruction::Or, op, op1, "", bo);

  // (!b && r) || (b && !r)
  op1 = BinaryOperator::Create(Instruction::Or, op2, op3, "", bo);

  // (!a && r) || (a && !r) ^ (!b && r) || (b && !r)
  op = BinaryOperator::Create(Instruction::Xor, op, op1, "", bo);
  bo->replaceAllUsesWith(op);
}

