//===-- llvm/DerivedTypes.h - Classes for handling data types ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of classes that represent "derived
// types".  These are things like "arrays of x" or "structure of x, y, z" or
// "method returning x taking (y,z) as parameters", etc...
//
// The implementations of these classes live in the Type.cpp file.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DERIVED_TYPES_H
#define LLVM_DERIVED_TYPES_H

#include "llvm/Type.h"

namespace llvm {

class Value;
template<class ValType, class TypeClass> class TypeMap;
class FunctionValType;
class ArrayValType;
class StructValType;
class PointerValType;
class VectorValType;
class IntegerValType;
class APInt;

class DerivedType : public Type {
  friend class Type;

protected:
  DerivedType(TypeID id) : Type(id) {}

  /// notifyUsesThatTypeBecameConcrete - Notify AbstractTypeUsers of this type
  /// that the current type has transitioned from being abstract to being
  /// concrete.
  ///
  void notifyUsesThatTypeBecameConcrete();

  /// dropAllTypeUses - When this (abstract) type is resolved to be equal to
  /// another (more concrete) type, we must eliminate all references to other
  /// types, to avoid some circular reference problems.
  ///
  void dropAllTypeUses();

public:

  //===--------------------------------------------------------------------===//
  // Abstract Type handling methods - These types have special lifetimes, which
  // are managed by (add|remove)AbstractTypeUser. See comments in
  // AbstractTypeUser.h for more information.

  /// refineAbstractTypeTo - This function is used to when it is discovered that
  /// the 'this' abstract type is actually equivalent to the NewType specified.
  /// This causes all users of 'this' to switch to reference the more concrete
  /// type NewType and for 'this' to be deleted.
  ///
  void refineAbstractTypeTo(const Type *NewType);

  void dump() const { Type::dump(); }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const DerivedType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->isDerivedType();
  }
};

/// Class to represent integer types. Note that this class is also used to
/// represent the built-in integer types: Int1Ty, Int8Ty, Int16Ty, Int32Ty and
/// Int64Ty. 
/// @brief Integer representation type
class IntegerType : public DerivedType {
protected:
  IntegerType(unsigned NumBits) : DerivedType(IntegerTyID) {
    setSubclassData(NumBits);
  }
  friend class TypeMap<IntegerValType, IntegerType>;
public:
  /// This enum is just used to hold constants we need for IntegerType.
  enum {
    MIN_INT_BITS = 1,        ///< Minimum number of bits that can be specified
    MAX_INT_BITS = (1<<23)-1 ///< Maximum number of bits that can be specified
      ///< Note that bit width is stored in the Type classes SubclassData field
      ///< which has 23 bits. This yields a maximum bit width of 8,388,607 bits.
  };

  /// This static method is the primary way of constructing an IntegerType. 
  /// If an IntegerType with the same NumBits value was previously instantiated,
  /// that instance will be returned. Otherwise a new one will be created. Only
  /// one instance with a given NumBits value is ever created.
  /// @brief Get or create an IntegerType instance.
  static const IntegerType* get(unsigned NumBits);

  /// @brief Get the number of bits in this IntegerType
  unsigned getBitWidth() const { return getSubclassData(); }

  /// getBitMask - Return a bitmask with ones set for all of the bits
  /// that can be set by an unsigned version of this type.  This is 0xFF for
  /// sbyte/ubyte, 0xFFFF for shorts, etc.
  uint64_t getBitMask() const {
    return ~uint64_t(0UL) >> (64-getPrimitiveSizeInBits());
  }

  /// For example, this is 0xFF for an 8 bit integer, 0xFFFF for i16, etc.
  /// @returns a bit mask with ones set for all the bits of this type.
  /// @brief Get a bit mask for this type.
  APInt getMask() const;

  /// This method determines if the width of this IntegerType is a power-of-2
  /// in terms of 8 bit bytes. 
  /// @returns true if this is a power-of-2 byte width.
  /// @brief Is this a power-of-2 byte-width IntegerType ?
  bool isPowerOf2ByteWidth() const;

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const IntegerType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == IntegerTyID;
  }
};


/// FunctionType - Class to represent function types
///
class FunctionType : public DerivedType {
public:
  /// Function parameters can have attributes to indicate how they should be
  /// treated by optimizations and code generation. This enumeration lists the
  /// set of possible attributes.
  /// @brief Function parameter attributes enumeration.
  enum ParameterAttributes {
    NoAttributeSet    = 0,      ///< No attribute value has been set 
    ZExtAttribute     = 1,      ///< zero extended before/after call
    SExtAttribute     = 1 << 1, ///< sign extended before/after call
    NoReturnAttribute = 1 << 2, ///< mark the function as not returning
    InRegAttribute    = 1 << 3, ///< force argument to be passed in register
    StructRetAttribute= 1 << 4  ///< hidden pointer to structure to return
  };
  typedef std::vector<ParameterAttributes> ParamAttrsList;
private:
  friend class TypeMap<FunctionValType, FunctionType>;
  bool isVarArgs;
  ParamAttrsList *ParamAttrs;

  FunctionType(const FunctionType &);                   // Do not implement
  const FunctionType &operator=(const FunctionType &);  // Do not implement
  FunctionType(const Type *Result, const std::vector<const Type*> &Params,
               bool IsVarArgs, const ParamAttrsList &Attrs);

public:
  /// FunctionType::get - This static method is the primary way of constructing
  /// a FunctionType. 
  ///
  static FunctionType *get(
    const Type *Result, ///< The result type
    const std::vector<const Type*> &Params, ///< The types of the parameters
    bool isVarArg, ///< Whether this is a variable argument length function
    const ParamAttrsList & Attrs = ParamAttrsList()
      ///< Indicates the parameter attributes to use, if any. The 0th entry
      ///< in the list refers to the return type. Parameters are numbered
      ///< starting at 1. 
  );

  inline bool isVarArg() const { return isVarArgs; }
  inline const Type *getReturnType() const { return ContainedTys[0]; }

  typedef std::vector<PATypeHandle>::const_iterator param_iterator;
  param_iterator param_begin() const { return ContainedTys.begin()+1; }
  param_iterator param_end() const { return ContainedTys.end(); }

  // Parameter type accessors...
  const Type *getParamType(unsigned i) const { return ContainedTys[i+1]; }

  /// getNumParams - Return the number of fixed parameters this function type
  /// requires.  This does not consider varargs.
  ///
  unsigned getNumParams() const { return unsigned(ContainedTys.size()-1); }

  bool isStructReturn() const {
    return (getNumParams() && paramHasAttr(1, StructRetAttribute));
  }
  
  /// The parameter attributes for the \p ith parameter are returned. The 0th
  /// parameter refers to the return type of the function.
  /// @returns The ParameterAttributes for the \p ith parameter.
  /// @brief Get the attributes for a parameter
  ParameterAttributes getParamAttrs(unsigned i) const;

  /// @brief Determine if a parameter attribute is set
  bool paramHasAttr(unsigned i, ParameterAttributes attr) const {
    return getParamAttrs(i) & attr;
  }

  /// @brief Return the number of parameter attributes this type has.
  unsigned getNumAttrs() const { 
    return (ParamAttrs ?  unsigned(ParamAttrs->size()) : 0);
  }

  /// @brief Convert a ParameterAttribute into its assembly text
  static std::string getParamAttrsText(ParameterAttributes Attr);

  // Implement the AbstractTypeUser interface.
  virtual void refineAbstractType(const DerivedType *OldTy, const Type *NewTy);
  virtual void typeBecameConcrete(const DerivedType *AbsTy);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const FunctionType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == FunctionTyID;
  }
};


/// CompositeType - Common super class of ArrayType, StructType, PointerType
/// and VectorType
class CompositeType : public DerivedType {
protected:
  inline CompositeType(TypeID id) : DerivedType(id) { }
public:

  /// getTypeAtIndex - Given an index value into the type, return the type of
  /// the element.
  ///
  virtual const Type *getTypeAtIndex(const Value *V) const = 0;
  virtual bool indexValid(const Value *V) const = 0;

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const CompositeType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == ArrayTyID ||
           T->getTypeID() == StructTyID ||
           T->getTypeID() == PointerTyID ||
           T->getTypeID() == VectorTyID;
  }
};


/// StructType - Class to represent struct types
///
class StructType : public CompositeType {
  friend class TypeMap<StructValType, StructType>;
  StructType(const StructType &);                   // Do not implement
  const StructType &operator=(const StructType &);  // Do not implement
  StructType(const std::vector<const Type*> &Types, bool isPacked);
public:
  /// StructType::get - This static method is the primary way to create a
  /// StructType.
  ///
  static StructType *get(const std::vector<const Type*> &Params, 
                         bool isPacked=false);

  // Iterator access to the elements
  typedef std::vector<PATypeHandle>::const_iterator element_iterator;
  element_iterator element_begin() const { return ContainedTys.begin(); }
  element_iterator element_end() const { return ContainedTys.end(); }

  // Random access to the elements
  unsigned getNumElements() const { return unsigned(ContainedTys.size()); }
  const Type *getElementType(unsigned N) const {
    assert(N < ContainedTys.size() && "Element number out of range!");
    return ContainedTys[N];
  }

  /// getTypeAtIndex - Given an index value into the type, return the type of
  /// the element.  For a structure type, this must be a constant value...
  ///
  virtual const Type *getTypeAtIndex(const Value *V) const ;
  virtual bool indexValid(const Value *V) const;

  // Implement the AbstractTypeUser interface.
  virtual void refineAbstractType(const DerivedType *OldTy, const Type *NewTy);
  virtual void typeBecameConcrete(const DerivedType *AbsTy);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const StructType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == StructTyID;
  }

  bool isPacked() const { return getSubclassData(); }
};


/// SequentialType - This is the superclass of the array, pointer and packed
/// type classes.  All of these represent "arrays" in memory.  The array type
/// represents a specifically sized array, pointer types are unsized/unknown
/// size arrays, vector types represent specifically sized arrays that
/// allow for use of SIMD instructions.  SequentialType holds the common
/// features of all, which stem from the fact that all three lay their
/// components out in memory identically.
///
class SequentialType : public CompositeType {
  SequentialType(const SequentialType &);                  // Do not implement!
  const SequentialType &operator=(const SequentialType &); // Do not implement!
protected:
  SequentialType(TypeID TID, const Type *ElType) : CompositeType(TID) {
    ContainedTys.reserve(1);
    ContainedTys.push_back(PATypeHandle(ElType, this));
  }

public:
  inline const Type *getElementType() const { return ContainedTys[0]; }

  virtual bool indexValid(const Value *V) const;

  /// getTypeAtIndex - Given an index value into the type, return the type of
  /// the element.  For sequential types, there is only one subtype...
  ///
  virtual const Type *getTypeAtIndex(const Value *V) const {
    return ContainedTys[0];
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const SequentialType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == ArrayTyID ||
           T->getTypeID() == PointerTyID ||
           T->getTypeID() == VectorTyID;
  }
};


/// ArrayType - Class to represent array types
///
class ArrayType : public SequentialType {
  friend class TypeMap<ArrayValType, ArrayType>;
  uint64_t NumElements;

  ArrayType(const ArrayType &);                   // Do not implement
  const ArrayType &operator=(const ArrayType &);  // Do not implement
  ArrayType(const Type *ElType, uint64_t NumEl);
public:
  /// ArrayType::get - This static method is the primary way to construct an
  /// ArrayType
  ///
  static ArrayType *get(const Type *ElementType, uint64_t NumElements);

  inline uint64_t getNumElements() const { return NumElements; }

  // Implement the AbstractTypeUser interface.
  virtual void refineAbstractType(const DerivedType *OldTy, const Type *NewTy);
  virtual void typeBecameConcrete(const DerivedType *AbsTy);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const ArrayType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == ArrayTyID;
  }
};

/// VectorType - Class to represent vector types
///
class VectorType : public SequentialType {
  friend class TypeMap<VectorValType, VectorType>;
  unsigned NumElements;

  VectorType(const VectorType &);                   // Do not implement
  const VectorType &operator=(const VectorType &);  // Do not implement
  VectorType(const Type *ElType, unsigned NumEl);
public:
  /// VectorType::get - This static method is the primary way to construct an
  /// VectorType
  ///
  static VectorType *get(const Type *ElementType, unsigned NumElements);

  /// @brief Return the number of elements in the Vector type.
  inline unsigned getNumElements() const { return NumElements; }

  /// @brief Return the number of bits in the Vector type.
  inline unsigned getBitWidth() const { 
    return NumElements *getElementType()->getPrimitiveSizeInBits();
  }

  // Implement the AbstractTypeUser interface.
  virtual void refineAbstractType(const DerivedType *OldTy, const Type *NewTy);
  virtual void typeBecameConcrete(const DerivedType *AbsTy);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const VectorType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == VectorTyID;
  }
};


/// PointerType - Class to represent pointers
///
class PointerType : public SequentialType {
  friend class TypeMap<PointerValType, PointerType>;
  PointerType(const PointerType &);                   // Do not implement
  const PointerType &operator=(const PointerType &);  // Do not implement
  PointerType(const Type *ElType);
public:
  /// PointerType::get - This is the only way to construct a new pointer type.
  static PointerType *get(const Type *ElementType);

  // Implement the AbstractTypeUser interface.
  virtual void refineAbstractType(const DerivedType *OldTy, const Type *NewTy);
  virtual void typeBecameConcrete(const DerivedType *AbsTy);

  // Implement support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const PointerType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == PointerTyID;
  }
};


/// OpaqueType - Class to represent abstract types
///
class OpaqueType : public DerivedType {
  OpaqueType(const OpaqueType &);                   // DO NOT IMPLEMENT
  const OpaqueType &operator=(const OpaqueType &);  // DO NOT IMPLEMENT
  OpaqueType();
public:
  /// OpaqueType::get - Static factory method for the OpaqueType class...
  ///
  static OpaqueType *get() {
    return new OpaqueType();           // All opaque types are distinct
  }

  // Implement the AbstractTypeUser interface.
  virtual void refineAbstractType(const DerivedType *OldTy, const Type *NewTy) {
    abort();   // FIXME: this is not really an AbstractTypeUser!
  }
  virtual void typeBecameConcrete(const DerivedType *AbsTy) {
    abort();   // FIXME: this is not really an AbstractTypeUser!
  }

  // Implement support for type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const OpaqueType *T) { return true; }
  static inline bool classof(const Type *T) {
    return T->getTypeID() == OpaqueTyID;
  }
};

} // End llvm namespace

#endif
