#ifndef LOOP_MEM_PAT_NODE_H_
#define LOOP_MEM_PAT_NODE_H_
#include <iostream>
#include <vector>
#include <string>
#include "pattern.h"

enum loop_mem_pat_node_type_t {
    FUNC_NODE = 100,
    LOOP_NODE = 101,
    MEM_ACS_NODE = 102,
    CALL_NODE = 103
};

class LoopPat {
private:
    std::string ind_var; // for loop ind var
    PatNode* start;
    PatNode* end;
    PatNode* step;

public:
    void dump() {
        // do nothing now
    }
};

class MemAcsPat {
private:
    PatNode* pat;
public:
    MemAcsPat(PatNode* pat)
     : pat(pat) {}
    void dump(int depth) {
        dumpPattern(pat, depth);
    }
};

class LoopMemPatNode {
private:
    loop_mem_pat_node_type_t type;
    std::string func_name = nullptr;
    LoopPat* loop_pat = nullptr;
    MemAcsPat* mem_acs_pat = nullptr;
    int num_children = 0;
    std::vector<LoopMemPatNode *> children;

public:
    LoopMemPatNode(loop_mem_pat_node_type_t type)
     : type(static_cast<loop_mem_pat_node_type_t>(type)) { }
    LoopMemPatNode(loop_mem_pat_node_type_t type, const std::string& func_name)
     : type(static_cast<loop_mem_pat_node_type_t>(type)), func_name(std::string(func_name)) { }
    LoopMemPatNode(loop_mem_pat_node_type_t type, LoopPat* loop_pat)
     : type(static_cast<loop_mem_pat_node_type_t>(type)), loop_pat(loop_pat) { }
    LoopMemPatNode(loop_mem_pat_node_type_t type, MemAcsPat* mem_acs_pat)
     : type(static_cast<loop_mem_pat_node_type_t>(type)), mem_acs_pat(mem_acs_pat) { }
    

    void AddChild(LoopMemPatNode * child) {
        num_children ++; 
        children.push_back(child);
    }

    loop_mem_pat_node_type_t getType() {
        return type;
    }

    int getNumChildren() {
        return num_children;
    }

    std::vector<LoopMemPatNode *>& getChildren() {
        return children;
    }

    LoopMemPatNode* getChild(int i) {
        if (i < num_children) {
            return children[i];
        } else {
            return nullptr;
        }
    }

    std::string& getFuncName() {
        if (type == FUNC_NODE) {
            return func_name;
        } else {
            func_name = std::string("nullptr");
        }        
        return func_name;
    }

    LoopPat* getLoopPat() {
        if (type == LOOP_NODE) {
            return loop_pat;
        } else {
            return nullptr;
        }
    }

    MemAcsPat* getMemAcsPat() {
        return mem_acs_pat;
    }

};

void dumpLoopMemPatTree(LoopMemPatNode *node, int depth) {

  if (!node) {
    return;
  }
  for (int i = 0; i < depth; i++) {
    std::cout << ' ';
  }
  auto type = node->getType();
  std::cout << node->getType() << ": ";

  if (type == FUNC_NODE) {
    std::cout << node->getFuncName() << " \n";
  } else if (type == LOOP_NODE) {
    auto loop_pat = node->getLoopPat();
    if (!loop_pat) {
        loop_pat->dump();
    }
  } else if (type == MEM_ACS_NODE) {
    auto mem_acs_pat = node->getMemAcsPat();
    if (!mem_acs_pat) {
        mem_acs_pat->dump(depth);
    }
  }

  auto children = node->getChildren();
  for (auto child : children) {
    dumpLoopMemPatTree(child, depth + 1);
  }
}

#endif