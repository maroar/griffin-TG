/******************************************************************************
 * Copyright (c) 2016 Leandro T. C. Melo (ltcmelo@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *****************************************************************************/

#ifndef PSYCHE_RANGE_H__
#define PSYCHE_RANGE_H__


#include "Symbol.h"
#include "FullySpecifiedType.h"
#include "TranslationUnit.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <utility>
#include <list>
#include <set>

namespace psyche {

class AbstractValue;
class IntegerValue;
class SymbolValue;
class EmptyValue;
class InfinityValue;
class NAryValue;
class UndefinedValue;
class UnaryValue;

struct Simbol : public CPlusPlus::Symbol {
public:
    Simbol(const char*, CPlusPlus::TranslationUnit*);
    ~Simbol();

    // Simbol's interface
    virtual CPlusPlus::FullySpecifiedType type() const
    { return CPlusPlus::FullySpecifiedType(); }

    static unsigned cnt_;
protected:
    virtual void visitSymbol0(CPlusPlus::SymbolVisitor *visitor)
    {}

public:
    const CPlusPlus::StringLiteral *_initializer;
    CPlusPlus::FullySpecifiedType _type;
    CPlusPlus::Identifier* id_;
    CPlusPlus::TranslationUnit* program_;
};

/*!
 * \brief The Range class
 */
class Range final
{
public:
    Range(std::unique_ptr<AbstractValue> lower,
          std::unique_ptr<AbstractValue> upper);
    Range(const Range&);
    Range(Range&&) = default;
    Range& operator=(const Range&);
    Range& operator=(Range&&) = default;

    Range rangeIntersection(Range* rB);
    Range rangeUnion(const Range &rB);
    Range evaluate();
    bool operator==(const Range& rhs) const;
    bool operator!=(const Range& rhs) const { return !(*this == rhs); }
    bool empty() const;
    bool isConst();

    std::unique_ptr<AbstractValue> upper();
    std::unique_ptr<AbstractValue> lower();

    friend std::ostream& operator<<(std::ostream& os, const Range& range);


    std::unique_ptr<AbstractValue> lower_;
    std::unique_ptr<AbstractValue> upper_;
private:
    friend class RangeAnalysis;
};

/*!
 * \brief The AbstractValue class
 */
enum KindOfValue : uint8_t { KInteger, KSymbol, KEmpty, KInfinity, KUndefined, KBool, KNAry, KUnary };
enum Sign : uint8_t { Positive, Negative };

class AbstractValue
{
public:
    AbstractValue(KindOfValue k) : kindOfValue_(k) {}
    virtual ~AbstractValue() = default;
    virtual std::unique_ptr<AbstractValue> clone() const = 0;
    virtual std::string toString() = 0;
    virtual std::string toCCode() { return toString(); }
    virtual KindOfValue getKindOfValue() = 0;
    virtual bool sameType(AbstractValue *value) {return getKindOfValue() == value->getKindOfValue();}
    virtual std::unique_ptr<AbstractValue> evaluate() = 0;
    //! Develop products of NaryValue
    virtual std::unique_ptr<AbstractValue> develop() = 0;
    virtual void buildSymbolDependence() { symbolDep_.clear(); }
    virtual std::list<std::unique_ptr<AbstractValue> > termsClone()
    // For a correct copy of the terms of a NAryValue from an AbtractValue pointer
        { return std::list<std::unique_ptr<AbstractValue> >();}
    virtual std::set<std::unique_ptr<AbstractValue> > asSet() = 0; //!< Separate min and max argument into different AV

    virtual std::unique_ptr<AbstractValue> operator+(int);
    virtual std::unique_ptr<AbstractValue> operator+(AbstractValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator+(IntegerValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator+(SymbolValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator+(InfinityValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator+(NAryValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator+(UndefinedValue&);
    virtual std::unique_ptr<AbstractValue> operator+(UnaryValue&) {return nullptr;}

    virtual std::unique_ptr<AbstractValue> operator-(int);
    virtual std::unique_ptr<AbstractValue> operator-(AbstractValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator-(IntegerValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator-(SymbolValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator-(InfinityValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator-(NAryValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator-(UndefinedValue&);
    virtual std::unique_ptr<AbstractValue> operator-(UnaryValue&) {return nullptr;}

    virtual std::unique_ptr<AbstractValue> operator*(AbstractValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator*(IntegerValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator*(SymbolValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator*(InfinityValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator*(NAryValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator*(UndefinedValue&);
    virtual std::unique_ptr<AbstractValue> operator*(UnaryValue&) {return nullptr;}

    virtual std::unique_ptr<AbstractValue> operator/(AbstractValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator/(IntegerValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator/(SymbolValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator/(InfinityValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator/(NAryValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator/(UndefinedValue&);
    virtual std::unique_ptr<AbstractValue> operator/(UnaryValue&) {return nullptr;}

    virtual std::unique_ptr<AbstractValue> operator<<(AbstractValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator<<(IntegerValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator<<(SymbolValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator<<(InfinityValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator<<(NAryValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator<<(UndefinedValue&);
    virtual std::unique_ptr<AbstractValue> operator<<(UnaryValue&) {return nullptr;}

    virtual std::unique_ptr<AbstractValue> operator>>(AbstractValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator>>(IntegerValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator>>(SymbolValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator>>(InfinityValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator>>(NAryValue&) {return nullptr;}
    virtual std::unique_ptr<AbstractValue> operator>>(UndefinedValue&);
    virtual std::unique_ptr<AbstractValue> operator>>(UnaryValue&) {return nullptr;}

    virtual bool operator==(AbstractValue&) {return false;}
    virtual bool operator==(IntegerValue&) {return false;}
    virtual bool operator==(SymbolValue&) {return false;}
    virtual bool operator==(InfinityValue&) {return false;}
    virtual bool operator==(NAryValue&) {return false;}
    virtual bool operator==(UndefinedValue&) {return false;}
    virtual bool operator==(UnaryValue&) {return false;}

    virtual bool operator<(AbstractValue&) {return false;}
    virtual bool operator<(IntegerValue&) {return false;}
    virtual bool operator<(SymbolValue&) {return false;}
    virtual bool operator<(InfinityValue&) {return false;}
    virtual bool operator<(NAryValue&) {return false;}
    virtual bool operator<(UndefinedValue&) {return false;}
    virtual bool operator<(UnaryValue&) {return false;}

    virtual bool operator>(AbstractValue&) {return false;}
    virtual bool operator>(IntegerValue&) {return false;}
    virtual bool operator>(SymbolValue&) {return false;}
    virtual bool operator>(InfinityValue&) {return false;}
    virtual bool operator>(NAryValue&) {return false;}
    virtual bool operator>(UndefinedValue&) {return false;}
    virtual bool operator>(UnaryValue&) {return false;}

    std::set<const CPlusPlus::Symbol*> symbolDep_;
    KindOfValue kindOfValue_;

protected:
    AbstractValue() = default;
};

class IntegerValue final : public AbstractValue
{
public:
    IntegerValue(int64_t value)
        : AbstractValue(KInteger), value_(value)
    {}

    std::unique_ptr<AbstractValue> clone() const override;
    //Bound evaluate() override { return Bound(value_); }
    std::unique_ptr<AbstractValue> evaluate() override;
    std::unique_ptr<AbstractValue> develop() override
        { return clone(); }
    std::string toString() override;
    KindOfValue getKindOfValue() override;
    int64_t getValue();
    std::set<std::unique_ptr<AbstractValue> > asSet() override;

    std::unique_ptr<AbstractValue> operator+(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator+(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator+(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator+(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator+(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator+(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator+(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator-(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator-(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator-(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator-(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator-(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator-(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator-(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator*(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator*(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator*(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator*(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator*(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator*(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator*(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator/(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator/(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator/(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator/(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator/(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator/(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator/(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator>>(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator>>(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator>>(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator>>(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator>>(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator<<(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator<<(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator<<(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator<<(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator<<(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UnaryValue&) override;

    bool operator==(AbstractValue&) override;
    bool operator==(IntegerValue&) override;
    bool operator==(SymbolValue&) override;
    bool operator==(InfinityValue&) override;
    bool operator==(NAryValue&) override;
    bool operator==(UndefinedValue&) override;
    bool operator==(UnaryValue&) override;

    bool operator<(AbstractValue&) override;
    bool operator<(IntegerValue&) override;
    bool operator<(SymbolValue&) override;
    bool operator<(InfinityValue&) override;
    bool operator<(NAryValue&) override;
    bool operator<(UndefinedValue&) override;
    bool operator<(UnaryValue&) override;

    bool operator>(AbstractValue&) override;
    bool operator>(IntegerValue&) override;
    bool operator>(SymbolValue&) override;
    bool operator>(InfinityValue&) override;
    bool operator>(NAryValue&) override;
    bool operator>(UndefinedValue&) override;
    bool operator>(UnaryValue&) override;

private:
    int64_t value_;
    //KindOfValue kindOfValue_;
};

class SymbolValue final : public AbstractValue
{
public:
    SymbolValue(const CPlusPlus::Symbol* symbol)
        : AbstractValue(KSymbol), symbol_(symbol)
    {}

    std::unique_ptr<AbstractValue> clone() const override;
    //Bound evaluate() override;
    std::unique_ptr<AbstractValue> evaluate() override;
    std::unique_ptr<AbstractValue> develop() override
        { return clone(); }
    std::string toString() override;
    KindOfValue getKindOfValue() override;
    void buildSymbolDependence() override;
    std::set<std::unique_ptr<AbstractValue> > asSet() override;

    std::unique_ptr<AbstractValue> operator+(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator+(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator+(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator+(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator+(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator+(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator+(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator-(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator-(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator-(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator-(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator-(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator-(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator-(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator*(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator*(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator*(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator*(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator*(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator*(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator*(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator/(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator/(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator/(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator/(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator/(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator/(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator/(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator>>(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator>>(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator>>(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator>>(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator>>(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator<<(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator<<(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator<<(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator<<(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator<<(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UnaryValue&) override;

    bool operator==(AbstractValue&) override;
    bool operator==(IntegerValue&) override;
    bool operator==(SymbolValue&) override;
    bool operator==(InfinityValue&) override;
    bool operator==(NAryValue&) override;
    bool operator==(UndefinedValue&) override;
    bool operator==(UnaryValue&) override;

    bool operator<(AbstractValue&) override;
    bool operator<(IntegerValue&) override;
    bool operator<(SymbolValue&) override;
    bool operator<(InfinityValue&) override;
    bool operator<(NAryValue&) override;
    bool operator<(UndefinedValue&) override;
    bool operator<(UnaryValue&) override;

    bool operator>(AbstractValue&) override;
    bool operator>(IntegerValue&) override;
    bool operator>(SymbolValue&) override;
    bool operator>(InfinityValue&) override;
    bool operator>(NAryValue&) override;
    bool operator>(UndefinedValue&) override;
    bool operator>(UnaryValue&) override;

private:
    const CPlusPlus::Symbol* symbol_;
    //KindOfValue kindOfValue_;
};

enum Operation : std::uint8_t
{
    BadValue,
    Addition,
    Subtraction,
    Multiplication,
    Division,
    SquareRoot,
    // discrete things
    Modulo,
    Minimum,
    Maximum,
    // language things
    ShiftRight,
    ShiftLeft
};

void printop(Operation op);

class InfinityValue final : public AbstractValue
{
public:
    InfinityValue(Sign sign)
        : AbstractValue(KInfinity), sign_(sign)
    {}

    std::unique_ptr<AbstractValue> clone() const override;
    std::unique_ptr<AbstractValue> evaluate() override;
    std::unique_ptr<AbstractValue> develop() override
        { return clone(); }
    std::string toString() override;
    KindOfValue getKindOfValue() override;
    Sign getSign();
    std::set<std::unique_ptr<AbstractValue> > asSet() override;

    std::unique_ptr<AbstractValue> operator+(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator+(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator+(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator+(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator+(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator+(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator+(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator-(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator-(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator-(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator-(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator-(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator-(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator-(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator*(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator*(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator*(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator*(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator*(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator*(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator*(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator/(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator/(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator/(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator/(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator/(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator/(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator/(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator>>(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator>>(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator>>(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator>>(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator>>(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator<<(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator<<(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator<<(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator<<(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator<<(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UnaryValue&) override;

    bool operator==(AbstractValue&) override;
    bool operator==(IntegerValue&) override;
    bool operator==(SymbolValue&) override;
    bool operator==(InfinityValue&) override;
    bool operator==(NAryValue&) override;
    bool operator==(UndefinedValue&) override;
    bool operator==(UnaryValue&) override;

    bool operator<(AbstractValue&) override;
    bool operator<(IntegerValue&) override;
    bool operator<(SymbolValue&) override;
    bool operator<(InfinityValue&) override;
    bool operator<(NAryValue&) override;
    bool operator<(UndefinedValue&) override;
    bool operator<(UnaryValue&) override;

    bool operator>(AbstractValue&) override;
    bool operator>(IntegerValue&) override;
    bool operator>(SymbolValue&) override;
    bool operator>(InfinityValue&) override;
    bool operator>(NAryValue&) override;
    bool operator>(UndefinedValue&) override;
    bool operator>(UnaryValue&) override;

private:
    Sign sign_;
    //KindOfValue kindOfValue_;
};

class NAryValue final : public AbstractValue
{
public:
    NAryValue(std::unique_ptr<AbstractValue> a,
              std::unique_ptr<AbstractValue> b,
              Operation op);
    NAryValue(std::unique_ptr<AbstractValue> a,
              std::unique_ptr<AbstractValue> b,
              std::unique_ptr<AbstractValue> c,
              Operation op);
    NAryValue(std::list<std::unique_ptr<AbstractValue> >& terms,
              Operation op);

    std::unique_ptr<AbstractValue> evaluate() override; // _EVAL ***************************************
    std::unique_ptr<AbstractValue> develop() override;

    std::unique_ptr<AbstractValue> clone() const override;
    std::list<std::unique_ptr<AbstractValue> > termsClone() override;
    std::string toString() override;
    // In C Code, min and max must include the number of operands
    std::string toCCode() override;
    KindOfValue getKindOfValue() override;
    void buildSymbolDependence() override;
    std::set<std::unique_ptr<AbstractValue> > asSet() override;

    void add(std::unique_ptr<AbstractValue> v); // _ADD

    void simplify();

    // Addition
    void simplifyAdd();
    void simplifyInt();
    void simplifyTerms();

    // Multiplication
    void simplifyMul();
    void simplifySumsInMul();
    void simplifyDiv();
    //void simplifyCoefficients();

    // Min
    void simplifyMin();
    void simplifyRemoveLargers();
    void simplifyRemoveMinMin();

    // Max
    void simplifyMax();
    void simplifyRemoveMinors();
    void simplifyRemoveMaxMax();

    // Min/Max
    void simplifyMaxMin();
    void simplifyEq();
    void simplifyInf(Sign sign);

    // simplification for printing things
    void removeTimesOne();
    void removeSumToZero();

    std::unique_ptr<AbstractValue> operator+(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator+(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator+(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator+(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator+(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator+(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator+(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator-(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator-(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator-(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator-(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator-(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator-(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator-(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator*(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator*(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator*(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator*(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator*(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator*(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator*(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator/(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator/(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator/(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator/(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator/(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator/(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator/(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator>>(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator>>(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator>>(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator>>(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator>>(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator<<(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator<<(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator<<(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator<<(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator<<(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UnaryValue&) override;

    bool operator==(AbstractValue&) override;
    bool operator==(IntegerValue&) override;
    bool operator==(SymbolValue&) override;
    bool operator==(InfinityValue&) override;
    bool operator==(NAryValue&) override;
    bool operator==(UndefinedValue&) override;
    bool operator==(UnaryValue&) override;

    bool operator<(AbstractValue&) override;
    bool operator<(IntegerValue&) override;
    bool operator<(SymbolValue&) override;
    bool operator<(InfinityValue&) override;
    bool operator<(NAryValue&) override;
    bool operator<(UndefinedValue&) override;
    bool operator<(UnaryValue&) override;

    bool operator>(AbstractValue&) override;
    bool operator>(IntegerValue&) override;
    bool operator>(SymbolValue&) override;
    bool operator>(InfinityValue&) override;
    bool operator>(NAryValue&) override;
    bool operator>(UndefinedValue&) override;
    bool operator>(UnaryValue&) override;

    bool isDiscrete();
    bool isShift();
    bool isTimesOne();

    std::list<std::unique_ptr<AbstractValue> > terms_;
    //KindOfValue kindOfValue_;
    Operation op_;
};

class UndefinedValue final : public AbstractValue
{
public:
    UndefinedValue()
        : AbstractValue(KUndefined) {}

    std::unique_ptr<AbstractValue> clone() const override;
    std::unique_ptr<AbstractValue> evaluate() override;
    std::unique_ptr<AbstractValue> develop() override
        { return clone(); }
    std::string toString() override;
    KindOfValue getKindOfValue() override;
    std::set<std::unique_ptr<AbstractValue> > asSet() override;

    std::unique_ptr<AbstractValue> operator+(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator+(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator+(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator+(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator+(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator+(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator+(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator-(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator-(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator-(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator-(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator-(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator-(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator-(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator*(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator*(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator*(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator*(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator*(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator*(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator*(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator/(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator/(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator/(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator/(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator/(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator/(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator/(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator>>(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator>>(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator>>(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator>>(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator>>(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator<<(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator<<(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator<<(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator<<(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator<<(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UnaryValue&) override;

    bool operator==(AbstractValue&) {return false;}
    bool operator==(IntegerValue&) {return false;}
    bool operator==(SymbolValue&) {return false;}
    bool operator==(InfinityValue&) {return false;}
    bool operator==(NAryValue&) {return false;}
    bool operator==(UndefinedValue&) {return false;}
    bool operator==(UnaryValue&) {return false;}

    bool operator<(AbstractValue&) {return false;}
    bool operator<(IntegerValue&) {return false;}
    bool operator<(SymbolValue&) {return false;}
    bool operator<(InfinityValue&) {return false;}
    bool operator<(NAryValue&) {return false;}
    bool operator<(UndefinedValue&) {return false;}
    bool operator<(UnaryValue&) {return false;}

    bool operator>(AbstractValue&) {return false;}
    bool operator>(IntegerValue&) {return false;}
    bool operator>(SymbolValue&) {return false;}
    bool operator>(InfinityValue&) {return false;}
    bool operator>(NAryValue&) {return false;}
    bool operator>(UndefinedValue&) {return false;}
    bool operator>(UnaryValue&) {return false;}
};

class UnaryValue final : public AbstractValue
{
public:

    UnaryValue(std::unique_ptr<AbstractValue> value, Operation op) :
        AbstractValue(KUnary), value_(std::move(value)), op_(op) {}

    virtual std::unique_ptr<AbstractValue> clone() const override;
    virtual std::string toString() override;
    virtual std::string toCCode() override;
    virtual KindOfValue getKindOfValue() override;
    virtual std::unique_ptr<AbstractValue> evaluate() override;
    virtual std::unique_ptr<AbstractValue> develop() override;
    virtual void buildSymbolDependence() override;
    virtual std::list<std::unique_ptr<AbstractValue> > termsClone() override;
    virtual std::set<std::unique_ptr<AbstractValue> > asSet() override;

    std::unique_ptr<AbstractValue> operator+(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator+(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator+(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator+(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator+(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator+(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator+(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator-(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator-(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator-(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator-(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator-(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator-(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator-(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator*(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator*(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator*(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator*(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator*(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator*(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator*(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator/(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator/(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator/(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator/(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator/(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator/(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator/(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator>>(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator>>(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator>>(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator>>(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator>>(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator>>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> operator<<(AbstractValue&) override;
    std::unique_ptr<AbstractValue> operator<<(IntegerValue&) override;
    std::unique_ptr<AbstractValue> operator<<(SymbolValue&) override;
    std::unique_ptr<AbstractValue> operator<<(InfinityValue&) override;
    std::unique_ptr<AbstractValue> operator<<(NAryValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UndefinedValue&) override;
    std::unique_ptr<AbstractValue> operator<<(UnaryValue&) override;

    bool operator==(AbstractValue&) override;
    bool operator==(IntegerValue&) override;
    bool operator==(SymbolValue&) override;
    bool operator==(InfinityValue&) override;
    bool operator==(NAryValue&) override;
    bool operator==(UndefinedValue&) override;
    bool operator==(UnaryValue&) override;

    bool operator<(AbstractValue&) override;
    bool operator<(IntegerValue&) override;
    bool operator<(SymbolValue&) override;
    bool operator<(InfinityValue&) override;
    bool operator<(NAryValue&) override;
    bool operator<(UndefinedValue&) override;
    bool operator<(UnaryValue&) override;

    bool operator>(AbstractValue&) override;
    bool operator>(IntegerValue&) override;
    bool operator>(SymbolValue&) override;
    bool operator>(InfinityValue&) override;
    bool operator>(NAryValue&) override;
    bool operator>(UndefinedValue&) override;
    bool operator>(UnaryValue&) override;

    std::unique_ptr<AbstractValue> value_;
    Operation op_;
};

std::ostream& operator<<(std::ostream& os, const Range& range);

using AV = std::unique_ptr<psyche::AbstractValue>;

} // namespace psyche

#endif
