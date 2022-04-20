#include <llvm/IR/Value.h>

#include <iostream>
#include <string>
#include <vector>

enum pat_node_type_t {
  CONSTANT = 0,
  BIN_OP = 1,
  CAST_INST = 2,
  LOOP_IND_VAR = 3,
  GEP_INST = 4
};

std::string typeToString(pat_node_type_t type) {
  std::string str;
  switch (type) {
  case 0:
    str = std::string("CONSTANT");
    break;
  case 1:
    str = std::string("BIN_OP");
    break;
  case 2:
    str = std::string("CAST_INST");
    break;
  case 3:
    str = std::string("LOOP_IND_VAR");
    break;
  case 4:
    str = std::string("GEP_INST");
    break;
  default:
    break;
  }
  return str;
}

class PatNode {
private:
  llvm::Value *val;
  pat_node_type_t type;
  std::string type_name;
  //   std::string constant = std::string('null'); // for constant type
  //   std::string val_name = std::string('null'); // for loop ind var type
  //   std::string op = std::string("null");       // for cast/binary
  std::string constant; // for constant type
  std::string val_name; // for loop ind var type
  std::string op;       // for cast/binary

  int num_children = 0;
  std::vector<PatNode *> children;

public:
  PatNode(llvm::Value *val, pat_node_type_t type, std::string str)
      : val(val), type(static_cast<pat_node_type_t>(type)) {
    if (type == CONSTANT) {
      constant = std::string(str);
      val_name = std::string(" ");
      op = std::string(" ");
    } else if (type == LOOP_IND_VAR || type == GEP_INST) {
      val_name = std::string(str);
      constant = std::string(" ");
      op = std::string(" ");
    } else {
      op = std::string(str);
      val_name = std::string(" ");
      constant = std::string(" ");
    }
    type_name = typeToString(type);
  }
  // PatNode(llvm::Value *val, pat_node_type_t type, int value)
  //     : val(val), type(static_cast<int>(type)), constant(value) {}

  llvm::Value *getValue() { return val; }

  int getType() { return type; }
  const std::string &getTypeName() {
    // std::string str = typeToString(type);
    // return str;
    return type_name;
  }

  const std::string &getConstantNum() {
    if (type == CONSTANT) {
      return constant;
    } else {
      return constant;
      // std::string str = std::string(" ");
      // return str;
      // // return 0;
    }
  }

  const std::string &getValueName() {
    if (type != CONSTANT) {
      return val_name;
    } else {
      // std::string str = std::string(" ");
      // return str;
      return constant;
    }
  }

  std::string &getOp() {
    if (type == BIN_OP || type == CAST_INST) {
      return op;
    } else {
      return op;
      // std::string str = std::string(" ");
      // return str;
    }
  }

  void addChild(PatNode *child) {
    children.push_back(child);
    num_children++;
  }

  std::vector<PatNode *> &getChildren() { return children; }
};

void dumpPattern(PatNode *pn, int depth) {
  //   if (pn == nullptr) {
  //     return;
  //   }
  if (!pn) {
    return;
  }
  for (int i = 0; i < depth; i++) {
    std::cout << ' ';
  }
  auto type = pn->getType();
  std::cout << pn->getType() << ": ";
  // if (type == CONSTANT) {
  //   std::cout << pn->getConstantNum() << "\n";
  // } else
  if (type != BIN_OP && type != CAST_INST) {
    std::cout << pn->getValueName() << "\n";
  } else {
    std::cout << pn->getOp() << "\n";
  }

  auto children = pn->getChildren();
  for (auto child : children) {
    dumpPattern(child, depth + 1);
  }
}

// class Pattern {

// }

// Pattern::