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

#include "pattern.h"
#include "loop_mem_pat_node.h"
#include <list>
#include <map>
#include <sstream>
#include <string>

#define DEBUG

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
  std::vector<Loop *> loop_stack;

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

  void dumpGraph(raw_fd_ostream &file, Function *F) {
    // errs() << "Write\n";
    file << "digraph \"DFG for'" + F->getName() + "\' function\" {\n";

    // dump node
    for (node_list::iterator node = nodes.begin(), node_end = nodes.end();
         node != node_end; ++node) {
      // errs() << "Node First:" << node->first << "\n";
      // errs() << "Node Second:" << node-> second << "\n";
      if (dyn_cast<Instruction>(node->first)) {
        // file << "\tNode" << node->second << "[shape=record, label=\""
        //      << *(node->first) << "\"];\n";
        file << "\tNode" << node->first << "[shape=record, label=\""
             << node->second.c_str() << "\"];\n";
      } else {
        file << "\tNode" << node->first << "[shape=record, label=\""
             << node->second.c_str() << "\"];\n";
      }
    }

    //  dump instruction edges
#ifdef CFG
    for (edge_list::iterator edge = inst_edges.begin(),
                             edge_end = inst_edges.end();
         edge != edge_end; ++edge) {
      file << "\tNode" << edge->first.first << " -> Node" <<
      edge->second.first << "\n";
    }
#endif

    // dump data flow edges
    file << "edge [color=red]"
         << "\n";
    for (edge_list::iterator edge = edges.begin(), edge_end = edges.end();
         edge != edge_end; ++edge) {
      file << "\tNode" << edge->first.first << " -> Node" << edge->second.first
           << "\n";
    }

    file << "}\n";
  }

  // Convert instruction to string
  std::string convertIns2Str(Instruction *ins) {
    std::string temp_str;
    raw_string_ostream os(temp_str);
    ins->print(os);
    return os.str();
  }

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
      temp_result = v->getName().str();
      // errs() << temp_result << "\n";
    }
    return temp_result;
  }

  // const MDNode *findVar(const Value *V, const Function *F) {
  //   for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
  //        Iter != End; ++Iter) {
  //     const Instruction *I = &*Iter;
  //     if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
  //       if (DbgDeclare->getAddress() == V) {
  //         return DbgDeclare->getVariable();
  //         // return DbgDeclare->getOperand(1);
  //       }
  //     } else if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
  //       // errs() << "\nvalue:" <<DbgValue->getValue()  <<
  //       // *(DbgValue->getValue()) << "\nV:" << V << *V<< "\n";
  //       if (DbgValue->getValue() == V) {
  //         return DbgValue->getVariable();
  //         // return DbgValue->getOperand(1);
  //       }
  //     }
  //   }
  //   return nullptr;
  // }

  // std::string getDbgName(const Value *V, Function *F) {
  //   // TODO handle globals as well

  //   // const Function* F = findEnclosingFunc(V);
  //   if (!F)
  //     return V->getName().str();

  //   const MDNode *Var = findVar(V, F);
  //   if (!Var)
  //     return "tmp";

  //   // MDString * mds = dyn_cast_or_null<MDString>(Var->getOperand(0));
  //   // //errs() << mds->getString() << '\n';
  //   // if(mds->getString().str() != std::string("")) {
  //   // return mds->getString().str();
  //   // }else {
  //   // 	return "##";
  //   // }

  //   auto var = dyn_cast<DIVariable>(Var);
  //   // DIVariable *var(Var);

  //   return var->getName().str();
  // }

  bool endWith(const std::string &str, const std::string &tail) {
    return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
  }

  bool startWith(const std::string &str, const std::string &head) {
    return str.compare(0, head.size(), head) == 0;
  }

  Value *getLoopIndvar(Loop *L, ScalarEvolution &SE) {
    // auto phi = dyn_cast<PHINode>(L);
    PHINode *indvar_phinode = L->getInductionVariable(SE);
    // errs() << " phi: "<< indvar_phinode << '\n';
    // Value *indvar = dyn_cast<Value>(indvar_phinode);
    // return indvar;
    return indvar_phinode;
  }

  bool isLoopIndVar(Value *v) {
    auto iter = variant_value.find(v);
    if (iter != variant_value.end()) {
      return true;
    }
    return false;
  }

  PatNode *getGEPPattern(GetElementPtrInst *gep_inst, DataLayout *DL) {
    GEPOperator *gep_op = dyn_cast<GEPOperator>(gep_inst);
    Value *obj = gep_op->getPointerOperand();
    // errs() << getValueName(obj) << '\n';

    PatNode **patnode_array = new PatNode *[loop_stack.size()];

    for (int l = loop_stack.size() - 1; l >= 0; l--) {
      PatNode *gep_node = new PatNode(gep_inst, GEP_INST, getValueName(obj));
      auto LL = loop_stack[l];
      int num_operand = gep_inst->getNumOperands();
      for (int i = 1; i < num_operand; i++) {
        Value *idx = gep_inst->getOperand(i);
        PatNode *op_node = getOpPattern(idx, LL);
        gep_node->addChild(op_node);
      }
      dumpPattern(gep_node, 0);
      patnode_array[l] = gep_node;
    }

    // PatNode *gep_node1 = new PatNode(gep_inst, GEP_INST, getValueName(obj));

    // // int num_operand = gep_inst->getNumOperands();
    // for (int i = 1; i < num_operand; i++) {
    //   Value *idx = gep_inst->getOperand(i);
    //   PatNode *op_node = getOpPattern(idx, loop_stack[loop_stack.size() -
    //   2]); gep_node1->addChild(op_node);
    // }
    // dumpPattern(gep_node1, 0);

    // PatNode *gep_node2 = new PatNode(gep_inst, GEP_INST, getValueName(obj));

    // // int num_operand = gep_inst->getNumOperands();
    // for (int i = 1; i < num_operand; i++) {
    //   Value *idx = gep_inst->getOperand(i);
    //   PatNode *op_node = getOpPattern(idx, loop_stack[loop_stack.size() -
    //   3]); gep_node2->addChild(op_node);
    // }
    // dumpPattern(gep_node2, 0);

    return patnode_array[0];
  }

  PatNode *getBinaryOpPattern(BinaryOperator *bin_op, Loop *L) {
    auto opcode = bin_op->getOpcode();
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
    case llvm::Instruction::Shl:
      oss << "<<";
      break;
    case llvm::Instruction::LShr:
      oss << ">>";
      break;
    default:
      oss << "opcode: " << opcode;
      break;
    }

    PatNode *bin_node = new PatNode(bin_op, BIN_OP, oss.str());
#ifdef DEBUG
    errs() << "Process binary op " << getValueName(bin_op) << ": bin op is "
           << oss.str() << '\n';
#endif

    // Traverse all operands
    int num_operands = bin_op->getNumOperands();
    for (int i = 0; i < num_operands; i++) {
      auto operand = bin_op->getOperand(i);
      // auto operand = dyn_cast<Instruction>(operandi);
#ifdef DEBUG
      errs() << "  Process binary op operand " << getValueName(operand) << ": "
             << getValueName(operand) << '\n';
#endif
      if (!L->isLoopInvariant(operand)) {
#ifdef DEBUG
        errs() << "  variant vars: " << getValueName(operand) << '\n';
#endif
        auto child = getOpPattern(dyn_cast<Instruction>(operand), L);
        bin_node->addChild(child);
      } else if (isa<ConstantInt>(operand)) {
#ifdef DEBUG
        errs() << "  invariant vars: " << getValueName(operand) << '\n';
#endif
        auto child = getOpPattern(operand, L);
        bin_node->addChild(child);
      } else {
        PatNode *invar_var =
            new PatNode(operand, CONSTANT, getValueName(operand));
        bin_node->addChild(invar_var);
      }
    }

    return bin_node;
  }

  void getSExtPattern(SExtInst *sext_inst) { errs() << '=' << '\n'; }
  PatNode *getCastPattern(CastInst *sext_inst, Loop *L) {
    PatNode *cast_node = new PatNode(sext_inst, CAST_INST,
                                     getValueName(sext_inst->getOperand(0)));
#ifdef DEBUG
    errs() << "Process cast " << getValueName(sext_inst) << '\n';
#endif
    // traverse operand
    auto operand0 = sext_inst->getOperand(0);
    auto operand = dyn_cast<Instruction>(operand0);
#ifdef DEBUG
    errs() << "  Process cast operand " << getValueName(operand) << '\n';
#endif
    if (!L->isLoopInvariant(operand)) {
#ifdef DEBUG
      errs() << "  variant vars " << getValueName(operand) << '\n';
#endif
      auto child = getOpPattern(operand, L);

      cast_node->addChild(child);
    } else if (isa<ConstantInt>(operand)) {
#ifdef DEBUG
      errs() << "  invariant vars " << getValueName(operand) << '\n';
#endif
      auto child = getOpPattern(operand, L);

      cast_node->addChild(child);
    }

    return cast_node;
  }

  PatNode *getConstPattern(ConstantInt *const_v) {
#ifdef DEBUG
    errs() << "Process constant " << getValueName(const_v)
           << ": value = " << const_v->getSExtValue() << '\n';
#endif
    std::string temp_result = std::to_string(const_v->getSExtValue());
    PatNode *const_node = new PatNode(const_v, CONSTANT, temp_result);
    return const_node;
  }

  PatNode *getOpPattern(Instruction *curII, Loop *L) {
    if (isLoopIndVar(curII)) {
      PatNode *indvar_node =
          new PatNode(curII, LOOP_IND_VAR, getValueName(curII));
      return indvar_node;
    } else if (isa<BinaryOperator>(curII)) {
      auto bin_op = dyn_cast<BinaryOperator>(curII);
      return getBinaryOpPattern(bin_op, L);
    } else if (isa<CastInst>(curII)) {
      auto sext_inst = dyn_cast<CastInst>(curII);
      return getCastPattern(sext_inst, L);
    } else if (isa<ConstantInt>(curII)) {
      auto constant_v = dyn_cast<ConstantInt>(curII);
      return getConstPattern(constant_v);
    } else if (isa<PHINode>(curII)) {
      PatNode *phi_node = new PatNode(curII, CONSTANT, getValueName(curII));
      return phi_node;
    }

    return nullptr;
  }

  void handleLoop(Loop *L, LoopInfo &LI, DataLayout *DL, ScalarEvolution &SE,
                  Function *F, LoopMemPatNode* parent_node) {
    loop_stack.push_back(L);
    Value *indvar = getLoopIndvar(L, SE);
    // getValueName(indvar, F)

    variant_value.insert(make_pair(indvar, std::string("xx")));

    LoopPat* loop_pat = nullptr;
    LoopMemPatNode* loop_node = new LoopMemPatNode(LOOP_NODE, loop_pat);
    parent_node->AddChild(loop_node);


    errs() << "Loop index var:" << getValueName(indvar) << "\n\n";
    for (Loop::block_iterator BB = L->block_begin(), BEnd = L->block_end();
         BB != BEnd; ++BB) {
      BasicBlock *curBB = *BB;
      // check if bb belongs to L or inner loop, traverse the BB of Loop
      // itself.
      if (L != LI.getLoopFor(curBB)) {
        continue;
      }
      for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end();
           II != IEnd; ++II) {

        Instruction *curII = &*II;

        if (isa<GetElementPtrInst>(curII)) {
          errs() << *(curII) << "\n";
          GetElementPtrInst *gepinst = dyn_cast<GetElementPtrInst>(curII);

          //  auto user = dyn_cast<User>(curII);
          auto gep_pat = getGEPPattern(gepinst, DL);

          MemAcsPat* mem_acs_pat = new MemAcsPat(gep_pat);
          LoopMemPatNode* mem_acs_node = new LoopMemPatNode(MEM_ACS_NODE, mem_acs_pat);
          loop_node->AddChild(mem_acs_node);

          // // old version for simple pattern A[i]
          // GEPOperator *gepop = dyn_cast<GEPOperator>(curII);
          // Value *obj = gepop->getPointerOperand();
          // Value *idx = gepinst->getOperand(1);
          // errs() << getOriginalName(obj, F) << "[" << getOriginalName(idx, F)
          //        << "]" << '\n';
        }

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
      handleLoop(*SL, LI, DL, SE, F, loop_node);
    }
    loop_stack.pop_back();
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

    edges.clear();
    nodes.clear();
    inst_edges.clear();

    DataLayout *DL = new DataLayout(&M);

    LoopMemPatNode* func_node = new LoopMemPatNode(FUNC_NODE, F->getName().str());

    for (LoopInfo::iterator LL = LI.begin(), LEnd = LI.end(); LL != LEnd;
         ++LL) {
      Loop *L = *LL;
      handleLoop(L, LI, DL, SE, F, func_node);
    }

    dumpGraph(file, F);

    dumpLoopMemPatTree(func_node, 0);

    file.close();

    return;
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

    for (auto &F : M) {
      if (!(F.isDeclaration())) {
        funcDFG(&F, M);
      }
    }
    return true;
  }
};
} // namespace

char DFGPass::ID = 0;
static RegisterPass<DFGPass> X("DFGPass", "DFG Pass Analyze", false, false);