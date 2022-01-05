#include <llvm/Pass.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Use.h>

#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Support/raw_ostream.h>
//#include <llvm/DebugInfo.h>

#include <list>
#include <string>

using namespace llvm;
namespace {

	struct DFGPass : public ModulePass {
    public:
        static char ID;

		typedef std::pair<Value*, std::string> node;
		typedef std::pair<node, node> edge;
		typedef std::list<node> node_list;
		typedef std::list<edge> edge_list;
		
		//std::error_code error;
		edge_list inst_edges;  // control flow
		edge_list edges;    // data flow
		node_list nodes;	// instruction
		int num;
		int func_id = 0;
        DFGPass() : ModulePass(ID) {num = 0;}

		void getAnalysisUsage(AnalysisUsage& AU) const override{
			//AU.addRequired<CFG>();
			AU.setPreservesCFG();
			AU.addRequired<LoopInfoWrapperPass>();
			//AU.addRequired<RegionInfo>();
			ModulePass::getAnalysisUsage(AU);
		}

        // Convert instruction to string
		std::string convertIns2Str(Instruction* ins) {
			std::string temp_str;
			raw_string_ostream os(temp_str);
			ins->print(os);
			return os.str();
		}

        // std::string removeCharFromStr(std::string, char c) {

        // }

        template< class ForwardIt, class T >
        ForwardIt remove(ForwardIt first, ForwardIt last, const T& value)
        {
            first = std::find(first, last, value);
            if (first != last)
                for(ForwardIt i = first; ++i != last; )
                    if (!(*i == value))
                        *first++ = std::move(*i);
            return first;
        }

		// If v is variable, then use the name.
        // If v is instruction, then use the content.
		std::string getValueName(Value* v)
		{
			std::string temp_result = "val";
			if (!v) {
				return "undefined";
			}
			if (v->getName().empty())
			{
				temp_result += std::to_string(num);
				num++;
                //errs() << temp_result << "\n";
			}
			else {
				// std::string str = std::string(v->getName().str());
                // str.erase( std::remove( str.begin(),  str.end(),  '\"' ),  str.end() );
                // temp_result = str;
                temp_result = v->getName().str();
                //errs() << temp_result << "\n";
			}
			//StringRef result(temp_result);
			//errs() << result;
			//return result;
			return temp_result;
		}

        bool runOnModule(Module& M) override {
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
            
            //for(Module::iterator FI=M.begin(), E=M.end(); FI!=E; ++FI) {
			for (auto &f: M) {
				//Function *F = **FI;
				if (!(f.isDeclaration())) {
                funcDFG(&f);
				}
            }
        }

        void funcDFG(Function *F){
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(*F).getLoopInfo();
            std::error_code error;
			enum sys::fs::OpenFlags F_None;
			errs() << func_id << ": "<< F->getName().str() << "\n";
			StringRef fileName( std::to_string(func_id) + ".dot");
			func_id ++;
			raw_fd_ostream file(fileName, error, F_None);
			//errs() << "Hello\n";
			edges.clear();
			nodes.clear();
			inst_edges.clear();
			for(LoopInfo::iterator LL = LI.begin(),LEnd=LI.end();LL!=LEnd;++LL){
				Loop *L = *LL;
			for (Loop::block_iterator BB = L->block_begin(), BEnd = L->block_end(); BB != BEnd; ++BB) {
				BasicBlock *curBB = *BB;
				for (BasicBlock::iterator II = curBB->begin(), IEnd = curBB->end(); II != IEnd; ++II) {
					Instruction* curII = &*II;
					//errs() << getValueName(curII) << "\n";
					switch (curII->getOpcode())
					{
						// Load and store instruction
						case llvm::Instruction::Load:
						{
							LoadInst* linst = dyn_cast<LoadInst>(curII);
							Value* loadValPtr = linst->getPointerOperand();
							edges.push_back(edge(node(loadValPtr, getValueName(loadValPtr)), node(curII, getValueName(curII))));
							break;
						}
						case llvm::Instruction::Store: {
							StoreInst* sinst = dyn_cast<StoreInst>(curII);
							Value* storeValPtr = sinst->getPointerOperand();
							Value* storeVal = sinst->getValueOperand();
							edges.push_back(edge(node(storeVal, getValueName(storeVal)), node(curII, getValueName(curII))));
							edges.push_back(edge(node(curII, getValueName(curII)), node(storeValPtr, getValueName(storeValPtr))));
							break;
						}
						default: {
							for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op)
							{
								Instruction* tempIns;
								if (dyn_cast<Instruction>(*op))
								{
									edges.push_back(edge(node(op->get(), getValueName(op->get())), node(curII, getValueName(curII))));
								}
							}
							break;
						}

					}
					BasicBlock::iterator next = II;
					//errs() << curII << "\n";
					nodes.push_back(node(curII, getValueName(curII)));
					++next;
					if (next != IEnd) {
						inst_edges.push_back(edge(node(curII, getValueName(curII)), node(&*next, getValueName(&*next))));
					}
				}

				Instruction* terminator = curBB->getTerminator();
				for (BasicBlock* sucBB : successors(curBB)) {
					Instruction* first = &*(sucBB->begin());
					inst_edges.push_back(edge(node(terminator, getValueName(terminator)), node(first, getValueName(first))));
				}
			}
			}
			//errs() << "Write\n";
			file << "digraph \"DFG for'" + F->getName() + "\' function\" {\n";

			//dump node
			for (node_list::iterator node = nodes.begin(), node_end = nodes.end(); node != node_end; ++node) {
				//errs() << "Node First:" << node->first << "\n";
				//errs() << "Node Second:" << node-> second << "\n";
				if(dyn_cast<Instruction>(node->first))
					//file << "\tNode" << node->second << "[shape=record, label=\"" << *(node->first) << "\"];\n";
                    file << "\tNode" << node->first << "[shape=record, label=\"" << node->second.c_str() << "\"];\n";
				else
					file << "\tNode" << node->first << "[shape=record, label=\"" << node->second.c_str() << "\"];\n";
			}
			//errs() << "Write Done\n";
			// dump instruction edges
			for (edge_list::iterator edge = inst_edges.begin(), edge_end = inst_edges.end(); edge != edge_end; ++edge) {
				//file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
			}
			//dump data flow edges
			file << "edge [color=red]" << "\n";
			for (edge_list::iterator edge = edges.begin(), edge_end = edges.end(); edge != edge_end; ++edge) {
				file << "\tNode" << edge->first.first << " -> Node" << edge->second.first << "\n";
			}
			//errs() << "Write Done\n";
			file << "}\n";
			file.close();
			//return false;
			return;

        }
    };
}

char DFGPass::ID = 0;
static RegisterPass<DFGPass> X("DFGPass", "DFG Pass Analyze",
	false, false
);