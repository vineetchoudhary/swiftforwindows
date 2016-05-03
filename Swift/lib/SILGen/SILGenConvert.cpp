//===--- SILGenConvert.cpp - Type Conversion Routines ---------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2016 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "SILGen.h"
#include "Scope.h"
#include "swift/AST/ASTContext.h"
#include "swift/AST/Decl.h"
#include "swift/AST/Types.h"
#include "swift/Basic/Fallthrough.h"
#include "swift/Basic/type_traits.h"
#include "swift/SIL/SILArgument.h"
#include "swift/SIL/TypeLowering.h"
#include "Initialization.h"
#include "LValue.h"
#include "RValue.h"
#include "ArgumentSource.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SaveAndRestore.h"

using namespace swift;
using namespace Lowering;

// FIXME: With some changes to their callers, all of the below functions
// could be re-worked to use emitInjectEnum().
ManagedValue
SILGenFunction::emitInjectOptional(SILLocation loc,
                                   ManagedValue v,
                                   CanType inputFormalType,
                                   CanType substFormalType,
                                   const TypeLowering &expectedTL,
                                   SGFContext ctxt) {
  // Optional's payload is currently maximally abstracted. FIXME: Eventually
  // it shouldn't be.
  auto opaque = AbstractionPattern::getOpaque();

  OptionalTypeKind substOTK;
  auto substObjectType = substFormalType.getAnyOptionalObjectType(substOTK);

  auto loweredTy = getLoweredType(opaque, substObjectType);
  if (v.getType() != loweredTy)
    v = emitTransformedValue(loc, v,
                             AbstractionPattern(inputFormalType), inputFormalType,
                             opaque, substObjectType);

  auto someDecl = getASTContext().getOptionalSomeDecl(substOTK);
  SILType optTy = getLoweredType(substFormalType);
  if (v.getType().isAddress()) {
    auto buf = getBufferForExprResult(loc, optTy.getObjectType(), ctxt);
    auto payload = B.createInitEnumDataAddr(loc, buf, someDecl,
                                            v.getType());
    // FIXME: Is it correct to use IsTake here even if v doesn't have a cleanup?
    B.createCopyAddr(loc, v.forward(*this), payload,
                     IsTake, IsInitialization);
    B.createInjectEnumAddr(loc, buf, someDecl);
    v = manageBufferForExprResult(buf, expectedTL, ctxt);
  } else {
    auto some = B.createEnum(loc, v.getValue(), someDecl, optTy);
    v = ManagedValue(some, v.getCleanup());
  }

  return v;
}

void SILGenFunction::emitInjectOptionalValueInto(SILLocation loc,
                                                 ArgumentSource &&value,
                                                 SILValue dest,
                                                 const TypeLowering &optTL) {
  SILType optType = optTL.getLoweredType();
  OptionalTypeKind optionalKind;
  auto loweredPayloadTy
    = optType.getAnyOptionalObjectType(SGM.M, optionalKind);
  assert(optionalKind != OTK_None);

  // Project out the payload area.
  auto someDecl = getASTContext().getOptionalSomeDecl(optionalKind);
  auto destPayload = B.createInitEnumDataAddr(loc, dest,
                                            someDecl,
                                            loweredPayloadTy.getAddressType());
  
  AbstractionPattern origType = AbstractionPattern::getOpaque();

  // Emit the value into the payload area.
  TemporaryInitialization emitInto(destPayload, CleanupHandle::invalid());
  auto &payloadTL = getTypeLowering(origType, value.getSubstType());
  std::move(value).forwardInto(*this, origType,
                               &emitInto,
                               payloadTL);
  
  // Inject the tag.
  B.createInjectEnumAddr(loc, dest, someDecl);
}

void SILGenFunction::emitInjectOptionalNothingInto(SILLocation loc, 
                                                   SILValue dest,
                                                   const TypeLowering &optTL) {
  OptionalTypeKind OTK;
  optTL.getLoweredType().getSwiftRValueType()->getAnyOptionalObjectType(OTK);
  assert(OTK != OTK_None);
  
  B.createInjectEnumAddr(loc, dest, getASTContext().getOptionalNoneDecl(OTK));
}
      
/// Return a value for an optional ".None" of the specified type. This only
/// works for loadable enum types.
SILValue SILGenFunction::getOptionalNoneValue(SILLocation loc,
                                              const TypeLowering &optTL) {
  assert(optTL.isLoadable() && "Address-only optionals cannot use this");
  OptionalTypeKind OTK;
  optTL.getLoweredType().getSwiftRValueType()->getAnyOptionalObjectType(OTK);
  assert(OTK != OTK_None);

  return B.createEnum(loc, SILValue(), getASTContext().getOptionalNoneDecl(OTK),
                      optTL.getLoweredType());
}

/// Return a value for an optional ".Some(x)" of the specified type. This only
/// works for loadable enum types.
ManagedValue SILGenFunction::
getOptionalSomeValue(SILLocation loc, ManagedValue value,
                     const TypeLowering &optTL) {
  assert(optTL.isLoadable() && "Address-only optionals cannot use this");
  SILType optType = optTL.getLoweredType();
  CanType formalOptType = optType.getSwiftRValueType();

  OptionalTypeKind OTK;
  auto formalObjectType = formalOptType->getAnyOptionalObjectType(OTK)
    ->getCanonicalType();
  assert(OTK != OTK_None);
  auto someDecl = getASTContext().getOptionalSomeDecl(OTK);
  
  AbstractionPattern origType = AbstractionPattern::getOpaque();

  // Reabstract input value to the type expected by the enum.
  value = emitSubstToOrigValue(loc, value, origType, formalObjectType);

  SILValue result =
    B.createEnum(loc, value.forward(*this), someDecl,
                 optTL.getLoweredType());
  return emitManagedRValueWithCleanup(result, optTL);
}

static Substitution getSimpleSubstitution(GenericSignature *genericSig,
                                          CanType typeArg) {
  assert(genericSig->getGenericParams().size() == 1);
  return Substitution{typeArg, {}};
}

/// Create the correct substitution for calling the given function at
/// the given type.
static Substitution getSimpleSubstitution(FuncDecl *fn, CanType typeArg) {
  auto genericFnType =
    cast<GenericFunctionType>(fn->getInterfaceType()->getCanonicalType());
  return getSimpleSubstitution(genericFnType->getGenericSignature(), typeArg);
}

static CanType getOptionalValueType(SILType optType,
                                    OptionalTypeKind &optionalKind) {
  auto generic = cast<BoundGenericType>(optType.getSwiftRValueType());
  optionalKind = generic->getDecl()->classifyAsOptionalType();
  assert(optionalKind);
  return generic.getGenericArgs()[0];
}

void SILGenFunction::emitPreconditionOptionalHasValue(SILLocation loc,
                                                      SILValue addr) {
  OptionalTypeKind OTK;
  getOptionalValueType(addr->getType().getObjectType(), OTK);

  // Generate code to the optional is present, and if not abort with a message
  // (provided by the stdlib).
  SILBasicBlock *contBB = createBasicBlock();
  SILBasicBlock *failBB = createBasicBlock();

  auto NoneEnumElementDecl = getASTContext().getOptionalNoneDecl(OTK);
  B.createSwitchEnumAddr(loc, addr, /*defaultDest*/contBB,
                         { { NoneEnumElementDecl, failBB }});

  B.emitBlock(failBB);

  // Call the standard library implementation of _diagnoseUnexpectedNilOptional.
  if (auto diagnoseFailure =
        getASTContext().getDiagnoseUnexpectedNilOptional(nullptr)) {
    emitApplyOfLibraryIntrinsic(loc, diagnoseFailure, {}, {},
                                SGFContext());
  }

  B.createUnreachable(loc);
  B.clearInsertionPoint();
  B.emitBlock(contBB);
}

SILValue SILGenFunction::emitDoesOptionalHaveValue(SILLocation loc,
                                                   SILValue addrOrValue) {
  SILType optType = addrOrValue->getType().getObjectType();
  OptionalTypeKind optionalKind;
  getOptionalValueType(optType, optionalKind);

  auto boolTy = SILType::getBuiltinIntegerType(1, getASTContext());
  SILValue yes = B.createIntegerLiteral(loc, boolTy, 1);
  SILValue no = B.createIntegerLiteral(loc, boolTy, 0);
  auto someDecl = getASTContext().getOptionalSomeDecl(optionalKind);
  
  if (addrOrValue->getType().isAddress())
    return B.createSelectEnumAddr(loc, addrOrValue, boolTy, no,
                                  std::make_pair(someDecl, yes));
  return B.createSelectEnum(loc, addrOrValue, boolTy, no,
                            std::make_pair(someDecl, yes));
}

ManagedValue SILGenFunction::emitCheckedGetOptionalValueFrom(SILLocation loc,
                                                      ManagedValue src,
                                                      const TypeLowering &optTL,
                                                      SGFContext C) {
  SILType optType = src.getType().getObjectType();
  OptionalTypeKind optionalKind;
  CanType valueType = getOptionalValueType(optType, optionalKind);

  FuncDecl *fn = getASTContext().getOptionalUnwrappedDecl(
      nullptr, optionalKind);
  Substitution sub = getSimpleSubstitution(fn, valueType);

  // The intrinsic takes its parameter indirectly.
  if (src.getType().isObject()) {
    auto buf = emitTemporaryAllocation(loc, src.getType());
    B.createStore(loc, src.forward(*this), buf);
    src = emitManagedBufferWithCleanup(buf);
  }

  RValue result = emitApplyOfLibraryIntrinsic(loc, fn, sub, src, C);
  if (result) {
    return std::move(result).getAsSingleValue(*this, loc);
  } else {
    return ManagedValue::forInContext();
  }
}

ManagedValue SILGenFunction::emitUncheckedGetOptionalValueFrom(SILLocation loc,
                                                    ManagedValue addrOrValue,
                                                    const TypeLowering &optTL,
                                                    SGFContext C) {
  OptionalTypeKind OTK;
  SILType origPayloadTy =
    addrOrValue.getType().getAnyOptionalObjectType(SGM.M, OTK);

  auto formalOptionalTy = addrOrValue.getType().getSwiftRValueType();
  auto formalPayloadTy = formalOptionalTy
    ->getAnyOptionalObjectType()
    ->getCanonicalType();
  
  auto someDecl = getASTContext().getOptionalSomeDecl(OTK);
 
  ManagedValue payload;

  // Take the payload from the optional.  Cheat a bit in the +0
  // case—UncheckedTakeEnumData will never actually invalidate an Optional enum
  // value.
  SILValue payloadVal;
  if (!addrOrValue.getType().isAddress()) {
    payloadVal = B.createUncheckedEnumData(loc, addrOrValue.forward(*this),
                                           someDecl);
  } else {
    payloadVal =
      B.createUncheckedTakeEnumDataAddr(loc, addrOrValue.forward(*this),
                                        someDecl, origPayloadTy);
  
    if (optTL.isLoadable())
      payloadVal = B.createLoad(loc, payloadVal);
  }

  // Produce a correctly managed value.
  if (addrOrValue.hasCleanup())
    payload = emitManagedRValueWithCleanup(payloadVal);
  else
    payload = ManagedValue::forUnmanaged(payloadVal);
  
  // Reabstract it to the substituted form, if necessary.
  return emitOrigToSubstValue(loc, payload, AbstractionPattern::getOpaque(),
                              formalPayloadTy, C);
}

/// Emit an optional-to-optional transformation.
ManagedValue
SILGenFunction::emitOptionalToOptional(SILLocation loc,
                                       ManagedValue input,
                                       SILType resultTy,
                                       const ValueTransform &transformValue) {
  auto contBB = createBasicBlock();
  auto isNotPresentBB = createBasicBlock();
  auto isPresentBB = createBasicBlock();

  // Create a temporary for the output optional.
  auto &resultTL = getTypeLowering(resultTy);

  // If the result is address-only, we need to return something in memory,
  // otherwise the result is the BBArgument in the merge point.
  SILValue result;
  if (resultTL.isAddressOnly())
    result = emitTemporaryAllocation(loc, resultTy);
  else
    result = new (F.getModule()) SILArgument(contBB, resultTL.getLoweredType());

  
  // Branch on whether the input is optional, this doesn't consume the value.
  auto isPresent = emitDoesOptionalHaveValue(loc, input.getValue());
  B.createCondBranch(loc, isPresent, isPresentBB, isNotPresentBB);

  // If it's present, apply the recursive transformation to the value.
  B.emitBlock(isPresentBB);
  SILValue branchArg;
  {
    // Don't allow cleanups to escape the conditional block.
    FullExpr presentScope(Cleanups, CleanupLocation::get(loc));

    CanType resultValueTy =
      resultTy.getSwiftRValueType().getAnyOptionalObjectType();
    assert(resultValueTy);
    SILType loweredResultValueTy = getLoweredType(resultValueTy);

    // Pull the value out.  This will load if the value is not address-only.
    auto &inputTL = getTypeLowering(input.getType());
    auto inputValue = emitUncheckedGetOptionalValueFrom(loc, input,
                                                        inputTL, SGFContext());

    // Transform it.
    auto resultValue = transformValue(*this, loc, inputValue,
                                      loweredResultValueTy);

    // Inject that into the result type if the result is address-only.
    if (resultTL.isAddressOnly()) {
      ArgumentSource resultValueRV(loc, RValue(resultValue, resultValueTy));
      emitInjectOptionalValueInto(loc, std::move(resultValueRV),
                                  result, resultTL);
    } else {
      resultValue = getOptionalSomeValue(loc, resultValue, resultTL);
      branchArg = resultValue.forward(*this);
    }
  }
  if (branchArg)
    B.createBranch(loc, contBB, branchArg);
  else
    B.createBranch(loc, contBB);

  // If it's not present, inject 'nothing' into the result.
  B.emitBlock(isNotPresentBB);
  if (resultTL.isAddressOnly()) {
    emitInjectOptionalNothingInto(loc, result, resultTL);
    B.createBranch(loc, contBB);
  } else {
    branchArg = getOptionalNoneValue(loc, resultTL);
    B.createBranch(loc, contBB, branchArg);
  }

  // Continue.
  B.emitBlock(contBB);
  if (resultTL.isAddressOnly())
    return emitManagedBufferWithCleanup(result, resultTL);

  return emitManagedRValueWithCleanup(result, resultTL);
}

SILGenFunction::OpaqueValueRAII::~OpaqueValueRAII() {
  auto entry = Self.OpaqueValues.find(OpaqueValue);
  assert(entry != Self.OpaqueValues.end());
  Self.OpaqueValues.erase(entry);
}

RValue
SILGenFunction::emitPointerToPointer(SILLocation loc,
                                     ManagedValue input,
                                     CanType inputType,
                                     CanType outputType,
                                     SGFContext C) {
  auto converter = getASTContext().getConvertPointerToPointerArgument(nullptr);

  // The generic function currently always requires indirection, but pointers
  // are always loadable.
  auto origBuf = emitTemporaryAllocation(loc, input.getType());
  B.createStore(loc, input.forward(*this), origBuf);
  auto origValue = emitManagedBufferWithCleanup(origBuf);
  
  // Invoke the conversion intrinsic to convert to the destination type.
  Substitution subs[2] = {
    getPointerSubstitution(inputType),
    getPointerSubstitution(outputType),
  };
  
  return emitApplyOfLibraryIntrinsic(loc, converter, subs, origValue, C);
}


namespace {

/// This is an initialization for an address-only existential in memory.
class ExistentialInitialization : public KnownAddressInitialization {
  CleanupHandle Cleanup;
public:
  /// \param existential The existential container
  /// \param address Address of value in existential container
  /// \param concreteFormalType Unlowered AST type of value
  /// \param repr Representation of container
  ExistentialInitialization(SILValue existential, SILValue address,
                            CanType concreteFormalType,
                            ExistentialRepresentation repr,
                            SILGenFunction &gen)
      : KnownAddressInitialization(address) {
    // Any early exit before we store a value into the existential must
    // clean up the existential container.
    Cleanup = gen.enterDeinitExistentialCleanup(existential,
                                                concreteFormalType,
                                                repr);
  }

  void finishInitialization(SILGenFunction &gen) {
    SingleBufferInitialization::finishInitialization(gen);
    gen.Cleanups.setCleanupState(Cleanup, CleanupState::Dead);
  }
};

}

ManagedValue SILGenFunction::emitExistentialErasure(
                            SILLocation loc,
                            CanType concreteFormalType,
                            const TypeLowering &concreteTL,
                            const TypeLowering &existentialTL,
                            ArrayRef<ProtocolConformanceRef> conformances,
                            SGFContext C,
                            llvm::function_ref<ManagedValue (SGFContext)> F) {
  // Mark the needed conformances as used.
  for (auto conformance : conformances)
    SGM.useConformance(conformance);

  switch (existentialTL.getLoweredType().getObjectType()
            .getPreferredExistentialRepresentation(SGM.M, concreteFormalType)) {
  case ExistentialRepresentation::None:
    llvm_unreachable("not an existential type");
  case ExistentialRepresentation::Metatype: {
    assert(existentialTL.isLoadable());

    SILValue metatype = F(SGFContext()).getUnmanagedValue();
    assert(metatype->getType().castTo<AnyMetatypeType>()->getRepresentation()
             == MetatypeRepresentation::Thick);

    auto upcast =
      B.createInitExistentialMetatype(loc, metatype,
                                      existentialTL.getLoweredType(),
                                      conformances);
    return ManagedValue::forUnmanaged(upcast);
  }
  case ExistentialRepresentation::Class: {
    assert(existentialTL.isLoadable());

    ManagedValue sub = F(SGFContext());
    SILValue v = B.createInitExistentialRef(loc,
                                            existentialTL.getLoweredType(),
                                            concreteFormalType,
                                            sub.getValue(),
                                            conformances);
    return ManagedValue(v, sub.getCleanup());
  }
  case ExistentialRepresentation::Boxed: {
    // Allocate the existential.
    auto *existential = B.createAllocExistentialBox(loc,
                                           existentialTL.getLoweredType(),
                                           concreteFormalType,
                                           conformances);
    auto *valueAddr = B.createProjectExistentialBox(loc,
                                           concreteTL.getLoweredType(),
                                           existential);
    // Initialize the concrete value in-place.
    InitializationPtr init(
        new ExistentialInitialization(existential, valueAddr, concreteFormalType,
                                      ExistentialRepresentation::Boxed,
                                      *this));
    ManagedValue mv = F(SGFContext(init.get()));
    if (!mv.isInContext()) {
      mv.forwardInto(*this, loc, init->getAddress());
      init->finishInitialization(*this);
    }
    
    return emitManagedRValueWithCleanup(existential);
  }
  case ExistentialRepresentation::Opaque: {
    // Allocate the existential.
    SILValue existential =
      getBufferForExprResult(loc, existentialTL.getLoweredType(), C);

    // Allocate the concrete value inside the container.
    SILValue valueAddr = B.createInitExistentialAddr(
                            loc, existential,
                            concreteFormalType,
                            concreteTL.getLoweredType(),
                            conformances);
    // Initialize the concrete value in-place.
    InitializationPtr init(
        new ExistentialInitialization(existential, valueAddr, concreteFormalType,
                                      ExistentialRepresentation::Opaque,
                                      *this));
    ManagedValue mv = F(SGFContext(init.get()));
    if (!mv.isInContext()) {
      mv.forwardInto(*this, loc, init->getAddress());
      init->finishInitialization(*this);
    }

    return manageBufferForExprResult(existential, existentialTL, C);
  }
  }
}

ManagedValue SILGenFunction::emitClassMetatypeToObject(SILLocation loc,
                                                       ManagedValue v,
                                                       SILType resultTy) {
  SILValue value = v.getUnmanagedValue();

  // Convert the metatype to objc representation.
  auto metatypeTy = value->getType().castTo<MetatypeType>();
  auto objcMetatypeTy = CanMetatypeType::get(metatypeTy.getInstanceType(),
                                             MetatypeRepresentation::ObjC);
  value = B.createThickToObjCMetatype(loc, value,
                           SILType::getPrimitiveObjectType(objcMetatypeTy));
  
  // Convert to an object reference.
  value = B.createObjCMetatypeToObject(loc, value, resultTy);

  return ManagedValue::forUnmanaged(value);
}

ManagedValue SILGenFunction::emitExistentialMetatypeToObject(SILLocation loc,
                                                             ManagedValue v,
                                                             SILType resultTy) {
  SILValue value = v.getUnmanagedValue();
  
  // Convert the metatype to objc representation.
  auto metatypeTy = value->getType().castTo<ExistentialMetatypeType>();
  auto objcMetatypeTy = CanExistentialMetatypeType::get(
                                              metatypeTy.getInstanceType(),
                                              MetatypeRepresentation::ObjC);
  value = B.createThickToObjCMetatype(loc, value,
                               SILType::getPrimitiveObjectType(objcMetatypeTy));
  
  // Convert to an object reference.
  value = B.createObjCExistentialMetatypeToObject(loc, value, resultTy);
  
  return ManagedValue::forUnmanaged(value);
}

ManagedValue SILGenFunction::emitProtocolMetatypeToObject(SILLocation loc,
                                                          CanType inputTy,
                                                          SILType resultTy) {
  ProtocolDecl *protocol = inputTy->castTo<MetatypeType>()
    ->getInstanceType()->castTo<ProtocolType>()->getDecl();

  SILValue value = B.createObjCProtocol(loc, protocol, resultTy);
  
  // Protocol objects, despite being global objects, inherit default reference
  // counting semantics from NSObject, so we need to retain the protocol
  // reference when we use it to prevent it being released and attempting to
  // deallocate itself. It doesn't matter if we ever actually clean up that
  // retain though.
  B.createStrongRetain(loc, value, Atomicity::Atomic);
  
  return ManagedValue::forUnmanaged(value);
}

SILGenFunction::OpaqueValueState
SILGenFunction::emitOpenExistential(
       SILLocation loc,
       ManagedValue existentialValue,
       CanArchetypeType openedArchetype,
       SILType loweredOpenedType) {
  // Open the existential value into the opened archetype value.
  bool isUnique = true;
  bool canConsume;
  ManagedValue archetypeMV;
  
  SILType existentialType = existentialValue.getType();
  switch (existentialType.getPreferredExistentialRepresentation(SGM.M)) {
  case ExistentialRepresentation::Opaque: {
    assert(existentialType.isAddress());
    SILValue archetypeValue = B.createOpenExistentialAddr(
                                loc, existentialValue.forward(*this),
                                loweredOpenedType);
    if (existentialValue.hasCleanup()) {
      canConsume = true;
      // Leave a cleanup to deinit the existential container.
      enterDeinitExistentialCleanup(existentialValue.getValue(), CanType(),
                                    ExistentialRepresentation::Opaque);
      archetypeMV = emitManagedBufferWithCleanup(archetypeValue);
    } else {
      canConsume = false;
      archetypeMV = ManagedValue::forUnmanaged(archetypeValue);
    }
    break;
  }
  case ExistentialRepresentation::Metatype:
    assert(existentialType.isObject());
    archetypeMV =
        ManagedValue::forUnmanaged(
            B.createOpenExistentialMetatype(
                       loc, existentialValue.forward(*this),
                       loweredOpenedType));
    // Metatypes are always trivial. Consuming would be a no-op.
    canConsume = false;
    break;
  case ExistentialRepresentation::Class: {
    assert(existentialType.isObject());
    SILValue archetypeValue = B.createOpenExistentialRef(
                       loc, existentialValue.forward(*this),
                       loweredOpenedType);
    canConsume = existentialValue.hasCleanup();
    archetypeMV = (canConsume ? emitManagedRValueWithCleanup(archetypeValue)
                              : ManagedValue::forUnmanaged(archetypeValue));
    break;
  }
  case ExistentialRepresentation::Boxed:
    if (existentialType.isAddress()) {
      existentialValue = emitLoad(loc, existentialValue.getValue(),
                                  getTypeLowering(existentialType),
                                  SGFContext::AllowGuaranteedPlusZero,
                                  IsNotTake);
    }

    existentialType = existentialValue.getType();
    assert(existentialType.isObject());
    // NB: Don't forward the cleanup, because consuming a boxed value won't
    // consume the box reference.
    archetypeMV = ManagedValue::forUnmanaged(
        B.createOpenExistentialBox(loc, existentialValue.getValue(),
                                   loweredOpenedType));
    // The boxed value can't be assumed to be uniquely referenced. We can never
    // consume it.
    // TODO: We could use isUniquelyReferenced to shorten the duration of
    // the box to the point that the opaque value is copied out.
    isUnique = false;
    canConsume = false;
    break;
  case ExistentialRepresentation::None:
    llvm_unreachable("not existential");
  }
  setArchetypeOpeningSite(openedArchetype, archetypeMV.getValue());

  assert(!canConsume || isUnique); (void) isUnique;

  return SILGenFunction::OpaqueValueState{
    archetypeMV,
    /*isConsumable*/ canConsume,
    /*hasBeenConsumed*/ false
  };
}

ManagedValue SILGenFunction::manageOpaqueValue(OpaqueValueState &entry,
                                               SILLocation loc,
                                               SGFContext C) {
  // If the opaque value is consumable, we can just return the
  // value with a cleanup. There is no need to retain it separately.
  if (entry.IsConsumable) {
    assert(!entry.HasBeenConsumed
           && "Uniquely-referenced opaque value already consumed");
    entry.HasBeenConsumed = true;
    return entry.Value;
  }

  assert(!entry.Value.hasCleanup());

  // If the context wants a +0 value, guaranteed or immediate, we can
  // give it to them, because OpenExistential emission guarantees the
  // value.
  if (C.isGuaranteedPlusZeroOk()) {
    return entry.Value;
  }

  // If the context wants us to initialize a buffer, copy there instead
  // of making a temporary allocation.
  if (auto I = C.getEmitInto()) {
    if (SILValue address = I->getAddressForInPlaceInitialization()) {
      entry.Value.copyInto(*this, address, loc);
      I->finishInitialization(*this);
      return ManagedValue::forInContext();
    }
  }

  // Otherwise, copy the value into a temporary.
  return entry.Value.copyUnmanaged(*this, loc);
}