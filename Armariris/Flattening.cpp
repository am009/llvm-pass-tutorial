#include "Transforms/Obfuscation/Flattening.h"
#include "Transforms/Obfuscation/Utils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/CryptoUtils.h"

#define DEBUG_TYPE "flattening"

using namespace llvm;

// Stats
STATISTIC(Flattened, "Functions flattened");

static cl::opt<string> FunctionName(
    "funcFLA", cl::init(""),
    cl::desc(
        "Flatten only certain functions: -mllvm -funcFLA=\"func1,func2\""));

static cl::opt<int> Percentage(
    "perFLA", cl::init(100),
    cl::desc("Flatten only a certain percentage of functions"));

namespace {
struct Flattening : public FunctionPass {
  static char ID;  // Pass identification, replacement for typeid
  bool flag;

  Flattening() : FunctionPass(ID) {}
  Flattening(bool flag) : FunctionPass(ID) {
    this->flag = flag;
    // Check if the number of applications is correct
    if ( !((Percentage > 0) && (Percentage <= 100)) ) {
      LLVMContext ctx;
      ctx.emitError(Twine ("Flattening application function percentage -perFLA=x must be 0 < x <= 100"));
    }
  }

  bool runOnFunction(Function &F);
  bool flatten(Function *f);
};
}

char Flattening::ID = 0;
static RegisterPass<Flattening> X("flattening", "Call graph flattening");
Pass *llvm::createFlattening(bool flag) { return new Flattening(flag); }

bool Flattening::runOnFunction(Function &F) {
  Function *tmp = &F;

  // Do we obfuscate
  if (toObfuscate(flag, tmp, "fla") && ((int)llvm::cryptoutils->get_range(100) <= Percentage)) {
    //errs()<<"Flatten: "<<tmp->getName()<<"\n";

      /*if(tmp->getName()=="main"){
        errs()<<"Function: "<<tmp->getName()<<"\n";
        //fixStack(tmp);
        flatten(tmp);
        ++Flattened;
      }*/
    if(flatten(tmp)) {
    //  errs()<<"Function: "<<tmp->getName()<<"\n";
      ++Flattened;
    }
  }

  return false;
}

bool Flattening::flatten(Function *f) {
  vector<BasicBlock *> origBB;
  BasicBlock *loopEntry;
  BasicBlock *loopEnd;
  LoadInst *load;
  SwitchInst *switchI;
  AllocaInst *switchVar;

  // SCRAMBLER
  char scrambling_key[16];
  llvm::cryptoutils->get_bytes(scrambling_key, 16);
  // END OF SCRAMBLER

  // Lower switch
#if LLVM_VERSION_MAJOR >= 9
    // >=9.0, LowerSwitchPass depends on LazyValueInfoWrapperPass, which cause AssertError.
    // So I move LowerSwitchPass into register function, just before FlatteningPass.
#else
  FunctionPass *lower = createLowerSwitchPass();
  lower->runOnFunction(*f);
#endif
  // Save all original BB
  //errs()<<"Flatten: "<<f->getName()<<"\n";
  for (Function::iterator i = f->begin(); i != f->end(); ++i) {
    BasicBlock *tmp = &*i;
    origBB.push_back(tmp);

    BasicBlock *bb = &*i;
    //errs()<<(bb->getTerminator()->getNumSuccessors())<<"\n";
    if (isa<InvokeInst>(bb->getTerminator())) {
      /*if(f->getName()=="main"){
        continue;
      }*/
      //return false;
      continue;
    }
  }

  // Nothing to flatten
  if (origBB.size() <= 1) {
    return false;
  }



  // Remove first BB
  origBB.erase(origBB.begin());

  // Get a pointer on the first BB
  Function::iterator tmp = f->begin();  //++tmp;
  BasicBlock *insert = &*tmp;

  // If main begin with an if
  BranchInst *br = NULL;
  if (isa<BranchInst>(insert->getTerminator())) {
    br = cast<BranchInst>(insert->getTerminator());
    //errs()<<"Branch Inst"<<"\n";
  }

  if ((br != NULL && br->isConditional()) ||
      insert->getTerminator()->getNumSuccessors() > 1) {
    BasicBlock::iterator i = insert->back().getIterator();
    //errs()<<"Conditional Branch"<<"\n";

    if (insert->size() > 1) {
      i--;                    //Get the second last instruction
    }

    //errs()<<"IF Original Terminator: "<<*(insert->getTerminator())<<"\n";
    BasicBlock *tmpBB = insert->splitBasicBlock(i, "first");
    origBB.insert(origBB.begin(), tmpBB);
  }

  // Remove jump
  //errs()<<"Terminator: "<<*(insert->getTerminator())<<"\n";
  insert->getTerminator()->eraseFromParent();
  //errs()<<"New Terminator: "<<*(insert->back().getIterator())<<"\n";

  // Create switch variable and set as it
  switchVar =
      new AllocaInst(Type::getInt32Ty(f->getContext()), 0, "switchVar", insert);
  new StoreInst(
      ConstantInt::get(Type::getInt32Ty(f->getContext()),
                       llvm::cryptoutils->scramble32(0, scrambling_key)),
      switchVar, insert);

  // Create main loop
  loopEntry = BasicBlock::Create(f->getContext(), "loopEntry", f, insert);
  loopEnd = BasicBlock::Create(f->getContext(), "loopEnd", f, insert);

  load = new LoadInst(Type::getInt32Ty(f->getContext()), switchVar, "switchVar", loopEntry);

  // Move first BB on top
  insert->moveBefore(loopEntry);
  BranchInst::Create(loopEntry, insert);

  // loopEnd jump to loopEntry
  BranchInst::Create(loopEntry, loopEnd);

  BasicBlock *swDefault =
      BasicBlock::Create(f->getContext(), "switchDefault", f, loopEnd);
  BranchInst::Create(loopEnd, swDefault);

  // Create switch instruction itself and set condition
  switchI = SwitchInst::Create(&*(f->begin()), swDefault, 0, loopEntry);
  switchI->setCondition(load);

  // Remove branch jump from 1st BB and make a jump to the while
  //errs()<<"Terminator: "<<*(f->begin()->getTerminator())<<"\n";

  //f->begin()->getTerminator()->eraseFromParent();//???????????????????????????????????????????????????????????????????????
  //BranchInst::Create(loopEntry, &*(f->begin()));//????????????????????????????????????????????????????????????????????????

  // Put all BB in the switch
  for (vector<BasicBlock *>::iterator b = origBB.begin(); b != origBB.end();
       ++b) {
    BasicBlock *i = *b;
    ConstantInt *numCase = NULL;

    // Move the BB inside the switch (only visual, no code logic)
    i->moveBefore(loopEnd);

    // Add case to switch
    numCase = cast<ConstantInt>(ConstantInt::get(
        switchI->getCondition()->getType(),
        llvm::cryptoutils->scramble32(switchI->getNumCases(), scrambling_key)));
    switchI->addCase(numCase, i);
  }

  // Recalculate switchVar
  //errs()<<"Function: "<<f->getName()<<"\n";
  for (vector<BasicBlock *>::iterator b = origBB.begin(); b != origBB.end();
       ++b) {
    BasicBlock *i = *b;
    ConstantInt *numCase = NULL;

  //  errs()<<"Terminator: "<<*(i->getTerminator())<<"\n";

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(isa<InvokeInst>(i->getTerminator())){
      /*errs()<<"Has invoke instruction"<<"\n";
      errs()<<*(i->getTerminator())<<"\n";
      */
      InvokeInst *tmp=cast<InvokeInst>(i->getTerminator());
      //i->getTerminator()->eraseFromParent();

      BasicBlock *transfer=BasicBlock::Create(f->getContext(), "Transfer", f, insert);
      BasicBlock *normal=tmp->getNormalDest();
      transfer->moveBefore(loopEnd);
      numCase = cast<ConstantInt>(ConstantInt::get(
          switchI->getCondition()->getType(),
          llvm::cryptoutils->scramble32(switchI->getNumCases(), scrambling_key)));
      switchI->addCase(numCase, transfer);
      BranchInst::Create(normal,transfer);

      tmp->setNormalDest(transfer);

      numCase = switchI->findCaseDest(transfer);

      if (numCase == NULL) {
        numCase = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
                             llvm::cryptoutils->scramble32(
                                 switchI->getNumCases() - 1, scrambling_key)));
      }

      StoreInst *store=new StoreInst(numCase, load->getPointerOperand(),i);
      store->moveBefore(tmp);

      BranchInst::Create(loopEnd, i);
      //tmp->eraseFromParent();
      continue;

    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Ret BB
    if (i->getTerminator()->getNumSuccessors() == 0) {
      continue;
    }

    // If it's a non-conditional jump
    if (i->getTerminator()->getNumSuccessors() == 1) {
      // Get successor and delete terminator
      BasicBlock *succ = i->getTerminator()->getSuccessor(0);
      i->getTerminator()->eraseFromParent();

      // Get next case
      numCase = switchI->findCaseDest(succ);

      // If next case == default case (switchDefault)
      if (numCase == NULL) {
        numCase = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
                             llvm::cryptoutils->scramble32(
                                 switchI->getNumCases() - 1, scrambling_key)));
      }

      // Update switchVar and jump to the end of loop
      new StoreInst(numCase, load->getPointerOperand(), i);
      BranchInst::Create(loopEnd, i);
      continue;
    }

    // If it's a conditional jump
    if (i->getTerminator()->getNumSuccessors() == 2) {
      // Get next cases
      ConstantInt *numCaseTrue =
          switchI->findCaseDest(i->getTerminator()->getSuccessor(0));
      ConstantInt *numCaseFalse =
          switchI->findCaseDest(i->getTerminator()->getSuccessor(1));

      // Check if next case == default case (switchDefault)
      if (numCaseTrue == NULL) {
        numCaseTrue = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
                             llvm::cryptoutils->scramble32(
                                 switchI->getNumCases() - 1, scrambling_key)));
      }

      if (numCaseFalse == NULL) {
        numCaseFalse = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
                             llvm::cryptoutils->scramble32(
                                 switchI->getNumCases() - 1, scrambling_key)));
      }

      // Create a SelectInst
      BranchInst *br = cast<BranchInst>(i->getTerminator());
      SelectInst *sel =
          SelectInst::Create(br->getCondition(), numCaseTrue, numCaseFalse, "",
                             i->getTerminator());

      // Erase terminator
      i->getTerminator()->eraseFromParent();

      // Update switchVar and jump to the end of loop
      new StoreInst(sel, load->getPointerOperand(), i);
      BranchInst::Create(loopEnd, i);
      continue;
    }
  }

  fixStack(f);

  return true;
}
