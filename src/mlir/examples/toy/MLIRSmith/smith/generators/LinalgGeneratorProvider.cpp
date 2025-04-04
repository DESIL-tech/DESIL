//
// Created by Stan Wang on 2022/10/28.
//

#include "smith/RegionGeneration.h"
#include "smith/TypeGeneration.h"
#include "smith/ValueGeneration.h"

using namespace mlir;

// a work around, TODO: change the dependency to use arithOperators in
std::vector<OpGeneration> arithOperatorsF_wa = {
    OpGeneration("arith.addF", addFGenerator()),
    OpGeneration("arith.cmpF", cmpFGenerator()),
    OpGeneration("arith.divF", divFGenerator()),
    OpGeneration("arith.maxF", maxFGenerator()),
    OpGeneration("arith.minF", minFGenerator()),
    OpGeneration("arith.mulF", mulFGenerator()),
    OpGeneration("arith.negF", negFGenerator()),
    OpGeneration("arith.remF", remFGenerator()),
    OpGeneration("arith.subF", subFGenerator()),
    OpGeneration("arith.constant", constantGenerator())};

std::vector<OpGeneration> arithOperatorsI_wa = {
    OpGeneration("arith.addI", addIGenerator()),
    OpGeneration("arith.andI", andIGenerator()),
    OpGeneration("arith.ceilDivSI", ceilDivSIGenerator()),
    OpGeneration("arith.ceilDivUI", ceilDivUIGenerator()),
    OpGeneration("arith.cmpI", cmpIGenerator()),
    OpGeneration("arith.constant", constantGenerator()),
    OpGeneration("arith.divSI", divSIGenerator()),
    OpGeneration("arith.divSI", divUIGenerator()),
    OpGeneration("arith.floorDivSI", floorDivSIGenerator()),
    OpGeneration("arith.maxUI", maxUIGenerator()),
    OpGeneration("arith.maxSI", maxSIGenerator()),
    OpGeneration("arith.minUI", minUIGenerator()),
    OpGeneration("arith.minSI", minSIGenerator()),
    OpGeneration("arith.mulI", mulIGenerator()),
    OpGeneration("arith.remSI", remSIGenerator()),
    OpGeneration("arith.remUI", remUIGenerator()),
    OpGeneration("arith.shlI", shlIGenerator()),
    OpGeneration("arith.shrSI", shrSIGenerator()),
    OpGeneration("arith.shrUI", shrUIGenerator()),
    OpGeneration("arith.subI", subIGenerator()),
    OpGeneration("arith.xorI", xorIGenerator())};

OpGenerator linalgGenericGenerator() {

  return [](OpBuilder &builder, Location loc, OpRegion &parent) {
    auto typedValuePool = parent.pool;
    int input_num = UR(linalg_operands_num_ub)+1;

    auto elemTy = randomIntOrFloatType(builder.getContext());

    // randomly create a indexingMap
    SmallVector<AffineMap> indexingMaps;
    SmallVector<Type> argsType;
    Type initTy;

    auto outputRank = UR(rank_ub) + 1;
    // dimMap maps from affineDimExpr(ith) to it dim size
    SmallVector<int64_t> dimMap;
    SmallVector<utils::IteratorType> iteratorTypes;
    SmallVector<Type> operandTys;
    for (int i = 0; i < outputRank; ++i) {
      if (UR(2)) {
        dimMap.push_back(dimPool[UR(dimPool.size())]);
      } else {
        dimMap.push_back(ShapedType::kDynamic);
      }
      if (UR(2)) {
        iteratorTypes.push_back(utils::IteratorType::parallel);
      } else {
        iteratorTypes.push_back(utils::IteratorType::reduction);
      }
    }
    // iterator types cannot be all reduction.
    iteratorTypes[UR(iteratorTypes.size())] = utils::IteratorType::parallel;
    // indexing maps and its corresponding arg Type,(i.e.,operands and init).

    SmallVector<AffineExpr> affineExprs0;
    SmallVector<int64_t> shape0;
    for (int j = 0; j < outputRank; ++j) {
        affineExprs0.push_back(builder.getAffineDimExpr(j));
        shape0.push_back(dimMap[j]);
    }
    indexingMaps.push_back(AffineMap::get(/*dimCount=*/outputRank,
                                          /*symbolCount=*/0, affineExprs0,
                                          builder.getContext()));
    bool optype = 0; // 0: tensor, 1: memref
    if (UR(2)) {
      operandTys.push_back(RankedTensorType::get(shape0, elemTy));
      optype = 0;
    } else {
      operandTys.push_back(MemRefType::get(shape0, elemTy));
      optype = 1;
    }

    for (int i = 1; i < input_num; i++) {
      SmallVector<AffineExpr> affineExprs;
      SmallVector<int64_t> shape;
      for (int j = 0; j < outputRank; ++j) {
        // if (UR(2)) {
          affineExprs.push_back(builder.getAffineDimExpr(j));
          shape.push_back(dimMap[j]);
        // }
      }
      indexingMaps.push_back(AffineMap::get(/*dimCount=*/outputRank,
                                            /*symbolCount=*/0, affineExprs,
                                            builder.getContext()));
      if (!optype) {
        operandTys.push_back(RankedTensorType::get(shape, elemTy));
      } else {
        operandTys.push_back(MemRefType::get(shape, elemTy));
      }
    }
    SmallVector<AffineExpr> fullRankExprs;
    SmallVector<int64_t> initShape;
    for (int i = 0; i < outputRank; ++i) {
      if (iteratorTypes[i] == utils::IteratorType::parallel) {
        fullRankExprs.push_back(builder.getAffineDimExpr(i));
        initShape.push_back(dimMap[i]);
      }
    }
    if (!optype) {
      initTy = RankedTensorType::get(initShape, elemTy);
    } else {
      initTy = MemRefType::get(initShape, elemTy);
    }
    // initTy = RankedTensorType::get(initShape, elemTy);
    indexingMaps.push_back(
        AffineMap::get(outputRank, 0, fullRankExprs, builder.getContext()));


    std::vector<PoolType> candidatesTypePool;
    if (!optype) {
      candidatesTypePool.push_back(PoolType::StaticShapedTensor);
      candidatesTypePool.push_back(PoolType::DynamicShapedTensor);
    } else {
      candidatesTypePool.push_back(PoolType::StaticShapedMemref);
      candidatesTypePool.push_back(PoolType::DynamicShapedMemref);
    }
    SmallVector<Value> operands;
    for (auto operandTy : operandTys) {
      auto candidates = parent.pool.searchCandidatesFrom(
          candidatesTypePool, typeEquivalentFilter(operandTy));
      if (candidates.empty()) {
        candidates.push_back(
            parent.pool.generateTypedValue(builder, loc, operandTy));
      }
      operands.push_back(
          sampleTypedValueFrom(candidates, "linalg.generic").val);
    }
    auto initCandidates = parent.pool.searchCandidatesFrom(
        candidatesTypePool, typeEquivalentFilter(initTy));
    if (initCandidates.empty()) {
      initCandidates.push_back(
          parent.pool.generateTypedValue(builder, loc, initTy));
    }
    auto init = sampleTypedValueFrom(initCandidates, "linalg.generic").val;


    TypeRange restTyperange;
    if(!optype)
      restTyperange = TypeRange(initTy);
    else
      restTyperange = TypeRange();
    auto op = builder.create<linalg::GenericOp>(
        loc,
        /*resultTensorTypes=*/restTyperange,
        /*inputs=*/ValueRange(operands),
        /*outputs=*/ValueRange(init),
        /*indexingMaps=*/indexingMaps,
        /*iteratorTypes=*/iteratorTypes,
        /*bodyBuilder=*/
        [&](OpBuilder &b, Location loc, ValueRange args) {
          auto region = OpRegion("linalg.generic", parent.depth + 1);
          // auto region = OpRegion("linalg.generic", 3);// change to 3 due to error "error: 'linalg.map' op expects region #0 to have 0 or 1 blocks"
    
          for (auto it = args.begin(); it < args.end(); ++it) {
            auto tVal = TypeValue(elemTy, *it);
            region.pool.addElement(tVal, "linalg.generic(arg)");
          }
          region.pool.merge(parent.pool);
          RegionGen gen(&region, {OpNameFilter(opsForLinalgGeneric)});
          // gen.apply(builder, loc, 0);
          gen.apply(builder, loc, 10); //test: to change the operations generated in the child block to 10
          std::vector<TypeValue> yieldCandidates;
          for (auto elem : region.pool.intOrFloatPool) {
            if (elem.type == elemTy) {
              yieldCandidates.push_back(elem);
            }
          }
          auto value2Yield =
              sampleTypedValueFrom(yieldCandidates, "linalg.yield");
          b.create<linalg::YieldOp>(loc, value2Yield.val);
        });
    // for (auto res : op->getResults()) {
    //   auto resV = TypeValue(initTy, res);
    //   typedValuePool.addTypeValue(resV, "linalg.generic");
    // }
    if(!optype) {
        
      auto tval = TypeValue(initTy, op->getResult(0));
      parent.pool.addStaticShapedTensor(tval, "linalg.generic");
    }

    return op.getOperation();
  };
}

OpGenerator linalgBroadCastGenerator() {
  return [](OpBuilder &builder, Location loc, OpRegion &region) {
    auto inputCandidates = region.pool.searchCandidatesFrom(
        {PoolType::StaticShapedTensor, PoolType::DynamicShapedTensor,  
         PoolType::StaticShapedMemref, PoolType::DynamicShapedMemref},
        emptyFilter());
    if (inputCandidates.empty()) {
      Type t;
      if (UR(2)) {
        t = randomRankedTensorType(builder.getContext()); 
        // t = randomStaticShapedTensorType(builder.getContext()); 
      } else {
        t = randomRankedMemrefType(builder.getContext());
      }
      inputCandidates.push_back(
          region.pool.generateTypedValue(builder, loc, t));
    }
    auto operand = sampleTypedValueFrom(inputCandidates, "linalg.broadcast");
    auto srcShapeTy = operand.type.dyn_cast<ShapedType>();
    // std::cout << srcShapeTy.getRank() << ": src\n";
    SmallVector<int64_t> initShape(srcShapeTy.getShape());
    initShape.push_back(dimPool[UR(dimPool.size())]);

    SmallVector<int64_t> dimension = {
        operand.type.dyn_cast<ShapedType>().getRank()};
    Type initTy;
    if (operand.type.isa<MemRefType>()) {
      initTy = MemRefType::get(
          initShape, operand.type.dyn_cast<ShapedType>().getElementType());
    } else {
      initTy = RankedTensorType::get(
          initShape, operand.type.dyn_cast<ShapedType>().getElementType());
    }
    // std::cout << initTy.dyn_cast<ShapedType>().getRank() << ": dest\n";
    std::vector<PoolType> destCandidatesTypePool ;
    if (operand.type.isa<MemRefType>()) {
      destCandidatesTypePool.push_back(PoolType::StaticShapedMemref);
      destCandidatesTypePool.push_back(PoolType::DynamicShapedMemref);
    } else {
      destCandidatesTypePool.push_back(PoolType::StaticShapedTensor);
      destCandidatesTypePool.push_back(PoolType::DynamicShapedTensor);
    }
    // auto initCandidates = region.pool.searchCandidatesFrom(
    //     {PoolType::StaticShapedTensor, PoolType::DynamicShapedTensor, // due to shape inconsistent, temporarily delated
    //      /*PoolType::StaticShapedMemref, PoolType::DynamicShapedMemref*/},
    //     typeEquivalentFilter(initTy));
    // llvm::outs() <<"linalg.broadcast searching candidates\n";
    auto initCandidates = region.pool.searchCandidatesFrom(
        destCandidatesTypePool, typeEquivalentFilter(initTy));
    // for (int i=0 ; i< initCandidates.size(); i++)
    //   llvm::outs() <<initCandidates[i].type<<" ";
    // llvm::outs()<<"\n";
    if (initCandidates.empty()) {
      if (initTy.isa<MemRefType>() &&
          !initTy.dyn_cast<ShapedType>().hasStaticShape()) {
        initCandidates.push_back(region.pool.generateDynamicShapedMemref(
            builder, loc, initTy.dyn_cast<MemRefType>()));
      } else {
        // llvm::outs() << initTy << " initTy candidate empty\n";
        initCandidates.push_back(
            region.pool.generateTypedValue(builder, loc, initTy));
      }
    }
    auto init = sampleTypedValueFrom(initCandidates, "linalg.broadcast");
    // llvm::outs() <<operand.type <<" " <<init.type <<"\n";

    SmallVector<NamedAttribute> attrs;
    // llvm::outs() << operand.val <<" " <<init.val <<"\n";
    auto op = builder.create<linalg::BroadcastOp>(loc, operand.val, init.val,
                                                  dimension, attrs);
    if (!op.getResult().empty()) {
      for (auto res : op->getResults()) {
        region.pool.addTypeValue(TypeValue(initTy, res), "linalg.broadcast");
      }
    }
    return op.getOperation();
  };
}

OpGenerator linalgTransposeGenerator() {
  return [](OpBuilder &builder, Location loc, OpRegion &parent) {
    // For this operation, permutation is not allowed to be empty, therefore,
    auto fromCandidates = parent.pool.searchCandidatesFrom(
        {PoolType::DynamicShapedTensor, PoolType::StaticShapedTensor, // due to shape inconsistent, temporarily delated
         PoolType::StaticShapedMemref, PoolType::DynamicShapedMemref},
        [](TypeValue tval) {
          return tval.type.dyn_cast<ShapedType>().getRank() > 0;
        });
    if (fromCandidates.empty()) {
      Type t;
      if (UR(2)) {
        t = randomRankedTensorType(builder.getContext()); // due to shape inconsistent, temporarily delated
        // t = randomStaticShapedTensorType(builder.getContext());
      } else {
        t = randomRankedMemrefType(builder.getContext());
      }
      fromCandidates.push_back(parent.pool.generateTypedValue(builder, loc, t));
    }
    auto from = sampleTypedValueFrom(fromCandidates, "linalg.transpose");
    SmallVector<int64_t> permutation;
    auto shapeTy = from.type.dyn_cast<ShapedType>();
    for (int i = 0; i < shapeTy.getRank(); ++i) {
      permutation.push_back(i);
    }

    struct timespec ts;
    std::shuffle(permutation.begin(), permutation.end(),
                 std::default_random_engine((time_t)ts.tv_nsec));

    SmallVector<int64_t> initShape;
    for (int i = 0; i < shapeTy.getRank(); ++i) {
      auto idx = permutation[i];
      initShape.push_back(shapeTy.getDimSize(idx));
    }
    Type initTy;
    if (from.type.isa<MemRefType>()) {
      initTy = MemRefType::get(initShape, shapeTy.getElementType());
    } else {
      initTy = RankedTensorType::get(initShape, shapeTy.getElementType());
    }
    std::vector<PoolType> initCandidatesTypePool ;
    if (from.type.isa<MemRefType>()) {
      initCandidatesTypePool.push_back(PoolType::StaticShapedMemref);
      initCandidatesTypePool.push_back(PoolType::DynamicShapedMemref);
    } else {
      initCandidatesTypePool.push_back(PoolType::StaticShapedTensor);
      initCandidatesTypePool.push_back(PoolType::DynamicShapedTensor);
    }
    auto initCandidates = parent.pool.searchCandidatesFrom(
        // {/*PoolType::DynamicShapedTensor,*/ PoolType::StaticShapedTensor,// due to shape inconsistent, temporarily delated
        //  /*PoolType::StaticShapedMemref, PoolType::DynamicShapedMemref*/},
        initCandidatesTypePool, typeEquivalentFilter(initTy));
    if (initCandidates.empty()) {
      if (initTy.isa<MemRefType>() &&
          !initTy.dyn_cast<ShapedType>().hasStaticShape()) {
        initCandidates.push_back(parent.pool.generateDynamicShapedMemref(
            builder, loc, initTy.dyn_cast<MemRefType>()));
      } else {

        initCandidates.push_back(
            parent.pool.generateTypedValue(builder, loc, initTy));
      }
    }
    auto init = sampleTypedValueFrom(initCandidates, "linalg.transpose");
    assert(!permutation.empty());
    SmallVector<NamedAttribute> attributes;
    auto op = builder.create<linalg::TransposeOp>(loc, from.val, init.val,
                                                  permutation, attributes);
    if (!op->getResults().empty()) {
      for (auto res : op->getResults()) {
        parent.pool.addTypeValue(TypeValue(initTy, res), "linalg.transpose");
      }
    }
    return op.getOperation();
  };
}

//'linalg.copy' op result #0 must be ranked tensor
OpGenerator linalgCopyGenerator() {
  return [](OpBuilder &builder, Location loc, OpRegion &parent) {
    auto tensorCandidates = parent.pool.searchCandidatesFrom(
        {PoolType::StaticShapedTensor, PoolType::DynamicShapedTensor, 
         PoolType::StaticShapedMemref, PoolType::DynamicShapedMemref}, // due to shape inconsistent, temporarily delated
        emptyFilter());
    if (tensorCandidates.empty()) {
      Type t;
      if (UR(2)) {
        t = randomRankedTensorType(builder.getContext()); // due to shape inconsistent, temporarily delated
        // t = randomStaticShapedTensorType(builder.getContext());
      } else {
        t = randomRankedMemrefType(builder.getContext());
      }
      tensorCandidates.push_back(parent.pool.generateTypedValue(builder, loc, t));
        // tensorCandidates.push_back(parent.pool.generateStaticShapedTensor(
        //   builder, loc, randomRankedTensorType(builder.getContext())));
    }
    auto in = sampleTypedValueFrom(tensorCandidates, "linalg.copy");

    std::vector<PoolType> initCandidatesTypePool ;
    if (in.type.isa<MemRefType>()) {
      initCandidatesTypePool.push_back(PoolType::StaticShapedMemref);
      initCandidatesTypePool.push_back(PoolType::DynamicShapedMemref);
    } else {
      initCandidatesTypePool.push_back(PoolType::StaticShapedTensor);
      initCandidatesTypePool.push_back(PoolType::DynamicShapedTensor);
    }
    auto outCandidates = parent.pool.searchCandidatesFrom(
        initCandidatesTypePool, // due to shape inconsistent, temporarily delated
        typeEquivalentFilter(in.type));
    if (outCandidates.empty()) {
      outCandidates.push_back(
          parent.pool.generateTypedValue(builder, loc, in.type));
    }
    auto out = sampleTypedValueFrom(outCandidates, "linalg.copy");

    SmallVector<NamedAttribute> attributes;

    mlir::linalg::CopyOp op;
    if (in.type.isa<MemRefType>()) 
      op = builder.create<linalg::CopyOp>(
        loc, TypeRange(), ValueRange({in.val}),
        ValueRange({out.val}), attributes);
    else {
      op = builder.create<linalg::CopyOp>(
        loc, TypeRange({out.val.getType()}), ValueRange({in.val}),
        ValueRange({out.val}), attributes);
      parent.pool.addTypeValue(TypeValue(in.type, op.getResult(0)),
                             "linalg.copy");
    }
    return op.getOperation();
  };
}

// 'linalg.map' op result #0 must be tensor of any type values
OpGenerator linalgMapGenerator() {

  return [](OpBuilder &builder, Location loc, OpRegion &parent) {
    auto typedValuePool = parent.pool;
    auto point = builder.saveInsertionPoint();

    int input_num = UR(linalg_operands_num_ub - 1) + 1;
    std::vector<Value> ins;
    std::vector<PoolType> candidatesTypePool;
    if (UR(2)) {
      candidatesTypePool.push_back(PoolType::StaticShapedTensor);
      candidatesTypePool.push_back(PoolType::DynamicShapedTensor);
    } else {
      candidatesTypePool.push_back(PoolType::StaticShapedMemref);
      candidatesTypePool.push_back(PoolType::DynamicShapedMemref);
    }
    auto candidates = parent.pool.searchCandidatesFrom(
        candidatesTypePool, 
        emptyFilter());
    if (candidates.empty()) {
      Type t;
      if (UR(2)) {
        t = randomRankedTensorType(builder.getContext()); 
        // t = randomStaticShapedTensorType(builder.getContext());
      } else {
        t = randomRankedMemrefType(builder.getContext()); 
      }
      candidates.push_back(parent.pool.generateTypedValue(builder, loc, t));
    }
    auto operand0 = sampleTypedValueFrom(candidates, "linalg.map");
    auto shape = operand0.type.dyn_cast<ShapedType>();
    ins.push_back(operand0.val);

    SmallVector<mlir::Type> types;
    // input operands
    for (int i = 1; i < input_num; ++i) {
      auto operandi = sampleTypedValueFrom(
          searchShapedInputFrom(shape, candidates), "linalg.map");
      ins.push_back(operandi.val);
      types.push_back(shape.getElementType());
    }
    types.push_back(shape.getElementType());

    auto locs = SmallVector<Location>(input_num, loc);

    Type initType;
    if(operand0.type.isa<RankedTensorType>())
      initType = RankedTensorType::get(shape.getShape(), shape.getElementType());
    else
      initType = MemRefType::get(shape.getShape(), shape.getElementType()); 

    auto init = parent.pool.generateTypedValue(builder, loc, initType);


    mlir::linalg::MapOp val ;
    if(operand0.type.isa<RankedTensorType>())
      val = builder.create<linalg::MapOp>(loc, TypeRange(initType),
                                             ValueRange(ins), init.val);
    else
      val = builder.create<linalg::MapOp>(loc, TypeRange(),
                                             ValueRange(ins), init.val);
    Block *block = builder.createBlock(&val.getMapper());
    block->addArguments(TypeRange(types), locs);

    OpRegion region("linalg.map", parent.depth + 1); 
    // OpRegion region("linalg.map", 3); // change to 3 due to error "error: 'linalg.map' op expects region #0 to have 0 or 1 blocks"
    for (auto arg : block->getArguments()) {
      region.pool.addElement(TypeValue(shape.getElementType(), arg),
                             "arg(linalg.map)");
    }
    region.pool.merge(parent.pool);
    RegionGen gen(&region, {OpNameFilter(opsForLinalgMap)});
    gen.apply(builder, loc, 32);

    builder.setInsertionPointToEnd(block);

    Type tmpshapeTy = shape.getElementType();
    auto yieldCandidates = region.pool.searchCandidatesFrom(
        {PoolType::IntOrFloat}, typeEquivalentFilter(tmpshapeTy));
    if (yieldCandidates.empty()) {
      yieldCandidates.push_back(
          region.pool.generateElement(builder, loc, shape.getElementType()));
    }
    auto yield = sampleTypedValueFrom(yieldCandidates, "linalg.yield");
    builder.create<linalg::YieldOp>(loc, ValueRange({yield.val}));
    if(operand0.type.isa<RankedTensorType>()) {
        
      auto tval = TypeValue(initType, val->getResult(0));
      region.pool.addStaticShapedTensor(tval, "linalg.map");
    }

    builder.restoreInsertionPoint(point);
    return val.getOperation();
  };
}

OpGenerator linalgReduceGenerator() {
  return [](OpBuilder &builder, Location loc, OpRegion &parent) {
    auto typedValuePool = parent.pool;
    // reduce shapedType that dimension > 2.
    auto inputCandidates = typedValuePool.searchCandidatesFrom(
        {PoolType::StaticShapedTensor, PoolType::StaticShapedMemref,
         PoolType::DynamicShapedTensor, PoolType::DynamicShapedMemref},
        [&](TypeValue typeValue) {
          return typeValue.type.dyn_cast<ShapedType>().getRank() > 0;
        });
    if (inputCandidates.empty()) {
      auto rank = UR(rank_ub) + 1; // do not generate 0-rank tensor
      SmallVector<int64_t> shape;
      for (int i = 0; i < rank; ++i) {
        shape.push_back(dimPool[UR(dimPool.size())]);
      }
      auto shapedTy = RankedTensorType::get(
                          shape, randomIntOrFloatType(builder.getContext()))
                          .dyn_cast<ShapedType>();
      Type ty = randomMemrefOrRankedTensorType(shapedTy);
      inputCandidates.push_back(
          parent.pool.generateTypedValue(builder, loc, ty));
    }
    auto operand = sampleTypedValueFrom(inputCandidates, "linalg.reduce");
    auto shapedType = operand.type.dyn_cast<ShapedType>();
    auto rank = shapedType.getRank();
    assert(rank > 0);
    SmallVector<int64_t> dimensions;

    for (int i = 0; i < rank; ++i) {
      // each dim has 1/2 prob to be reduced.
      if (!UR(2)) {
        dimensions.push_back(i);
      }
    }
    // make sure that at least one dim is reduced.
    if (dimensions.empty()) {
      dimensions.push_back(UR(rank));
    }

    SmallVector<int64_t> reducedShape;
    int i = 0;
    int j = 0;
    for (; i < rank; ++i) {
      if (j < dimensions.size() && i == dimensions[j]) {
        j++;
        continue;
      }
      reducedShape.push_back(shapedType.getDimSize(i));
    }

    Type initTy;
    if (UR(2)) {
      initTy = RankedTensorType::get(reducedShape, shapedType.getElementType());
    } else {
      initTy = MemRefType::get(reducedShape, shapedType.getElementType());
    }
    auto initCandidates = parent.pool.searchCandidatesFrom(
        {PoolType::StaticShapedMemref, PoolType::StaticShapedTensor,
         PoolType::DynamicShapedMemref, PoolType::DynamicShapedTensor},
        typeEquivalentFilter(initTy));
    if (initCandidates.empty()) {
      initCandidates.push_back(
          parent.pool.generateTypedValue(builder, loc, initTy));
    }
    auto init = sampleTypedValueFrom(initCandidates, "linalg.reduce");

    SmallVector<NamedAttribute> attributes;
    auto op = builder.create<linalg::ReduceOp>(
        loc, ValueRange(operand.val), ValueRange(init.val), dimensions,
        [&](OpBuilder &b, Location l, ValueRange args) {
          OpRegion region("", parent.depth + 1);
          for (auto arg : args) {
            region.pool.addTypeValue(TypeValue(arg.getType(), arg),
                                     "arg(linalg.reduce");
          }
          region.pool.merge(parent.pool);
          auto filter = OpNameFilter(opsForLinalgReduce);
          RegionGen gen(&region, {filter});
          gen.apply(b, l, 8);
          assert(shapedType.getElementType().isIntOrFloat());
          Type tmpshapeTy = shapedType.getElementType();
          auto yieldCandidates = region.pool.searchCandidatesFrom(
              {PoolType::IntOrFloat},
              typeEquivalentFilter(tmpshapeTy));
          if (yieldCandidates.empty()) {
            yieldCandidates.push_back(region.pool.generateTypedValue(
                builder, loc, shapedType.getElementType()));
          }
          auto yield = sampleTypedValueFrom(yieldCandidates, "linalg.yield");
          builder.create<linalg::YieldOp>(loc, yield.val);
        },
        attributes);

    if (!op->getResults().empty()) {
      for (auto res : op->getResults()) {
        parent.pool.addTypeValue(TypeValue(initTy, res), "linalg.broadcast");
      }
    }

    return op.getOperation();
  };
}

OpGenerator linalgDotGenerator() {
  return [](OpBuilder &builder, Location loc, OpRegion &region) {
    auto operand0Candidates = region.pool.searchCandidatesFrom(
        {PoolType::StaticShapedTensor/*, PoolType::StaticShapedMemref*/},
        [&](TypeValue tval) {
          return tval.type.dyn_cast<ShapedType>().getRank() == 1;
        });
    if (operand0Candidates.empty()) {
      SmallVector<int64_t> shape;
      shape.push_back(dimPool[UR(dimPool.size())]);
      auto shapeTy = RankedTensorType::get(
                         shape, randomIntOrFloatType(builder.getContext()))
                         .dyn_cast<ShapedType>();
      // operand0Candidates.push_back(region.pool.generateTypedValue(
      //     builder, loc, randomMemrefOrRankedTensorType(shapeTy)));
      operand0Candidates.push_back(region.pool.generateTypedValue(
          builder, loc, shapeTy));
    }
    auto operand0 = sampleTypedValueFrom(operand0Candidates, "linalg.dot");

    // auto operand1Ty =
    //     randomMemrefOrRankedTensorType(operand0.type.dyn_cast<ShapedType>());
    auto operand1Ty =
        operand0.type;
    auto operand1Candidates = region.pool.searchCandidatesFrom(
        {PoolType::StaticShapedTensor/*, PoolType::StaticShapedMemref*/},
        typeEquivalentFilter(operand1Ty));
    if (operand1Candidates.empty()) {
      operand1Candidates.push_back(
          region.pool.generateTypedValue(builder, loc, operand1Ty));
    }
    auto operand1 = sampleTypedValueFrom(operand1Candidates, "linalg.dot");

    auto resTy = RankedTensorType::get(
        SmallVector<int64_t>(),
        operand0.type.dyn_cast<ShapedType>().getElementType());
    auto resCandidates = region.pool.searchCandidatesFrom(
        {PoolType::StaticShapedTensor}, typeEquivalentFilter(resTy));
    if (resCandidates.empty()) {
      resCandidates.push_back(
          region.pool.generateTypedValue(builder, loc, resTy));
      // resCandidates.push_back(
      //     region.pool.generateTypedValue(builder, loc, operand0.type));
    }
    auto res = sampleTypedValueFrom(resCandidates, "linalg.dot");

    auto op = builder.create<linalg::DotOp>(
        loc, ValueRange({operand0.val, operand1.val}), ValueRange({res.val}),
        SmallVector<NamedAttribute>());
    for (auto res : op->getResults()) {
      region.pool.addTypeValue(TypeValue(res.getType(), res), "linalg.dot");
    }
    return op.getOperation();
  };
}

OpGenerator linalgMatMulGenerator() {

  return [](OpBuilder &builder, Location loc, OpRegion &region) {
    auto typedValuePool = region.pool;

    auto operand1Candidate = region.pool.searchCandidatesFrom(
        {PoolType::DynamicShapedTensor, PoolType::StaticShapedTensor, // due to shape inconsistent, temporarily delated
         PoolType::StaticShapedMemref, PoolType::DynamicShapedMemref},
        [&](TypeValue tval) {
          auto shapeTy = tval.type.dyn_cast<ShapedType>();
          return shapeTy.getRank() == 2 && !shapeTy.isDynamicDim(1);
        });
    if (operand1Candidate.empty()) {
      auto elemTy = randomIntOrFloatType(builder.getContext());
      SmallVector<int64_t> shape;
      shape.push_back(dimPool[UR(dimPool.size())]);
      shape.push_back(dimPool[UR(dimPool.size())]);
      Type type;
      if (UR(2)) {
        type = RankedTensorType::get(shape, elemTy);
      } else {
        type = MemRefType::get(shape, elemTy);
      }
      auto tVal = typedValuePool.generateTypedValue(builder, loc, type);
      operand1Candidate.push_back(tVal);
    }
    auto operand1 = sampleTypedValueFrom(operand1Candidate, "linalg.matmul");
    auto shapedType1 = operand1.type.dyn_cast<ShapedType>();
    auto elemTy = shapedType1.getElementType();
    auto x = shapedType1.getDimSize(0);
    std::vector<PoolType> op2initCandidatesTypePool ;
    if (operand1.type.isa<MemRefType>()) {
      op2initCandidatesTypePool.push_back(PoolType::StaticShapedMemref);
      op2initCandidatesTypePool.push_back(PoolType::DynamicShapedMemref);
    } else {
      op2initCandidatesTypePool.push_back(PoolType::StaticShapedTensor);
      op2initCandidatesTypePool.push_back(PoolType::DynamicShapedTensor);
    }
    auto operand2Candidates = region.pool.searchCandidatesFrom(
        op2initCandidatesTypePool,
        [&](TypeValue tval) {
          auto ty = tval.type.dyn_cast<ShapedType>();
          return ty.getElementType() == elemTy && ty.getRank() == 2 &&
                 ty.getDimSize(0) == shapedType1.getDimSize(1);
        });
    if (operand2Candidates.empty()) {
      SmallVector<int64_t> shape;
      shape.push_back(shapedType1.getDimSize(1));
      shape.push_back(dimPool[UR(dimPool.size())]);
      Type type;
      if (operand1.type.isa<MemRefType>()) 
        type = MemRefType::get(shape, elemTy);
      else 
        type = RankedTensorType::get(shape, elemTy);
      auto tVal = typedValuePool.generateTypedValue(builder, loc, type);
      operand2Candidates.push_back(tVal);
    }
    auto operand2 = sampleTypedValueFrom(operand2Candidates, "linalg.matmul");
    auto shapedType2 = operand2.type.dyn_cast<ShapedType>();
    auto y = shapedType2.getDimSize(1);
    Type resTensorType;
    if (operand1.type.isa<MemRefType>()) 
      resTensorType = MemRefType::get({x, y}, elemTy);
    else
      resTensorType = RankedTensorType::get({x, y}, elemTy);

    auto initCandidates = region.pool.searchCandidatesFrom(
        op2initCandidatesTypePool,// due to shape inconsistent, temporarily delated
        [&](TypeValue tval) {
          auto shapeTy = tval.type.dyn_cast<ShapedType>();
          return shapeTy.getRank() == 2 && shapeTy.getElementType() == elemTy &&
                 shapeTy.getDimSize(0) == x && shapeTy.getDimSize(1) == y;
        });
    if (initCandidates.empty()) {
      // auto initTy = RankedTensorType::get({x, y}, elemTy);
      // initCandidates.push_back(
      //     region.pool.generateRankedTensor(builder, loc, initTy));
      initCandidates.push_back(
          region.pool.generateTypedValue(builder, loc, resTensorType));
    }
    auto init = sampleTypedValueFrom(initCandidates, "linalg.matmul");
    auto resTy = init.type;
    mlir::linalg::MatmulOp op ;
    if (operand1.type.isa<MemRefType>())
      op = builder.create<linalg::MatmulOp>(loc, TypeRange({}),
                                      ValueRange({operand1.val, operand2.val}),
                                      ValueRange{init.val});
    else
      op = builder.create<linalg::MatmulOp>(loc, TypeRange({resTy}),
                                      ValueRange({operand1.val, operand2.val}),
                                      ValueRange{init.val});
            // .getResult(0);
    // auto tval = TypeValue(resTensorType, res);
    // typedValuePool.addStaticShapedTensor(tval, "linalg.matmul");
    if (!op->getResults().empty()) {
      for (auto res : op->getResults()) {
        region.pool.addTypeValue(TypeValue(resTensorType, res), "linalg.matmul");
      }
    }
    // return res.getDefiningOp();
    return op.getOperation();
  };
}
