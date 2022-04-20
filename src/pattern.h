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
  //   std::string constant = std::string('null'); // for constant type
  //   std::string val_name = std::string('null'); // for loop ind var type
  //   std::string op = std::string("null");       // for cast/binary
  int constant; // for constant type
  std::string val_name; // for loop ind var type
  std::string op;       // for cast/binary

  int num_children = 0;
  std::vector<PatNode *> children;

public:
  PatNode(llvm::Value *val, pat_node_type_t type, std::string str)
      : val(val), type(static_cast<int>(type)) {
    if (type == CONSTANT) {
      constant = 0;
    } else if (type == LOOP_IND_VAR || type == GEP_INST) {
      val_name = std::string(str);
    } else {
      op = std::string(str);
    }
  }
  PatNode(llvm::Value *val, pat_node_type_t type, int value)
      : val(val), type(static_cast<int>(type)), constant(value) {}

  llvm::Value *getValue() { return val; }

  int getType() { return type; }
  std::string &getTypeName() {
    std::string str = typeToString(type);
    return str;
  }

  int getConstantNum() {
    if (type == CONSTANT) {
      return constant;
    } else {
      //   return constant;
      // std::string str = std::string(" ");
      //     return str;
      return 0;
    }
  }

  std::string &getValueName() {
    if (type != CONSTANT) {
      return val_name;
    } else {
      std::string str = std::string(" ");
      return str;
      //   return constant;
    }
  }

  std::string &getOp() {
    if (type == BIN_OP || type == CAST_INST) {
      return op;
    } else {
      //   return op;
      std::string str = std::string(" ");
      return str;
    }
  }

  void addChild(PatNode *child) {
    children.push_back(child);
    num_children++;
  }

  std::vector<PatNode *> &getChildren() { return children; }
};

void dumpPattern(PatNode *pn, int depth) {
  if (!pn) {
    return;
  }
  for (int i = 0; i < depth; i++) {
    std::cout << ' ';
  }
  auto type = pn->getType();
  std::cout << pn->getType() << ": ";
  if (type == CONSTANT) {
    std::cout << pn->getConstantNum() << "\n";
  } else if (type != BIN_OP && type != CAST_INST) {
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