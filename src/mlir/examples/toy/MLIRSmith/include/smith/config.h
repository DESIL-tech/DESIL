//
// Created by Stan Wang on 2022/10/5.
//
#ifndef CONFIG_H
#define CONFIG_H

#include "smith/json.hpp"
#include "time.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using json = nlohmann::json;

const int priority_base = 100;
const int integer_set_num = 16;
const int integer_set_exprs_ub = 4;
const int affine_expr_comb_ub = 16;
const int affine_map_res_ub = 4;
const int affine_map_pool_num = 16;
const int affine_expr_dim_ub = 4;
const int memAllocsSize = 16;
const int tenConsSize = 16;
const int eleConstantSize = 16;
const int affine_ub = 16;
const int linalg_operands_num_ub = 4;
const int memref_access_num = 4;
const int affine_parallel_range_dim_ub = 4;
const int affine_range_ub = 128;

struct OpConf {
  std::string dialect;
  std::string op;
  float prob;
  std::vector<std::string> parentOps;
  bool hasRegion;
  bool isTerminator;

  OpConf(const OpConf &conf) {
    dialect = conf.dialect;
    op = conf.op;
    prob = conf.prob;
    parentOps = std::vector<std::string>(conf.parentOps);
    hasRegion = conf.hasRegion;
    isTerminator = conf.isTerminator;
  }

  OpConf(json obj) {
    dialect = obj["dialect"];
    op = obj["op"];
    prob = obj["prob"];
    json arr = obj["parent_ops"];
    for (json::iterator it = arr.begin(); it != arr.end(); it++) {
      parentOps.push_back(*it);
    }
    //    parentOps = obj["parent_ops"]; // TODO read form json arr
    isTerminator = obj["is_terminator"];
  }

  OpConf(std::string dialect, std::string op, float prob,
         std::vector<std::string> parentOps, bool isTerminator = false)
      : dialect(dialect), op(op), prob(prob), parentOps(parentOps),
        isTerminator(isTerminator) {}

  json toJson() {
    json j;
    j["dialect"] = dialect;
    j["op"] = op;
    j["prob"] = prob;
    for (auto p : parentOps) {
      j["parent_ops"].push_back(p);
    }
    j["is_terminator"] = isTerminator;
    return j;
  }

  std::string getFullName() { return dialect + "." + op; }
};

struct Conf {
  bool execute; // true for diff test
  bool debug;
  int regionLengthLimit;
  int regionDepthLimit;

  std::vector<OpConf> supported_ops;

  Conf(const Conf &conf) {
    debug = conf.debug;
    for (auto op : conf.supported_ops) {
      supported_ops.push_back(OpConf(op));
    }
  }

  Conf(json j) {
    debug = j["debug"];
    supported_ops = std::vector<OpConf>();
    json arr = j["supported_ops"];
    for (json::iterator it = arr.begin(); it != arr.end(); ++it) {
      auto op = OpConf(*it);
      supported_ops.push_back(OpConf(*it));
    }
  }

  json toJson() {
    json j;
    j["debug"] = debug;
    for (auto op : supported_ops) {
      j["supported_ops"].push_back(op.toJson());
    }
    return j;
  }

  Conf() : supported_ops(std::vector<OpConf>()) { debug = false; }
};

inline Conf conf;

inline int rand(int mod) {
  int rd = rand();
  return rd % mod;
}

inline Conf getConfigTemplate() {
  Conf c;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  /* using nano-seconds instead of seconds */
  srand((time_t)ts.tv_nsec);
  //  srand(0);
  c.execute = true;
  c.debug = false;
  c.regionDepthLimit = 3;

  std::vector<OpConf> funcOpConfs;
  funcOpConfs.push_back(
      OpConf("func", "func", rand(priority_base), {"builtin.module"}));
  funcOpConfs.push_back(
      OpConf("func", "call", rand(priority_base), {"func.func"}));

    std::vector<OpConf> linalgOpConfs;
    linalgOpConfs.push_back(OpConf("linalg", "matmul", rand(priority_base), {"func.func"}));
    linalgOpConfs.push_back(OpConf("linalg", "generic", rand(priority_base), {"func.func"}));
    linalgOpConfs.push_back(OpConf("linalg", "map", rand(priority_base), {"func.func"}));
    linalgOpConfs.push_back(OpConf("linalg", "copy", rand(priority_base), {}));
    linalgOpConfs.push_back(OpConf("linalg", "transpose", rand(priority_base), {}));
    linalgOpConfs.push_back(OpConf("linalg", "broadcast", rand(priority_base), {}));
    // linalgOpConfs.push_back(OpConf("linalg", "dot", rand(priority_base), {}));

    std::vector<OpConf> memrefOpConfs;

    memrefOpConfs.push_back(OpConf("memref", "assume_alignment", rand(priority_base), {"func.func"}));
    memrefOpConfs.push_back( OpConf("memref", "alloc",rand(priority_base), {"func.func"}));
    // memrefOpConfs.push_back(OpConf("memref", "atomic_rmw", rand(priority_base), {"func.func"}));
    // memrefOpConfs.push_back(OpConf("memref", "realloc", rand(priority_base), {"func.func"}));
    // memrefOpConfs.push_back(OpConf("memref", "tensor_store", rand(priority_base), {"func.func"}));
    memrefOpConfs.push_back(OpConf("memref", "store", rand(priority_base), {"func.func"}));
    memrefOpConfs.push_back(OpConf("memref", "alloca", rand(priority_base), {}));
    // memrefOpConfs.push_back( OpConf("memref", "alloca_scope", rand(priority_base), {}));
    memrefOpConfs.push_back( OpConf("memref", "load", rand(priority_base), {"func.func"}));
    memrefOpConfs.push_back( OpConf("memref", "copy", rand(priority_base), {"func.func"}));
    memrefOpConfs.push_back(OpConf("memref", "cast", rand(priority_base), {"func.func"}));
  
  std::vector<OpConf> arithOpConfs;

  arithOpConfs.push_back(
      OpConf("arith", "addf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "addi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "andi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "ceildivsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "cmpf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "cmpi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "constant", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "divf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "divsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "divui", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "floordivsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "maxf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "maxsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "maxui", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "minf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "minsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "minui", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "mulf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "muli", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "negf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "ori", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "remf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "remsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "remui", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "shli", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "shrsi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "shrui", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "subf", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "subi", rand(priority_base), {"func.func"}));
  arithOpConfs.push_back(
      OpConf("arith", "xori", rand(priority_base), {"func.func"}));

  std::vector<OpConf> mathOpConfs;

  mathOpConfs.push_back(
      OpConf("math", "absf", rand(priority_base), {"func.func"}));
    mathOpConfs.push_back(
        OpConf("math", "absi", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "atan", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "atan2", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "ceil", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "copySign", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "cos", rand(priority_base), {"func.func"}));
    mathOpConfs.push_back(
        OpConf("math", "ctlz", rand(priority_base), {"func.func"}));
    mathOpConfs.push_back(
        OpConf("math", "cttz", rand(priority_base), {"func.func"}));
    mathOpConfs.push_back(
        OpConf("math", "ctpop", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "exp", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "exp2", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "expm1", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "floor", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "fma", rand(priority_base), {"func.func"}));
  //   mathOpConfs.push_back(
  //       OpConf("math", "ipowi", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "log", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "log10", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "log1p", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "log2", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "powf", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "rsqrt", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "sqrt", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "tan", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "tanh", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "roundeven", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "round", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "trunc", rand(priority_base), {"func.func"}));
  mathOpConfs.push_back(
      OpConf("math", "fpowi", rand(priority_base), {"func.func"}));

  std::vector<OpConf> scfOpConfs;
  scfOpConfs.push_back(OpConf("scf", "if", rand(priority_base), {"func.func"}));
  scfOpConfs.push_back(
      OpConf("scf", "execute_region", rand(priority_base), {}));
  scfOpConfs.push_back(OpConf("scf", "index_switch", rand(priority_base), {}));
  scfOpConfs.push_back(OpConf("scf", "while", rand(priority_base), {}));
  // scfOpConfs.push_back(OpConf("scf", "parallel", rand(priority_base), {}));

    std::vector<OpConf> affineOpConfs;
    affineOpConfs.push_back(OpConf("affine", "apply", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "for", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "if", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "store", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "load", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "min", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "max", rand(priority_base), {}));
  //   affineOpConfs.push_back(OpConf("affine", "parallel", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "vector_load", rand(priority_base), {}));
    affineOpConfs.push_back(OpConf("affine", "vector_store", rand(priority_base), {}));

    std::vector<OpConf> vectorOpConfs;
    vectorOpConfs.push_back(OpConf("vector", "bitcast", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "broadcast", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "contract", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "compress_store", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "create_mask", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "extract", rand(priority_base), {}));
    vectorOpConfs.push_back( OpConf("vector", "extract_strided_slice", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "gather", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "mask", rand(priority_base), {}));
    // vectorOpConfs.push_back( OpConf("vector", "transfer_read", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "transfer_write", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "warp_execute_on_lane0", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "fma", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "flat_transpose", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "insert_element", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "insert", rand(priority_base),{}));
    //  vectorOpConfs.push_back(OpConf("vector", "insert_strided_slice", rand(priority_base), {}));// TODO-bug
    vectorOpConfs.push_back(OpConf("vector", "load", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "masked_load", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "matrix_multiply", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "multi_reduction", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "outer_product", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "print", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "reduction", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "scan", rand(priority_base), {}));
    // vectorOpConfs.push_back(OpConf("vector", "scatter", rand(priority_base), {})); 
    vectorOpConfs.push_back(OpConf("vector", "shuffle", rand(priority_base), {}));
    vectorOpConfs.push_back(OpConf("vector", "splat", rand(priority_base), {})); 
    vectorOpConfs.push_back( OpConf("vector", "transpose", rand(priority_base), {}));
  
  std::vector<OpConf> tensorOpConfs;

  tensorOpConfs.push_back(OpConf("tensor", "cast", rand(priority_base), {}));
  tensorOpConfs.push_back(
      OpConf("tensor", "collapse_shape", rand(priority_base), {}));
    tensorOpConfs.push_back(OpConf("tensor", "dim", rand(priority_base),
    {}));
  tensorOpConfs.push_back(OpConf("tensor", "empty", rand(priority_base), {}));
  // tensorOpConfs.push_back(
  //     OpConf("tensor", "expand_shape", rand(priority_base), {}));
  tensorOpConfs.push_back(OpConf("tensor", "extract", rand(priority_base), {}));
  tensorOpConfs.push_back(
      OpConf("tensor", "from_elements", rand(priority_base), {}));
    // tensorOpConfs.push_back(OpConf("tensor", "generate", rand(priority_base), {}));
  tensorOpConfs.push_back(OpConf("tensor", "insert", rand(priority_base), {}));
  //   tensorOpConfs.push_back(OpConf("tensor", "pack", rand(priority_base),
  //   {}));
  tensorOpConfs.push_back(OpConf("tensor", "rank", rand(priority_base), {}));
  tensorOpConfs.push_back(OpConf("tensor", "splat", rand(priority_base), {}));
  //   tensorOpConfs.push_back(OpConf("tensor", "scatter", rand(priority_base),
  //   {})); tensorOpConfs.push_back(OpConf("tensor", "unpack",
  //   rand(priority_base), {}));

  std::vector<OpConf> indexOpConfs;
  indexOpConfs.push_back(OpConf("index", "add", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "and", rand(priority_base), {}));
  indexOpConfs.push_back(
      OpConf("index", "bool.constant", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "casts", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "castu", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "ceildivs", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "ceildivu", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "constant", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "divs", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "divu", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "floordivs", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "maxs", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "maxu", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "mul", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "or", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "rems", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "remu", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "shl", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "shrs", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "shru", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "sizeof", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "sub", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("index", "xor", rand(priority_base), {}));

  /*
  indexOpConfs.push_back(
      OpConf("bufferization", "alloc_tensor", rand(priority_base), {}));
  indexOpConfs.push_back(
      OpConf("bufferization", "clone", rand(priority_base), {}));
  indexOpConfs.push_back(
      OpConf("bufferization", "dealloc_tensor", rand(priority_base), {}));
  indexOpConfs.push_back(
      OpConf("bufferization", "to_memref", rand(priority_base), {}));
  indexOpConfs.push_back(OpConf("bufferization", "to_tensor",
rand(priority_base), {}));

  std::vector<OpConf> spirvOpConfs;
  spirvOpConfs.push_back(OpConf("spirv", "BitCount", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "BitFieldInsert", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "BitReverse", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "Not", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "BitwiseAnd", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "BitwiseOr", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "BitwiseXor", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.ceil", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.cos", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.erf", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.exp", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.fabs", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.floor", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.log", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.round", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.rint", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.rsqrt", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.sin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.sqrt", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.tanh", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.sabs", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.fmax", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.fmin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.pow", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.smax", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.smin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.umax", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "CL.umin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "FNegate", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "FOrdEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "FOrdLessThanEqual",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"FOrdLessThan", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "FOrdNotEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "FUnordEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "FUnordGreaterThanEqual",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"FUnordGreaterThan", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "FUnordLessThanEqual",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"FUnordLessThan", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "FUnordNotEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "IEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "INotEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "LogicalEqual",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv", "LogicalNot",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"LogicalNotEqual", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "LogicalAnd", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "LogicalOr", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "SGreaterThanEqual",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"SGreaterThan", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "SLessThanEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "SLessEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "UGreaterThanEqual",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"UGreaterThan", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "ULessThanEqual", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "ULessThan", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "Unordered", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Acos", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Asin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Atan", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Ceil", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Cos", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Cosh", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Exp", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.FAbs", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.FSign", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Floor", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.InverseSqrt",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv", "GL.Log",
rand(priority_base), {})); spirvOpConfs.push_back(OpConf("spirv",
"GL.RoundEven", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.Round", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.Sin", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.Sinh", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.Sqrt", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.Tan", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.Tanh", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.FClamp", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.SAbs", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.SSign", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.FMax", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.FMin", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.FMix", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "GL.FindUMsb", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Ldexp", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Pow", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.SClamp", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.SMax", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.SMin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.UMax", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.UMin", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.UClamp", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "IsInf", rand(priority_base), {}));
  spirvOpConfs.push_back(OpConf("spirv", "IsNan", rand(priority_base), {}));
//   spirvOpConfs.push_back(OpConf("spirv", "SNegate", rand(priority_base),
{})); spirvOpConfs.push_back(OpConf("spirv", "GL.Frexp", rand(priority_base),
{}));
*/
//   std::vector<OpConf> tosaOpConfs;
//   tosaOpConfs.push_back(OpConf("tosa", "abs", rand(priority_base), {}));
//   tosaOpConfs.push_back(OpConf("tosa", "add", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "applyScale", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "argmax", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "arithmetic_right_shift",rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "bitwise_and",
// // rand(priority_base), {})); tosaOpConfs.push_back(OpConf("tosa", "bitwise_not",
// // rand(priority_base), {})); tosaOpConfs.push_back(OpConf("tosa", "bitwise_or",
// // rand(priority_base), {})); tosaOpConfs.push_back(OpConf("tosa", "bitwise_xor",
// // rand(priority_base), {})); tosaOpConfs.push_back(OpConf("tosa", "cast",
// // rand(priority_base), {})); tosaOpConfs.push_back(OpConf("tosa", "ceil",
// // rand(priority_base), {}));
//   tosaOpConfs.push_back(OpConf("tosa", "identity",rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "log",rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "logical_and",rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa","logical_left_shift", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "logical_not", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "logical_or", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "logical_right_shift",rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "logical_xor",rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "max_pool2d",rand(priority_base), {}));
//   tosaOpConfs.push_back(OpConf("tosa", "maximum",
// rand(priority_base), {})); tosaOpConfs.push_back(OpConf("tosa", "minimum",
// rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "mul", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "pow", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "rfft2d", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "reciprocal", rand(priority_base), {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "reduce_all", rand(priority_base),
// // {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "reduce_any", rand(priority_base),
// // {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "reduce_max", rand(priority_base),
// // {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "reduce_min", rand(priority_base),
// // {}));
//   //tosaOpConfs.push_back(OpConf("tosa", "reduce_prod", rand(priority_base),
// // {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "reduce_sum", rand(priority_base),
// // {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "rescale", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "reshape", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "resize", rand(priority_base), {}));
// //  tosaOpConfs.push_back(OpConf("tosa", "reverse", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "rsqrt", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "scatter", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "select", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "sigmoid", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "slice", rand(priority_base), {}));
//   tosaOpConfs.push_back(OpConf("tosa", "sub", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "table", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "tanh", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "tile", rand(priority_base), {}));
// //   tosaOpConfs.push_back(OpConf("tosa", "transpose", rand(priority_base), {}));


  c.supported_ops.insert(c.supported_ops.end(), funcOpConfs.begin(),
                         funcOpConfs.end());
    c.supported_ops.insert(c.supported_ops.end(), linalgOpConfs.begin(),
                           linalgOpConfs.end());
    c.supported_ops.insert(c.supported_ops.end(), memrefOpConfs.begin(),
                           memrefOpConfs.end());
  
  c.supported_ops.insert(c.supported_ops.end(), arithOpConfs.begin(),
                         arithOpConfs.end());
  c.supported_ops.insert(c.supported_ops.end(), mathOpConfs.begin(),
                         mathOpConfs.end());
  c.supported_ops.insert(c.supported_ops.end(), scfOpConfs.begin(),
                         scfOpConfs.end());
    c.supported_ops.insert(c.supported_ops.end(), affineOpConfs.begin(),
                           affineOpConfs.end());
  c.supported_ops.insert(c.supported_ops.end(), vectorOpConfs.begin(),
                           vectorOpConfs.end());
  c.supported_ops.insert(c.supported_ops.end(), tensorOpConfs.begin(),
                         tensorOpConfs.end());
  c.supported_ops.insert(c.supported_ops.end(), indexOpConfs.begin(),
                         indexOpConfs.end()); 
//    c.supported_ops.insert(c.supported_ops.end(), spirvOpConfs.begin(),
//                           spirvOpConfs.end());
//    c.supported_ops.insert(c.supported_ops.end(), tosaOpConfs.begin(),
//                           tosaOpConfs.end());
 

  return c;
}

inline void initConfig(std::string fileName = "config.json") {
  //   for experiment use.
  //    std::cout << fileName + " is current config file.\n";
  //    std::ifstream f(fileName);
  //    json j = json::parse(f);
  //    conf = Conf(j);

  // for development use.
  conf = getConfigTemplate();
}

// Conf getConfig() { return conf; }

#endif
