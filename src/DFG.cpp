#include <llvm/Pass.h>

#include "llvm/IR/DerivedUser.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Use.h>
#include <llvm/IR/User.h>
#include <llvm/IR/Value.h>

#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/ScalarEvolution.h>

#include <llvm/Support/raw_ostream.h>
//#include <llvm/DebugInfo.h>

#include <list>
#include <map>
#include <sstream>
#include <string>

using namespace llvm;
namespace {

struct DFGPass : public ModulePass {
public:
  static char ID;

  typedef std::pair<Value *, std::string> node;
  typedef std::pair<node, node> edge;
  typedef std::list<node> node_list;
  typedef std::list<edge> edge_list;

  std::map<Value *, std::string> variant_value;

  // std::error_code error;
  edge_list inst_edges; // control flow
  edge_list edges;      // data flow
  node_list nodes;      // instruction


  int num;
  int func_id = 0;
  DFGPass() : ModulePass(ID) { num = 0; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    // AU.addRequired<CFG>();
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    // AU.addRequired<RegionInfo>();
    ModulePass::getAnalysisUsage(AU);
  }

  // Convert instruction to string
  std::string convertIns2Str(Instruction *ins) {
    std::string temp_str;
    raw_string_ostream os(temp_str);
    ins->print(os);
    return os.str();
  }

  // std::string removeCharFromStr(std::string, char c) {

  // }

  template <class ForwardIt, class T>
  ForwardIt remove(ForwardIt first, ForwardIt last, const T &value) {
    first = std::find(first, last, value);
    if (first != last)
      for (ForwardIt i = first; ++i != last;)
        if (!(*i == value))
          *first++ = std::move(*i);
    return first;
  }

  // If v is variable, then use the name.
  // If v is instruction, then use the content.
  std::string getValueName(Value *v) {
    std::string temp_result = "#val";
    if (!v) {
      return "undefined";
    }
    if (v->getName().empty()) {
      if (isa<ConstantInt>(v)) {
        auto constant_v = dyn_cast<ConstantInt>(v);
        temp_result = std::to_string(constant_v->getSExtValue());
      } else {
        temp_result += std::to_string(num);
        num++;
      }
      // errs() << temp_result << "\n";
    } else {
      // std::string str = std::string(v->getName().str());
      // str.erase( std::remove( str.begin(),  str.end(),  '\"' ),  str.end() );
      // temp_result = str;
      temp_result = v->getName().str();
      // errs() << temp_result << "\n";
    }
    // StringRef result(temp_result);
    // errs() << result;
    // return result;
    return temp_result;
  }

  // std::string getDbgName(Value* v){

  // 	MDNode* N = cast<MDNode>(v);
  // 	if (N) {
  // 		MDString * mds = dyn_cast_or_null<MDString>(N->getOperand(2));
  // 		errs() << mds->getString();

  // 	 	// DIVariable *var(N);
  // 		// errs() << var.getName() << '\n';

  // 	}
  // 	return "$$";
  // }

  const MDNode *findVar(const Value *V, const Function *F) {
    for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
         Iter != End; ++Iter) {
      const Instruction *I = &*Iter;
      if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
        if (DbgDeclare->getAddress() == V) {
          return DbgDeclare->getVariable();
          // return DbgDeclare->getOperand(1);
        }
      } else if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
        // errs() << "\nvalue:" <<DbgValue->getValue()  <<
        // *(DbgValue->getValue()) << "\nV:" << V << *V<< "\n";
        if (DbgValue->getValue() == V) {
          return DbgValue->getVariable();
          // return DbgValue->getOperand(1);
        }
      }
    }
    return nullptr;
  }

  std::string getOriginalName(const Value *V, Function *F) {
    // TODO handle globals as well

    // const Function* F = findEnclosingFunc(V);
    if (!F)
      return V->getName().str();

    const MDNode *Var = findVar(V, F);
    if (!Var)
      return "tmp";

    // MDString * mds = dyn_cast_or_null<MDString>(Var->getOperand(0));
    // //errs() << mds->getString() << '\n';
    // if(mds->getString().str() != std::string("")) {
    // return mds->getString().str();
    // }else {
    // 	return "##";
    // }

    DIVariable *var(Var);

    return var->getName().str();
  }

  bool endWith(const std::string &str, const std::string &tail) {
    return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
  }

  bool startWith(const std::string &str, const std::string &head) {
    return str.compare(0, head.size(), head) == 0;
  }

  static std::string getGEPPattern(GetElementPtrInst *curII, DataLayout *DL) {
    // vector<std::string> op_pattern;
    auto usr = dyn_cast<User>(curII);
    std::ostringstream oss;
    uint64_t offset = 0;
    unsigned num_op = 0;
    Type *elemTy = usr->getOperand(0)->getType();
    auto GTI = gep_type_begin(usr);
    auto GTE = gep_type_end(usr);
    // gep_type start from Operand(1). Operand(0) is considered as a base
    // address. The rest is used for compute the offset.
    for (auto i = 1; GTI != GTE; ++GTI, ++i) {
      Value *idx = GTI.getOperand();
      // std::string &pat = op_pattern[i];
      if (StructType *sTy = GTI.getStructTypeOrNull()) {
        if (!idx->getType()->isIntegerTy(32)) {
          oss << "u";
          // myassert("Type error!");
          break;
        }
        unsigned fieldNo = cast<ConstantInt>(idx)->getZExtValue();
        const StructLayout *SL = DL->getStructLayout(sTy);
        offset += SL->getElementOffset(fieldNo);
      } else {
        if (ConstantInt *csti = dyn_cast<ConstantInt>(idx)) {
          uint64_t arrayIdx = csti->getSExtValue();
          offset += arrayIdx * DL->getTypeAllocSize(GTI.getIndexedType());
        } else {
          if (offset != 0) {
            oss << "c" << offset;
            offset = 0;
            ++num_op;
          }
          oss << "*2c" << DL->getTypeAllocSize(GTI.getIndexedType()); // << pat;
          ++num_op;
        }
      }
    }

    if (offset != 0) {
      oss << "c" << offset;
      offset = 0;
      ++num_op;
    }

    auto vs = oss.str();
    oss.str("");
    oss.clear();
    if (num_op) {
      oss << "+" << (num_op + 1) << vs;
    } else {
      // oss << ;
    }

    return oss.str();
  }

  bool runOnModule(Module &M) override {
    // Mark all recursive functions
    // unordered_map<CallGraphNode *, Function *> callGraphNodeMap;
    // auto &cg = getAnalysis<CallGraph>();
    // for (auto &f: M) {
    // 	auto cgn = cg[&f];
    // 	callGraphNodeMap[cgn] = &f;
    // }
    // scc_iterator<CallGraph *> cgSccIter = scc_begin(&cg);
    // while (!cgSccIter.isAtEnd()) {
    // 	if (cgSccIter.hasLoop()) {
    // 		const vector<CallGraphNode*>& nodeVec = *cgSccIter;
    // 		for (auto cgn: nodeVec) {
    // 			auto f = callGraphNodeMap[cgn];
    // 			recSet.insert(f);
    // 		}
    // 	}
    // 	++cgSccIter;
    // }

    // for(Module::iterator FI=M.begin(), E=M.end(); FI!=E; ++FI) {
    for (auto &f : M) {
      // Function *F = **FI;
      if (!(f.isDeclaration())) {
        funcDFG(&f, M);
      }
    }
    return true;
  }

  Value *getLoopIndvar(Loop *L, ScalarEvolution &SE) {
    // auto phi = dyn_cast<PHINode>(L);
    PHINode *indvar_phinode = L->getInductionVariable(SE);
    // errs() << " phi: "<< indvar_phinode << '\n';
    Value *indvar = dyn_cast<Value>(indvar_phinode);
    return indvar;
  }

  void getBinaryOpPattern(BinaryOperator *curII) {
    auto opcode = curII->getOpcode();
    // errs() << opcode << '\n';
    std::ostringstream oss;
    switch (opcode) {
    case 11:
    case llvm::Instruction::Add:
      oss << "+";
      break;
    case llvm::Instruction::Sub:
      oss << '-';
      break;
    case llvm::Instruction::UDiv:
      oss << '/';
      break;
    case llvm::Instruction::SDiv:
      oss << '/';
      break;
    case llvm::Instruction::Mul:
      oss << '*';
      break;
    case llvm::Instruction::FAdd:
      oss << "+";
      break;
    case llvm::Instruction::FSub:
      oss << '-';
      break;
    case llvm::Instruction::FDiv:
      oss << '/';
      break;
    case llvm::Instruction::FMul:
      oss << '*';
      break;
    // case llvm::Instruction::Ssub:
    //   oss << '-';
    //   break;
    // case llvm::Instruction::Sdiv:
    //   oss << '/';
    //   break;
    // case llvm::Instruction::Smul:
    //   oss << '*';
    //   break;
    // case 13:
    // case 14:
    //   oss << "-";
    //   break;
    // case 15:
    // case 16:
    //   oss << "*";
    //   break;
    // case 17:
    // case 18:
    // case 19:
    //   oss << "/";
    //   break;
    // case 20:
    // case 21:
    case 22:
      oss << "%%";
      break;
    // TODO: Logical shift and arithmetic shift
    case 23:
      oss << "<";
      break;
    case 24:
    case 25:
      oss << ">";
      break;
    case 26:
      oss << "&";
      break;
    case 27:
      oss << "|";
      break;
    case 28:
      oss << "^";
      break;
    default:
      oss << "opcode: " << opcode;
      break;
    }
    errs() << oss.str() << '\n';
  }

  void getSExtPattern(SExtInst *sext_inst) { errs() << '=' << '\n'; }
  void getSExtPattern(CastInst *sext_inst) { errs() << '=' << '\n'; }

  void getOpPattern(Instruction *curII, DataLayout *DL) {
    if (isa<BinaryOperator>(curII)) {
      auto *bin_op = dyn_cast<BinaryOperator>(curII);
      getBinaryOpPattern(bin_op);
    }
    // else if (isa<SExtInst>(curII)) {
    else if (isa<CastInst>(curII)) {
      auto *sext_inst = dyn_cast<CastInst>(curII);
      getSExtPattern(sext_inst);
    } else if (isa<GetElementPtrInst>(curII)) {
      auto *gep_inst = dyn_cast<GetElementPtrInst>(curII);
      getGEPPattern(gep_inst, DL);
    }
  }

  void handleLoop(Loop *L, LoopInfo &LI, DataLayout *DL, ScalarEvolution &SE, Function* F) {
    
    Value *indvar = getLoopIndvar(L, SE);
    // getValueName(indvar, F)

    variant_value.insert(make_pair(indvar, std::string("xx")));

    errs() << "Loop index var:" << getValueName(indvar) << '\n';
    for (Loop::block_iterator BB = L->block_begin(), BEnd = L->block_end();
         BB != BEnd; ++BB) {
      BasicBlock *curBB = *BB;
      // check if bb belongs to L or inner loop, traverse the BB of Loop itself.
      if (L != LI.getLoopFor(curBB)) {
        continue;
      }
      for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end();
           II != IEnd; ++II) {

        Instruction *curII = &*II;

        if (!isa<GetElementPtrInst>(curII)) {

          int num_operands = curII->getNumOperands();

          bool is_variant = false;
          for (int i = 0; i < num_operands; i++) {
            Value *operand = curII->getOperand(i);

            auto iter = variant_value.find(operand);
            if (iter != variant_value.end()) {
              is_variant = true;
            }
            // if (variant_value.count(variant_value) > 0) {
            //   is_variant = true;
            // }
            // errs() << getValueName(operand) << '\n';
          }
          if (is_variant == true) {
            errs() << *curII << '\n';
            for (int i = 0; i < num_operands; i++) {
              Value *operand = curII->getOperand(i);
              if (!L->isLoopInvariant(operand)) {
                errs() << "variant vars: " << getValueName(operand) << '\n';
              } else {
                errs() << "invariant vars: " << getValueName(operand) << '\n';
              }
            }
            variant_value.insert(std::make_pair(curII, std::string("xx")));
            
            // Analyze the pattern
            getOpPattern(curII, DL);
          }
        }

        // if (!L->hasLoopInvariantOperands(curII)) {
        // 	errs() << *curII << '\n';
        // }

        if (curII->getOpcode() == llvm::Instruction::GetElementPtr) {
          errs() << *(curII) << "\n";
          GetElementPtrInst *gepinst = dyn_cast<GetElementPtrInst>(curII);

          // auto user = dyn_cast<User>(curII);
          auto pat = getGEPPattern(gepinst, DL);

          errs() << pat << "\n";

          GEPOperator *gepop = dyn_cast<GEPOperator>(curII);
          Value *obj = gepop->getPointerOperand();
          // if(gepinst->hasIndices()){

          // }
          Value *idx = gepinst->getOperand(1);
          // MDNode* N1 = cast<MDNode>(gepinst->getOperand(0));
          // MDString * mds = dyn_cast_or_null<MDString>(N1->getOperand(0));
          // errs() << mds->getString() << '\n';
          // DIVariable *var1(N1);
          // errs() << var1->getName() << '\n';

          // MDNode* N2 = cast<MDNode>(gepinst->getOperand(1));
          // DIVariable *var2(N2);
          // errs() << var2->getName() << '\n';

          // if (startWith(getValueName(idx),std::string("#val"))) {

          // }
          errs() << getOriginalName(obj, F) << "[" << getOriginalName(idx, F)
                 << "]" << '\n';
          // errs() << idx << *idx<< '\n';
          // errs() <<  "[" << getOriginalName(obj, F) << "]" << '\n';
        }

        // errs() << getValueName(curII) << "\n";
        switch (curII->getOpcode()) {
        // Load and store instruction
        case llvm::Instruction::Load: {
          LoadInst *linst = dyn_cast<LoadInst>(curII);
          Value *loadValPtr = linst->getPointerOperand();
          edges.push_back(edge(node(loadValPtr, getValueName(loadValPtr)),
                               node(curII, getValueName(curII))));
          break;
        }
        case llvm::Instruction::Store: {
          StoreInst *sinst = dyn_cast<StoreInst>(curII);
          Value *storeValPtr = sinst->getPointerOperand();
          Value *storeVal = sinst->getValueOperand();
          edges.push_back(edge(node(storeVal, getValueName(storeVal)),
                               node(curII, getValueName(curII))));
          edges.push_back(edge(node(curII, getValueName(curII)),
                               node(storeValPtr, getValueName(storeValPtr))));
          break;
        }
        default: {
          for (Instruction::op_iterator op = curII->op_begin(),
                                        opEnd = curII->op_end();
               op != opEnd; ++op) {
            Instruction *tempIns;
            if (dyn_cast<Instruction>(*op)) {
              edges.push_back(edge(node(op->get(), getValueName(op->get())),
                                   node(curII, getValueName(curII))));
            }
          }
          break;
        }
        }
        BasicBlock::iterator next = II;
        // errs() << curII << "\n";
        nodes.push_back(node(curII, getValueName(curII)));
        ++next;
        if (next != IEnd) {
          inst_edges.push_back(edge(node(curII, getValueName(curII)),
                                    node(&*next, getValueName(&*next))));
        }
      }

      Instruction *terminator = curBB->getTerminator();
      for (BasicBlock *sucBB : successors(curBB)) {
        Instruction *first = &*(sucBB->begin());
        inst_edges.push_back(edge(node(terminator, getValueName(terminator)),
                                  node(first, getValueName(first))));
      }
    }

    // traverse the inner Loops by recursive method
    std::vector<Loop *> subLoops = L->getSubLoops();
    Loop::iterator SL, SLE;
    for (SL = subLoops.begin(), SLE = subLoops.end(); SL != SLE; ++SL) {
      handleLoop(*SL, LI, DL, SE, F);
    }
  }

  void funcDFG(Function *F, Module &M) {
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
    ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(*F).getSE();
    std::error_code error;
    enum sys::fs::OpenFlags F_None;
    errs() << func_id << ": " << F->getName().str() << "\n";
    StringRef fileName(std::to_string(func_id) + ".dot");
    func_id++;
    raw_fd_ostream file(fileName, error, F_None);
    // errs() << "Hello\n";
    edges.clear();
    nodes.clear();
    inst_edges.clear();

    // std::map <Value*, MDNode*> value2MD;

    DataLayout *DL = new DataLayout(&M);

    for (LoopInfo::iterator LL = LI.begin(), LEnd = LI.end(); LL != LEnd;
         ++LL) {
      Loop *L = *LL;
      handleLoop(L, LI, DL, SE, F);
    }

    // errs() << "Write\n";
    file << "digraph \"DFG for'" + F->getName() + "\' function\" {\n";

    // dump node
    for (node_list::iterator node = nodes.begin(), node_end = nodes.end();
         node != node_end; ++node) {
      // errs() << "Node First:" << node->first << "\n";
      // errs() << "Node Second:" << node-> second << "\n";
      if (dyn_cast<Instruction>(node->first))
        file << "\tNode" << node->second << "[shape=record, label=\""
             << *(node->first) << "\"];\n";
      // file << "\tNode" << node->first << "[shape=record, label=\"" <<
      // node->second.c_str() << "\"];\n";
      else
        file << "\tNode" << node->first << "[shape=record, label=\""
             << node->second.c_str() << "\"];\n";
    }
    // errs() << "Write Done\n";
    //  dump instruction edges
    for (edge_list::iterator edge = inst_edges.begin(),
                             edge_end = inst_edges.end();
         edge != edge_end; ++edge) {
      // file << "\tNode" << edge->first.first << " -> Node" <<
      // edge->second.first << "\n";
    }
    // dump data flow edges
    file << "edge [color=red]"
         << "\n";
    for (edge_list::iterator edge = edges.begin(), edge_end = edges.end();
         edge != edge_end; ++edge) {
      file << "\tNode" << edge->first.first << " -> Node" << edge->second.first
           << "\n";
    }
    // errs() << "Write Done\n";
    file << "}\n";
    file.close();

    return;
  }

}; 
}// namespace

char DFGPass::ID = 0;
static RegisterPass<DFGPass> X("DFGPass", "DFG Pass Analyze", false, false);