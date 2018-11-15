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

#include "Range.h"
#include "Assert.h"
#include "Literals.h"
#include "Symbols.h"
#include "Debug.h"

#include <algorithm>
#include <functional>
#include <string>
#include <cmath>

using namespace psyche;
using namespace CPlusPlus;
using namespace std;

void printKindOfValue(KindOfValue k)
{
    switch (k) {
    case KInteger:
        std::cout << "  Int  " << std::endl;
        break;
    case KSymbol:
        std::cout << "  Sym  " << std::endl;
        break;
    case KEmpty:
        std::cout << "  empty  " << std::endl;
        break;
    case KInfinity:
        std::cout << "  Inf  " << std::endl;
        break;
    case KUndefined:
        std::cout << "  undef  " << std::endl;
        break;
    case KBool:
        std::cout << "  bool  " << std::endl;
        break;
    case KNAry:
        std::cout << "  nary  " << std::endl;
        break;
    default:
        std::cout << "  wrong  " << std::endl;
        break;
    }
}

void printOperation(Operation op)
{
    switch (op) {
    case Addition:
        std::cout << "  Addition  " << std::endl;
        break;
    case Subtraction:
        std::cout << "  Subtraction  " << std::endl;
        break;
    case Multiplication:
        std::cout << "  Multiplication  " << std::endl;
        break;
    case Division:
        std::cout << "  Division  " << std::endl;
        break;
    case Maximum:
        std::cout << "  Maximum  " << std::endl;
        break;
    case Minimum:
        std::cout << "  Minimum  " << std::endl;
        break;
    case Modulo:
        std::cout << "  Modulo  " << std::endl;
        break;
    case ShiftRight:
        std::cout << "  ShiftRight  " << std::endl;
        break;
    case ShiftLeft:
        std::cout << "  ShiftLeft  " << std::endl;
        break;
    default:
        std::cout << "  undef  " << std::endl;
        break;
    }
}

void alertUndefined(AbstractValue& av1, string op, AbstractValue& av2)
{
    if (runingTests)
        return;

    AV n1 = av1.clone();
    AV n2 = av2.clone();
    string str1 = n1->toString();
    string str2 = n2->toString();
    string text = "";
    text += "[SymbolicComputation] ";
    text += "The values below generated an undefined value.\n";
    text += str1 + " " + op + " " + str2 + "\n";

    y(text);
}

// Range:

Range::Range(std::unique_ptr<AbstractValue> lower,
             std::unique_ptr<AbstractValue> upper)
    : lower_(std::move(lower))
    , upper_(std::move(upper))
{}

Range::Range(const Range &range)
    : lower_(range.lower_->clone())
    , upper_(range.upper_->clone())
{}

Range& Range::operator=(const Range& range)
{
    lower_ = range.lower_->clone();
    upper_ = range.upper_->clone();
    return *this;
}

Range Range::rangeIntersection(Range* rB)
{
    std::unique_ptr<AbstractValue> la = lower_->clone();
    std::unique_ptr<AbstractValue> ua = upper_->clone();

    std::unique_ptr<AbstractValue> lb = rB->lower_->clone();
    std::unique_ptr<AbstractValue> ub = rB->upper_->clone();

    KindOfValue kla = la->getKindOfValue();
    KindOfValue klb = lb->getKindOfValue();
    KindOfValue kua = ua->getKindOfValue();
    KindOfValue kub = ub->getKindOfValue();

    if (kla == KindOfValue::KInteger && klb == KindOfValue::KInteger &&
        kua == KindOfValue::KInteger && kub == KindOfValue::KInteger) {
        IntegerValue *ila = dynamic_cast<IntegerValue*>(la.get());
        IntegerValue *ilb = dynamic_cast<IntegerValue*>(lb.get());
        IntegerValue *iua = dynamic_cast<IntegerValue*>(ua.get());
        IntegerValue *iub = dynamic_cast<IntegerValue*>(ub.get());

        // if the bounds of ranges are integers we verify if they overlap and,
        // if so, we return the intersection.
        if (ila->getValue() < ilb->getValue()) {
            if (iua->getValue() < ilb->getValue()) {
                return Range(*this);
            } else {
                if (iua->getValue() < iub->getValue()) {
                    return Range(ilb->clone(), iua->clone());
                }
                else {
                    return Range(ilb->clone(), iub->clone());
                }
            }
        } else {
            if (ila->getValue() < iub->getValue()) {
                if (iua->getValue() < iub->getValue()) {
                    return Range(ila->clone(), iua->clone());
                }
                else {
                    return Range(ila->clone(), iub->clone());
                }
            } else {
                return Range(*this);
            }
        }
    }
    else {
        // General case
        NAryValue low(la->clone(), lb->clone(), Maximum);
        NAryValue up(ua->clone(), ub->clone(), Minimum);
        return Range(low.evaluate(), up.evaluate());
    }

    // if the ranges not overlap we return the first of them
    return Range(*this);
}

Range Range::rangeUnion(const Range &rB)
{
    // [min(va, vb), max(va, vb)]
    Range range(NAryValue(lower_->clone(), rB.lower_->clone(),
                               Operation::Minimum).evaluate(),
                     NAryValue(upper_->clone(), rB.upper_->clone(),
                               Operation::Maximum).evaluate());
    return range;
}

Range Range::evaluate()
{
    return Range(lower_->evaluate(), upper_->evaluate());
}

bool Range::operator==(const Range& rhs) const
{
    return *lower_ == *rhs.lower_.get() && *upper_ == *rhs.upper_.get();
}

bool Range::empty() const
{
    return *lower_->evaluate() > *upper_.get();
}

bool Range::isConst()
{
    return *lower_ == *upper_;
}

std::unique_ptr<AbstractValue> Range::upper()
{
    // r : [l, u] -> length(r) = u + 1
    return upper_->clone();
}

std::unique_ptr<AbstractValue> Range::lower()
{
    // r : [l, u] -> length(r) = u + 1
    return lower_->clone();
}

// Auxiliar

std::unique_ptr<AbstractValue> timesMinusOne(AbstractValue& av)
{
    IntegerValue iv(-1);
    return av * iv;
}

// remove common elements between two NAry values
bool removeEqual(NAryValue* op1, NAryValue* op2)
{
    auto it = op1->terms_.begin();
    bool remove;
    while (it != op1->terms_.end() && op1->terms_.size() > 0) {
        remove = false;
        auto it2 = op2->terms_.begin();
        while (it2 != op2->terms_.end() && op2->terms_.size() > 0) {
            if (**it == **it2) {
                remove = true;
                it = op1->terms_.erase(it);
                op2->terms_.erase(it2);
                break;
            } else
                it2++;
        }
        if (!remove) {
            return false;
        }
    }
    return (op1->terms_.size() == 0 && op2->terms_.size() == 0);
}

void simplifyCommonSymbols(NAryValue& n1, NAryValue& n2)
{
    SIMPLIFY:
    for (auto i = n1.terms_.begin(); i != n1.terms_.end(); i++) {
        if ((*i)->getKindOfValue() == KInteger)
            continue;
        for (auto j = n2.terms_.begin(); j != n2.terms_.end(); j++) {
            if ((*j)->getKindOfValue() == KInteger)
                continue;
            if (**i == **j) {
                n1.terms_.erase(i);
                n2.terms_.erase(j);
                goto SIMPLIFY;
            }
        }
    }
}

int gcd( int a, int b )
{
    if (a < 0 || b < 0)
        return gcd(std::abs(a), std::abs(b));

    if (a == 0 || b == 0)
       return 0;

    if (a%b == 0)
        return b;
    if (b%a == 0)
        return a;

    if (a > b) {
        return gcd(a%b, b);
    }
    return gcd(a, b%a);
}

// remove all common elements between two NAry values
bool removeAllEqual(NAryValue* op1, NAryValue* op2)
{
    if (op1->op_ != Addition || op2->op_ != Addition)
        return false;

    auto it = op1->terms_.begin();
    bool remove;
    while (it != op1->terms_.end() && op1->terms_.size() > 0) {
        remove = false;
        auto it2 = op2->terms_.begin();
        while (it2 != op2->terms_.end() && op2->terms_.size() > 0) {
            if (**it == **it2) {
                remove = true;
                it = op1->terms_.erase(it);
                op2->terms_.erase(it2);
                break;
            } else {
                it2++;
            }
        }
        if (!remove) {
            it++;
        }
    }
    return (op1->terms_.size() == 0 && op2->terms_.size() == 0);
}

std::unique_ptr<AbstractValue> toNAryValue(std::unique_ptr<AbstractValue> v, Operation op)
{
    std::list<std::unique_ptr<AbstractValue> > term;
    term.push_back(std::move(v));
    return NAryValue(term, op).evaluate();
}

// if the integer is greater than the division return 1
// if not, returns 2
int integerIsGreaterThanNAryDivision(IntegerValue& iv,
                                         NAryValue& nv) {
    if (nv.terms_.front()->getKindOfValue() == KInteger &&
        nv.terms_.back()->getKindOfValue() == KInteger) {
        IntegerValue* n = static_cast<IntegerValue*>(nv.terms_.front().get());
        IntegerValue* d = static_cast<IntegerValue*>(nv.terms_.back().get());
        double thisValue = (double) iv.getValue();
        double numerator = (double) n->getValue();
        double denominator = (double) d->getValue();
        double division = numerator/denominator;

        if (thisValue > division)
            return 1; // Integer, first argument, is greater
        else
            return 2; // NAry, second argument, is greater
    }

    return 0;
}

pair<bool, int> extractIntegerFromNAry(NAryValue& n)
{
    int num = 0;
    bool find = false;

    for (auto i = n.terms_.begin(); i != n.terms_.end(); i++) {
        if ((*i)->getKindOfValue() == KInteger) {
            IntegerValue* iv = static_cast<IntegerValue*>((*i).get());
            num = iv->getValue();
            n.terms_.erase(i);
            find = true;
            break;
        }
    }

    return pair<bool, int>(find, num);
}

void simplifyIntegersInDivision(NAryValue& n1, NAryValue& n2)
{
    auto integerInN1 = extractIntegerFromNAry(n1);
    bool find1 = integerInN1.first;
    int num1;
    if (!find1) num1 = 1;
    else num1 = integerInN1.second;

    auto integerInN2 = extractIntegerFromNAry(n2);
    bool find2 = integerInN2.first;
    int num2;
    if (!find2) num2 = 1;
    else num2 = integerInN2.second;

    int gcdValue = gcd(num1, num2);
    n1.add(IntegerValue(num1/gcdValue).evaluate());
    n2.add(IntegerValue(num2/gcdValue).evaluate());
}

// IntegerValue:

std::unique_ptr<AbstractValue> IntegerValue::clone() const
{
    return std::make_unique<IntegerValue>(value_);
}

std::string IntegerValue::toString()
{
    return std::to_string(value_);
}

KindOfValue IntegerValue::getKindOfValue()
{
    return kindOfValue_;
}

std::unique_ptr<AbstractValue> IntegerValue::evaluate()
{
    return std::make_unique<IntegerValue>(value_);
}

int64_t IntegerValue::getValue()
{
    return value_;
}

std::set<std::unique_ptr<AbstractValue> > IntegerValue::asSet()
{
    std::set<std::unique_ptr<AbstractValue> > s;
    s.insert(clone());
    return s;
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(NAryValue &nv)
{
    if (nv.op_ == Operation::Addition) {
        std::unique_ptr<AbstractValue> tmp = nv.clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(clone());
        return nvTmp->evaluate();
    } else if (nv.op_ == Operation::Minimum || nv.op_ == Operation::Maximum) {
        list<AV> terms;
        for (auto it = nv.terms_.begin(); it != nv.terms_.end(); it++) {
            terms.push_back(*this + **it);
        }

        return NAryValue(terms, nv.op_).evaluate();
    }

    return NAryValue(clone(), nv.clone(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(UnaryValue& ov)
{
    return NAryValue(clone(), ov.evaluate(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(NAryValue &nv)
{
    return *this + *timesMinusOne(nv);
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(UnaryValue& ov)
{
    return *this + *timesMinusOne(ov);
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(NAryValue &nv)
{
    if (getValue() == 0)
        return IntegerValue(0).evaluate();

    if (nv.op_ == Operation::Multiplication) {
        std::unique_ptr<AbstractValue> tmp = nv.clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(clone());
        return nvTmp->evaluate();
    } else if (nv.op_ == Operation::Addition) {
        list<AV> terms;
        for (auto it = nv.terms_.begin(); it != nv.terms_.end(); it++) {
            terms.push_back(*this * **it);
        }

        return NAryValue(terms, nv.op_).evaluate();
    } else if (nv.op_ == Operation::Division) {
        AV n = nv.terms_.front()->clone();

        AV newValue = *n * *this;

        return *newValue / *nv.terms_.back();
    } else if (nv.op_ == Operation::Maximum || nv.op_ == Operation::Minimum) {
        list<AV> terms;
        for (auto it = nv.terms_.begin(); it != nv.terms_.end(); it++) {
            terms.push_back(*this * **it);
        }

        if (getValue() > 0) {
            return NAryValue(terms, nv.op_).evaluate();
        } else {
            if (nv.op_ == Operation::Maximum) {
                return NAryValue(terms, Operation::Minimum).evaluate();
            } else {
                return NAryValue(terms, Operation::Maximum).evaluate();
            }
        }
    }

    return NAryValue(clone(), nv.clone(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(UnaryValue& ov)
{
    return NAryValue(clone(), ov.evaluate(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(NAryValue &nv)
{
    return NAryValue(clone(), nv.clone(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(UnaryValue& ov)
{
    return NAryValue(clone(), ov.evaluate(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(AbstractValue &av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return (*this >> iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> nv = av.clone();
        return (*this >> *nv);
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return (*this >> lv);
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return (*this >> nv);
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this >> uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this >> ov);
    }
    default:
        break;
    }
    r(toString());
    r(av.toString());
    std::cout << "ERROR: IntegerValue::operator>>(AbstractValue &av)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(IntegerValue &iv)
{
    return std::make_unique<IntegerValue>(getValue() >> iv.getValue());
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(SymbolValue &sv)
{
    return *this >> *sv.evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(InfinityValue &iv)
{
    alertUndefined(*this, ">>", iv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(NAryValue &nv)
{
    return std::make_unique<NAryValue>(clone(), nv.clone(), ShiftRight);
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator>>(UnaryValue& ov)
{
    return NAryValue(clone(), ov.evaluate(), Operation::ShiftRight).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(AbstractValue &av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return (*this << iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> nv = av.clone();
        return (*this << *nv);
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return (*this << lv);
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return (*this << nv);
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this << uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this << ov);
    }
    default:
        break;
    }
    r(toString());
    r(av.toString());
    std::cout << "ERROR: IntegerValue::operator<<(AbstractValue &)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(IntegerValue &iv)
{
    return std::make_unique<IntegerValue>(getValue() << iv.getValue());
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(SymbolValue &sv)
{
    return *this << *sv.evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(InfinityValue &iv)
{
    alertUndefined(*this, "<<", iv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(NAryValue &nv)
{
    return std::make_unique<NAryValue>(clone(), nv.clone(), ShiftLeft);
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator<<(UnaryValue& ov)
{
    return NAryValue(clone(), ov.evaluate(), Operation::ShiftLeft).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(AbstractValue& av)
{
    return av + *this;
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(IntegerValue& iv)
{
    return std::make_unique<IntegerValue>(getValue() + iv.getValue());
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(SymbolValue& sv)
{
    return NAryValue(clone(), sv.evaluate(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator+(InfinityValue& lv)
{
    return lv.clone();
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(AbstractValue& av)
{
    // a - b = a + ((-1) * b)
    std::unique_ptr<AbstractValue> negAv = timesMinusOne(av);
    return *this + *negAv;
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(IntegerValue& iv)
{
    return std::make_unique<IntegerValue>(getValue() - iv.getValue());
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(SymbolValue& sv)
{
    // a - b = a + ((-1) * b)
    return *this + *timesMinusOne(*sv.evaluate());
}

std::unique_ptr<AbstractValue> IntegerValue::operator-(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return std::make_unique<InfinityValue>(Sign::Negative);
    return std::make_unique<InfinityValue>(Sign::Positive);
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(AbstractValue& av)
{
    if (getValue() == 0)
        return (std::make_unique<IntegerValue>(0));
    if (getValue() == 1)
        return av.clone();

    return (av * *this);
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(IntegerValue& iv)
{
    return std::make_unique<IntegerValue>(getValue() * iv.getValue());
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(SymbolValue& sv)
{
    if (getValue() == 0)
        return (std::make_unique<IntegerValue>(0));

    return NAryValue(clone(),
                     sv.evaluate(),
                     Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator*(InfinityValue& lv)
{
    if (getValue() > 0) {
        return lv.clone();
    } else if (getValue() < 0) {
        if (lv.getSign() == Positive)
            return make_unique<InfinityValue>(Negative);
        return make_unique<InfinityValue>(Positive);
    }

    alertUndefined(*this, "*", lv);
    return std::make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this/iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> nv = av.evaluate();
        return (*this / *nv);
    }

    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return (*this/lv);
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return (*this/nv);
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this / uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this / ov);
    }
    default:
        break;
    }
    r(toString());
    r(av.toString());
    std::cout << "ERROR: IntegerValue::operator/(AbstractValue& av)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(IntegerValue& iv)
{
    if (iv.getValue() == 1)
        return clone();

    if (iv.getValue() == 0) {
        alertUndefined(*this, "/", iv);
        return UndefinedValue().evaluate();
    }

    return NAryValue(clone(),
                          iv.clone(),
                          Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(SymbolValue& sv)
{
    if (getValue() == 0)
        return (std::make_unique<IntegerValue>(0));

    return NAryValue(clone(),
                          sv.evaluate(),
                          Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> IntegerValue::operator/(InfinityValue&)
{
    return IntegerValue(0).clone();
}

bool IntegerValue::operator==(AbstractValue& av)
{
    return (av == *this);
}

bool IntegerValue::operator==(IntegerValue& iv)
{
    return (getValue() == iv.getValue());
}

bool IntegerValue::operator==(SymbolValue&) {return false;}

bool IntegerValue::operator==(InfinityValue&) {return false;}

bool IntegerValue::operator==(NAryValue &)
{
    return false;
}

bool IntegerValue::operator==(UndefinedValue &)
{
    return false;
}

bool IntegerValue::operator==(UnaryValue &)
{
    return false;
}

bool IntegerValue::operator<(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return (*this < iv);
    }
    case KSymbol: {
        SymbolValue& sv = dynamic_cast<SymbolValue&>(av);
        return *this < sv;
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return *this < lv;
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return *this < nv;
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this < uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this < ov);
    }
    default:
        break;
    }
    r(toString());
    r(av.toString());
    std::cout << "ERROR: IntegerValue::operator<(AbstractValue& av)" << std::endl;
    exit(1);
    return false;
}

bool IntegerValue::operator<(IntegerValue& iv)
{
    return (getValue() < iv.getValue());
}

bool IntegerValue::operator<(SymbolValue&) {return false;}

bool IntegerValue::operator<(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return true;
    return false;
}

bool IntegerValue::operator<(NAryValue& nv)
{
    if (nv.op_ == Division) {
        if (integerIsGreaterThanNAryDivision(*this, nv) == 2)
            return true;
        else
            return false;
    }

    return false;
}

bool IntegerValue::operator<(UndefinedValue &)
{
    return false;
}

bool IntegerValue::operator<(UnaryValue& ov)
{
    if (ov.op_ == SquareRoot && getValue() < 0)
        return true;

    return false;
}

bool IntegerValue::operator>(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return *this > iv;
    }
    case KSymbol: {
        SymbolValue& sv = dynamic_cast<SymbolValue&>(av);
        return *this > sv;
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return *this > lv;
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return *this > nv;
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this > uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this > ov);
    }
    default:
        break;
    }
    r(toString());
    r(av.toString());
    std::cout << "ERROR: IntegerValue::operator>(AbstractValue& av)" << std::endl;
    exit(1);
    return false;
}

bool IntegerValue::operator>(IntegerValue& iv)
{
    return (getValue() > iv.getValue());
}

bool IntegerValue::operator>(SymbolValue&) {return false;}

bool IntegerValue::operator>(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return false;
    else
        return true;
}

bool IntegerValue::operator>(NAryValue &nv)
{
    if (nv.op_ == Division) {
        if (integerIsGreaterThanNAryDivision(*this, nv) == 1)
            return true;
        else
            return false;
    }

    return false;
}

bool IntegerValue::operator>(UndefinedValue &)
{
    return false;
}

bool IntegerValue::operator>(UnaryValue& ov)
{
    if (ov.op_ == SquareRoot && getValue() < 0)
        return false;

    return false;
}

// SymbolValue:

std::unique_ptr<AbstractValue> SymbolValue::clone() const
{
    return std::make_unique<SymbolValue>(symbol_);
}

std::unique_ptr<AbstractValue> SymbolValue::evaluate()
{
    return std::make_unique<NAryValue>(
                std::make_unique<IntegerValue>(1),
                std::make_unique<SymbolValue>(symbol_),
                Operation::Multiplication);
}

std::string SymbolValue::toString()
{
    const Identifier *id = symbol_->name()->asNameId()->identifier();
    std::string ret(id->begin(), id->end());
    return ret;
}

KindOfValue SymbolValue::getKindOfValue()
{
    return kindOfValue_;
}

void SymbolValue::buildSymbolDependence()
{
    symbolDep_.clear();
    symbolDep_.insert(symbol_);
}

std::set<std::unique_ptr<AbstractValue> > SymbolValue::asSet()
{
    std::set<std::unique_ptr<AbstractValue> > s;
    s.insert(clone());
    return s;
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(NAryValue &nv)
{
    return *evaluate() + nv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(UnaryValue& ov)
{
    return NAryValue(evaluate(),
                     ov.evaluate(),
                     Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(NAryValue &nv)
{
    return *evaluate() + *timesMinusOne(nv);
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(UnaryValue& ov)
{
    return *evaluate() + *timesMinusOne(ov);
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(NAryValue &nv)
{
    return *evaluate() * nv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(UnaryValue& ov)
{
    return NAryValue(evaluate(),
                     ov.evaluate(),
                     Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(NAryValue &nv)
{
    return *evaluate() / nv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(UnaryValue& ov)
{
    return NAryValue(evaluate(),
                     ov.evaluate(),
                     Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(AbstractValue &av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return (*this >> iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return (*this >> *sv);
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return (*this >> lv);
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return (*this >> nv);
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this >> uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this >> ov);
    }
    default:
        break;
    }
    std::cout << "ERROR: SymbolValue::operator>>(AbstractValue &av)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(IntegerValue &iv)
{
    IntegerValue q(1 << iv.getValue());

    return *evaluate() / q;
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(SymbolValue &sv)
{
    return *clone() >> *sv.evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(InfinityValue &lv)
{
    alertUndefined(*this, ">>", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(NAryValue &nv)
{
    return *evaluate() >> nv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator>>(UnaryValue& ov)
{
    return NAryValue(evaluate(),
                     ov.evaluate(),
                     Operation::ShiftRight).evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(AbstractValue &av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this << iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return (*this << *sv);
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return (*this << lv);
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return (*this << nv);
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this << uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this << ov);
    }
    default:
        break;
    }
    std::cout << "ERROR: SymbolValue::operator<<(AbstractValue &av)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(NAryValue &nv)
{
    return *evaluate() << nv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(UnaryValue& ov)
{
    return NAryValue(evaluate(),
                     ov.evaluate(),
                     Operation::ShiftLeft).evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(IntegerValue &iv)
{
    IntegerValue q(1 << iv.getValue());

    return *evaluate() * q;
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(SymbolValue &sv)
{
    return *evaluate() << *sv.evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator<<(InfinityValue &lv)
{
    alertUndefined(*this, "<<", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(AbstractValue& av)
{
    return (av + *evaluate());
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(IntegerValue& iv)
{
    return *evaluate() + iv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(SymbolValue& sv)
{
    return *evaluate() + *sv.evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator+(InfinityValue& lv)
{
    return lv.clone();
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(AbstractValue& av)
{
    return *evaluate() + *timesMinusOne(av);
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(IntegerValue& iv)
{
    return *evaluate() + *timesMinusOne(iv);
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(SymbolValue& sv)
{
    return *evaluate() + *timesMinusOne(*sv.evaluate());
}

std::unique_ptr<AbstractValue> SymbolValue::operator-(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return std::make_unique<InfinityValue>(Sign::Negative);
    return std::make_unique<InfinityValue>(Sign::Positive);
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(AbstractValue& av)
{
    return (av * *evaluate());
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(IntegerValue& iv)
{
    return NAryValue(evaluate(),
                     iv.clone(),
                     Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(SymbolValue& sv)
{
    return *evaluate() * *sv.evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator*(InfinityValue& lv)
{
    alertUndefined(*this, "*", lv);
    return std::make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(AbstractValue& av)
{
    return *evaluate() / av;
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(IntegerValue& iv)
{
    if (iv.getValue() == 1)
        return evaluate();

    if (iv.getValue() == 0) {
        alertUndefined(*this, "/", iv);
        return UndefinedValue().evaluate();
    }

    return *evaluate() / iv;
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(SymbolValue& sv)
{
    return *evaluate() / *sv.evaluate();
}

std::unique_ptr<AbstractValue> SymbolValue::operator/(InfinityValue&)
{
    return std::make_unique<IntegerValue>(0);
}

bool SymbolValue::operator==(AbstractValue& av)
{
    return (av == *this);
}

bool SymbolValue::operator==(IntegerValue&) {return false;}

bool SymbolValue::operator==(SymbolValue& sv)
{
    return (toString().compare(sv.toString()) == 0);
}

bool SymbolValue::operator==(InfinityValue&) {return false;}

bool SymbolValue::operator==(NAryValue& nv)
{
    if (nv.isTimesOne()) { // 1*s1
        AV av = nv.clone();
        NAryValue& n = *static_cast<NAryValue*>(av.get());
        n.removeTimesOne();
        if (n.terms_.size() == 1 &&
                n.terms_.back()->getKindOfValue() == KSymbol) {
            SymbolValue* sv = static_cast<SymbolValue*>(n.terms_.back().get());
            return *this == *sv;
        }
    } else if (nv.op_ == Addition && nv.terms_.size() == 2) { // 1*s1 + 0
        AV av = nv.clone();
        NAryValue& n = *static_cast<NAryValue*>(av.get());
        auto integer = extractIntegerFromNAry(n);
        if (integer.first && integer.second == 0 &&
                n.terms_.back()->getKindOfValue() == KNAry) {
            return *this == *n.terms_.back();
        }
    }

    return  false;
}

bool SymbolValue::operator==(UndefinedValue &)
{
    return false;
}

bool SymbolValue::operator==(UnaryValue &)
{
    return false;
}

bool SymbolValue::operator<(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        return false;
    }
    case KSymbol: {
        return false;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this < lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *evaluate() < nv;
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this < ov);
    }
    default:
        break;
    }

    return false;
}

bool SymbolValue::operator<(IntegerValue&) {return false;}

bool SymbolValue::operator<(SymbolValue&) {return false;}

bool SymbolValue::operator<(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return true;
    return false;
}

bool SymbolValue::operator<(NAryValue& nv)
{
    return *evaluate() < nv;
}

bool SymbolValue::operator<(UndefinedValue &)
{
    return false;
}

bool SymbolValue::operator<(UnaryValue &)
{
    return false;
}

bool SymbolValue::operator>(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        return false;
    }
    case KSymbol: {
        return false;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this > lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *evaluate() > nv;
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this > ov);
    }
    default:
        break;
    }

    return false;
}

bool SymbolValue::operator>(IntegerValue&) {return false;}

bool SymbolValue::operator>(SymbolValue&) {return false;}

bool SymbolValue::operator>(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return false;
    return true;
}

bool SymbolValue::operator>(NAryValue& nv)
{
    return *evaluate() > nv;
}

bool SymbolValue::operator>(UndefinedValue &)
{
    return false;
}

bool SymbolValue::operator>(UnaryValue &)
{
    return false;
}

// InfinityValue:

std::unique_ptr<AbstractValue> InfinityValue::clone() const
{
    return std::make_unique<InfinityValue>(sign_);
}

std::unique_ptr<AbstractValue> InfinityValue::evaluate()
{
    return std::make_unique<InfinityValue>(sign_);
}

std::string InfinityValue::toString()
{
    if (sign_ == Positive)
      return "+Inf";
    else
      return "-Inf";
}

KindOfValue InfinityValue::getKindOfValue()
{
    return kindOfValue_;
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(NAryValue &nv)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(UnaryValue &)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(NAryValue &nv)
{
    alertUndefined(*this, "*", nv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(UnaryValue &)
{
    alertUndefined(*this, "sqrt", *IntegerValue(2).clone());
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(NAryValue &nv)
{
    alertUndefined(*this, "/", nv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(UnaryValue &)
{
    alertUndefined(*this, "sqrt", *IntegerValue(2).clone());
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(AbstractValue &av)
{
    alertUndefined(*this, ">>", av);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(IntegerValue &iv)
{
    alertUndefined(*this, ">>", iv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(SymbolValue &sv)
{
    alertUndefined(*this, ">>", sv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(InfinityValue &lv)
{
    alertUndefined(*this, ">>", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(NAryValue &nv)
{
    alertUndefined(*this, ">>", nv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator>>(UnaryValue &)
{
    alertUndefined(*this, "sqrt", *IntegerValue(2).clone());
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(AbstractValue &av)
{
    alertUndefined(*this, "<<", av);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(IntegerValue &iv)
{
    alertUndefined(*this, "<<", iv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(SymbolValue &sv)
{
    alertUndefined(*this, "<<", sv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(InfinityValue &lv)
{
    alertUndefined(*this, "<<", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(NAryValue &nv)
{
    alertUndefined(*this, "<<", nv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator<<(UnaryValue &)
{
    alertUndefined(*this, "sqrt", *IntegerValue(2).clone());
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(NAryValue &nv)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(UnaryValue &)
{
    return clone();
}

Sign InfinityValue::getSign()
{
    return sign_;
}

std::set<std::unique_ptr<AbstractValue> > InfinityValue::asSet()
{
    std::set<std::unique_ptr<AbstractValue> > s;
    s.insert(clone());
    return s;
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(AbstractValue& av)
{
    return (av + *this);
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(IntegerValue& iv)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(SymbolValue& sv)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator+(InfinityValue& lv)
{
    if (getSign() == lv.getSign())
        return clone();

    alertUndefined(*this, "+", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(AbstractValue& av)
{
    return *this + *timesMinusOne(av);
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(IntegerValue& iv)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(SymbolValue& sv)
{
    return clone();
}

std::unique_ptr<AbstractValue> InfinityValue::operator-(InfinityValue& lv)
{
    if (getSign() != lv.getSign())
        return clone();

    alertUndefined(*this, "-", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(AbstractValue& av)
{
    return av * *this;
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(IntegerValue& iv)
{
    if (iv.getValue() > 0) {
        return clone();
    } else if (iv.getValue() < 0) {
        if (getSign() == Positive)
            return make_unique<InfinityValue>(Negative);
        return make_unique<InfinityValue>(Positive);
    }

    alertUndefined(*this, "*", iv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(SymbolValue& sv)
{
    alertUndefined(*this, "*", sv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator*(InfinityValue& lv)
{
    if (getSign() == lv.getSign())
        return make_unique<InfinityValue>(Positive);
    return make_unique<InfinityValue>(Negative);
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(AbstractValue& av)
{
    alertUndefined(*this, "/", av);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(IntegerValue& iv)
{
    if (iv.getValue() > 0) {
        return clone();
    } else if (iv.getValue() < 0) {
        if (getSign() == Positive)
            return make_unique<InfinityValue>(Negative);
        return make_unique<InfinityValue>(Positive);
    }

    alertUndefined(*this, "/", iv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(SymbolValue& sv)
{
    alertUndefined(*this, "/", sv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> InfinityValue::operator/(InfinityValue& lv)
{
    alertUndefined(*this, "/", lv);
    return UndefinedValue().evaluate();
}

bool InfinityValue::operator==(AbstractValue& av)
{
    return (av == *this);
}

bool InfinityValue::operator==(IntegerValue& iv) {return false;}

bool InfinityValue::operator==(SymbolValue& sv) {return false;}

bool InfinityValue::operator==(InfinityValue& lv)
{
    return (getSign() == lv.getSign());
}

bool InfinityValue::operator==(NAryValue &)
{
    return false;
}

bool InfinityValue::operator==(UndefinedValue &)
{
    return false;
}

bool InfinityValue::operator==(UnaryValue &)
{
    return false;
}

bool InfinityValue::operator<(AbstractValue& av)
{
    if (av.getKindOfValue() == KUndefined)
        return false;

    if (getSign() == Sign::Negative)
        return true;
    return false;
}

bool InfinityValue::operator<(IntegerValue& iv)
{
    return (iv > *this);
}

bool InfinityValue::operator<(SymbolValue& sv)
{
    return (*clone() > *this);
}

bool InfinityValue::operator<(InfinityValue& lv)
{
    if (getSign() == Sign::Negative && lv.getSign() == Sign::Positive)
        return true;
    return false;
}

bool InfinityValue::operator<(NAryValue &)
{
    if (getSign() == Sign::Negative)
        return true;
    return false;
}

bool InfinityValue::operator<(UndefinedValue &)
{
    return false;
}

bool InfinityValue::operator<(UnaryValue &)
{
    if (getSign() == Sign::Negative)
        return true;
    return false;
}

bool InfinityValue::operator>(AbstractValue& av)
{
    if (av.getKindOfValue() == KUndefined)
        return false;

    if (getSign() == Sign::Positive)
        return true;
    return false;
}

bool InfinityValue::operator>(IntegerValue& iv)
{
    if (getSign() == Sign::Positive)
        return true;
    return false;
}

bool InfinityValue::operator>(SymbolValue& sv)
{
    if (getSign() == Sign::Positive)
        return true;
    return false;
}

bool InfinityValue::operator>(InfinityValue& lv)
{
    if (getSign() == Sign::Positive && lv.getSign() == Sign::Negative)
        return true;
    return false;
}

bool InfinityValue::operator>(NAryValue &)
{
    if (getSign() == Sign::Positive)
        return true;
    return false;
}

bool InfinityValue::operator>(UndefinedValue &)
{
    return false;
}

bool InfinityValue::operator>(UnaryValue &)
{
    if (getSign() == Sign::Positive)
        return true;
    return false;
}

// NAryValue:

NAryValue::NAryValue(std::unique_ptr<AbstractValue> a,
                     std::unique_ptr<AbstractValue> b,
                     Operation op) :
    AbstractValue(KNAry),
    op_(op)
{
    add(std::move(a));
    add(std::move(b));
}

NAryValue::NAryValue(std::unique_ptr<AbstractValue> a,
                     std::unique_ptr<AbstractValue> b,
                     std::unique_ptr<AbstractValue> c,
                     Operation op) :
    AbstractValue(KNAry),
    op_(op)
{
    add(std::move(a));
    add(std::move(b));
    add(std::move(c));
}

NAryValue::NAryValue(std::list<std::unique_ptr<AbstractValue> >& terms,
          Operation op) :
    AbstractValue(KNAry),
    op_(op),
    terms_(std::move(terms))
{
}

std::unique_ptr<AbstractValue> NAryValue::clone() const
{
    std::list<std::unique_ptr<AbstractValue> > terms;

    for (auto &it : terms_) {
        terms.push_back(it.get()->clone());
    }

    return std::make_unique<NAryValue>(terms, op_);
}

std::list<std::unique_ptr<AbstractValue> > NAryValue::termsClone()
{
    std::list<std::unique_ptr<AbstractValue> > terms;

    for (auto &it : terms_) {
        terms.push_back(it.get()->clone());
    }

    return terms;
}

std::unique_ptr<AbstractValue> NAryValue::evaluate()
{
    if (terms_.size() == 1)
        return terms_.front()->evaluate();
    // check if some term is an undefined value
    for (auto &it : terms_) {
        if (it.get()->getKindOfValue() == KUndefined) {
            return UndefinedValue().evaluate();
        }
    }
    // try to simplify the NAry value
    std::unique_ptr<AbstractValue> old;
    do {
        old = clone();
        simplify();
        // check if some term is an undefined value
        for (auto &it : terms_) {
            if (it.get()->getKindOfValue() == KUndefined) {
                return UndefinedValue().evaluate();
            }
        }

        if (terms_.size() == 1)
            return terms_.front()->evaluate();

        std::list<std::unique_ptr<AbstractValue> > terms;

        for (auto &it : terms_) {
            if (it.get()->getKindOfValue() == KSymbol) {
                if (op_ == Multiplication && terms_.size() == 1) {
                    terms.push_back(it.get()->evaluate());
                } else
                    terms.push_back(it.get()->clone());
            } else
                terms.push_back(it.get()->evaluate());
        }

        terms_.clear();
        terms_.merge(terms);

    } while (!(*old == *this));

    return clone();
}

std::unique_ptr<AbstractValue> NAryValue::develop()
{
    // just a copy of the terms
    std::list<std::unique_ptr<AbstractValue> > terms;
    for (auto &it : terms_) {
        terms.push_back(it.get()->develop());
    }

    if (op_ == Multiplication) {
        std::list<std::unique_ptr<AbstractValue> > resultTerms;
        std::vector<int> index(terms.size(), 0);

        // compute the number of terms in each term of this nary
        std::vector<int> maximum; // contains the number of terms in each term
        for (auto &it : terms) {
            if (it->getKindOfValue() == KNAry) {
                maximum.push_back(it->termsClone().size());
            } else {
                maximum.push_back(1);
            }
        }

        bool finish = false;
        while(!finish) {
            // Create new term
            int i = 0;
            std::unique_ptr<AbstractValue> newTerm = IntegerValue(1).clone();
            for (auto &it : terms) {
                if (it->getKindOfValue() == KNAry) {
                    std::list<std::unique_ptr<AbstractValue> > tmpList = it->termsClone();
                    auto it2 = tmpList.begin();
                    for (int j = 0; j < index[i]; j++) {
                        it2++;
                    }
                    newTerm = *newTerm * *((*it2)->clone());
                } else {
                    newTerm = *newTerm * *(it->clone());
                }
                i++;
            };
            resultTerms.push_back(newTerm->evaluate());
            // Increment to next term
            index.front()++;
            for (i = 0; i < maximum.size(); i++) {
                if (index[i] == maximum[i]) {
                    index[i] = 0;
                    if (i == maximum.size() - 1)
                        finish = true;
                    else
                        index[i+1] += 1;
                }
            }
        }
        return NAryValue(resultTerms, Addition).clone();
    } else {
        return NAryValue(terms, op_).evaluate();
    }
}

std::string NAryValue::toString()
{
    if (terms_.empty())
        return "empty";

    // simplify the number before print it
    if (!debugEnabled) {
        removeTimesOne();
        removeSumToZero();
    }

    if (terms_.empty())
        return "empty";

    if (terms_.size() == 1)
        return terms_.front()->toString();

    std::string op, exp = "(";

    switch (op_) {
    case Operation::Addition:
        op = " + ";
        break;
    case Operation::Multiplication:
        op = "*";
        break;
    case Operation::Division:
        op = "/";
        break;
    case Operation::Maximum:
        op = ", ";
        exp = "max(";
        break;
    case Operation::Minimum:
        op = ", ";
        exp = "min(";
        break;
    case Operation::ShiftLeft:
        op = " << ";
        break;
    case Operation::ShiftRight:
        op = " >> ";
        break;
    default:
        op = " op ";
        break;
    }

    exp += terms_.front().get()->toString();
    for (auto it = std::next(terms_.begin()); it != terms_.end(); ++it) {
        exp += op;
        exp += it->get()->toString();
    }

    exp += ")";

    return exp;
}

string NAryValue::toCCode()
{
    if (terms_.empty())
        return "empty";

    std::string op, exp = "(";

    switch (op_) {
    case Operation::Addition:
        op = " + ";
        break;
    case Operation::Multiplication:
        op = "*";
        break;
    case Operation::Division:
        op = "/";
        break;
    case Operation::Maximum:
        op = ", ";
        exp = "max(";
        break;
    case Operation::Minimum:
        op = ", ";
        exp = "min(";
        break;
    case Operation::ShiftLeft:
        op = " << ";
        break;
    case Operation::ShiftRight:
        op = " >> ";
        break;
    default:
        op = " op ";
        break;
    }

    if (op_ == Operation::Maximum || op_ == Operation::Minimum) {
        exp += std::to_string(terms_.size()) + op;
    }
    exp += terms_.front().get()->toCCode();
    for (auto it = std::next(terms_.begin()); it != terms_.end(); ++it) {
        exp += op;
        exp += it->get()->toCCode();
    }

    exp += ")";

    return exp;
}

KindOfValue NAryValue::getKindOfValue()
{
    return kindOfValue_;
}

void NAryValue::buildSymbolDependence()
{
    symbolDep_.clear();
    for (auto t = terms_.begin(); t != terms_.end(); ++t) {
        auto res = t->get()->clone();
        res->buildSymbolDependence();
        for (auto s = res->symbolDep_.begin(); s != res->symbolDep_.end(); s++) {
            symbolDep_.insert(*s);
        }
    }
}

std::set<std::unique_ptr<AbstractValue> > NAryValue::asSet()
{
    std::set<std::unique_ptr<AbstractValue> > s;
    // TODO
    s.insert(clone());
    return s;
}

void NAryValue::add(std::unique_ptr<AbstractValue> v)
{
    //terms_.push_back(std::move(v));
    //return ;
    if (getKindOfValue() == v->getKindOfValue()) {
        NAryValue *nv = static_cast<NAryValue*>(v.get());
        if (nv->op_ == op_) {
            if (op_ == Addition || op_ == Multiplication) {
                for (auto &it : nv->terms_) {
                    terms_.push_back(std::move(it->clone()));
                }
            } else if (op_ == Minimum || op_ == Maximum) {
                for (auto &it : nv->terms_) {
                    terms_.push_back(std::move(it->clone()));
                }
            } else
                terms_.push_back(std::move(v));
        } else
            terms_.push_back(std::move(v));
    } else
        terms_.push_back(std::move(v));
}

std::unique_ptr<AbstractValue> NAryValue::operator+(AbstractValue& av)
{
    return (av + *this);
}

std::unique_ptr<AbstractValue> NAryValue::operator+(IntegerValue& iv)
{
    if (op_ == Operation::Addition) {
        std::unique_ptr<AbstractValue> tmp = clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(iv.clone());
        return nvTmp->evaluate();
    } else if (op_ == Operation::Minimum || op_ == Operation::Maximum) {
        list<AV> terms;
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            terms.push_back(iv + **it);
        }

        return NAryValue(terms, op_).evaluate();
    }

    return NAryValue(clone(), iv.clone(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator+(SymbolValue& sv)
{
    return *this + *sv.evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator+(InfinityValue& lv)
{
    return lv.clone();
}

std::unique_ptr<AbstractValue> NAryValue::operator+(NAryValue& nv)
{
    if (op_ == Operation::Minimum || op_ == Operation::Maximum) {
        list<AV> terms;
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            terms.push_back(nv + **it);
        }

        return NAryValue(terms, op_).evaluate();
    } else if (nv.op_ == Operation::Minimum || nv.op_ == Operation::Maximum) {
        list<AV> terms;
        for (auto it = nv.terms_.begin(); it != nv.terms_.end(); it++) {
            terms.push_back(*this + **it);
        }

        return NAryValue(terms, nv.op_).evaluate();
    } else if (op_ == Operation::Addition) { ///////////////////////////// *-*
        std::unique_ptr<AbstractValue> tmp = clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(nv.clone());

        return nvTmp->evaluate();
    } else if (nv.op_ == Operation::Addition) {
        std::unique_ptr<AbstractValue> tmp = nv.clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(clone());

        return nvTmp->evaluate();
    } else if (op_ == Operation::Division && nv.op_ == Operation::Division) {
        if (*terms_.back() == *nv.terms_.back()) { // n1/d + n2/d = (n1+n2)/d
            AV n1 = terms_.front()->clone();
            AV n2 = nv.terms_.front()->clone();

            return *(*n1 + *n2) / *terms_.back()->clone();
        } else {
            //
        }
    }

    return NAryValue(clone(), nv.clone(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator+(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator+(UnaryValue& ov)
{
    if ((ov.op_ == SquareRoot) &&
            (op_ == Operation::Minimum || op_ == Operation::Maximum)) {
        list<AV> terms;
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            terms.push_back(ov + **it);
        }

        return NAryValue(terms, op_).evaluate();
    }

    return NAryValue(clone(), ov.clone(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator-(AbstractValue& av)
{
    return *this + *timesMinusOne(av);
}

std::unique_ptr<AbstractValue> NAryValue::operator-(IntegerValue& iv)
{
    return *this + *timesMinusOne(iv);
}

std::unique_ptr<AbstractValue> NAryValue::operator-(SymbolValue& sv)
{
    return *this + *timesMinusOne(*sv.evaluate());
}

std::unique_ptr<AbstractValue> NAryValue::operator-(InfinityValue& lv)
{
    if (lv.getSign() == Sign::Positive)
        return std::make_unique<InfinityValue>(Sign::Negative);
    return std::make_unique<InfinityValue>(Sign::Positive);
}

std::unique_ptr<AbstractValue> NAryValue::operator-(NAryValue& lv)
{
    return *this + *timesMinusOne(lv);
}

std::unique_ptr<AbstractValue> NAryValue::operator-(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator-(UnaryValue& ov)
{
    return *this + *timesMinusOne(ov);
}

std::unique_ptr<AbstractValue> NAryValue::operator*(AbstractValue& av)
{
    return av * *this;
}

std::unique_ptr<AbstractValue> NAryValue::operator*(IntegerValue& iv)
{
    if (iv.getValue() == 0)
        return IntegerValue(0).evaluate();

    if (op_ == Operation::Multiplication) {
        std::unique_ptr<AbstractValue> tmp = clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(iv.clone());
        return nvTmp->evaluate();
    } else if (op_ == Operation::Addition) {
        list<AV> terms;
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            terms.push_back(iv * **it);
        }

        return NAryValue(terms, op_).evaluate();
    } else if (op_ == Operation::Division) {
        AV n = terms_.front()->clone();

        AV newValue = *n * iv;

        return *newValue / *terms_.back()->clone();
    } else if (op_ == Operation::Maximum || op_ == Operation::Minimum) {
        list<AV> terms;
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            terms.push_back(iv * **it);
        }

        if (iv.getValue() > 0) {
            return NAryValue(terms, op_).evaluate();
        } else {
            if (op_ == Operation::Maximum) {
                return NAryValue(terms, Operation::Minimum).evaluate();
            } else {
                return NAryValue(terms, Operation::Maximum).evaluate();
            }
        }
    }

    return NAryValue(clone(), iv.clone(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator*(SymbolValue& sv)
{
    return *this * *sv.evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator*(InfinityValue& lv)
{
    alertUndefined(*this, "*", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator*(NAryValue& nv)
{
    if (op_ == Division && nv.op_ == Division) {
        AV n1 = terms_.front()->clone();
        AV n2 = nv.terms_.front()->clone();
        AV d1 = terms_.back()->clone();
        AV d2 = nv.terms_.back()->clone();

        AV n = *n1 * *n2;
        AV d = *d1 * *d2;

        return *n / *d;
    }

    if (op_ == Division) {
        AV av = terms_.front()->clone();

        return *(*av * nv) / *terms_.back();
    }

    if (nv.op_ == Division) {
        AV av = nv.terms_.front()->clone();

        return *(*av * *this) / *nv.terms_.back();
    }

    if (op_ == Operation::Multiplication) {
        std::unique_ptr<AbstractValue> tmp = clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(nv.clone());

        return nvTmp->evaluate();
    } else if (nv.op_ == Operation::Multiplication) {
        std::unique_ptr<AbstractValue> tmp = nv.clone();
        NAryValue* nvTmp = static_cast<NAryValue*>(tmp.get());
        nvTmp->add(clone());

        return nvTmp->evaluate();
    }

    return NAryValue(clone(), nv.clone(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator*(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator*(UnaryValue& ov)
{
    return NAryValue(clone(), ov.clone(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator/(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this / iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this / *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this / lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this / nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this / uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this / ov);
    }
    default:
        break;
    }
    r(toString());
    r(av.toString());
    std::cout << "ERROR: IntegerValue::operator/(AbstractValue& av)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> NAryValue::operator/(IntegerValue& iv)
{
    if (iv.getValue() == 1)
        return clone();

    if (iv.getValue() == 0) {
        alertUndefined(*this, "/", iv);
        return UndefinedValue().evaluate();
    }

    if (op_ == Division) {
        AV d = terms_.back()->clone();
        d = *d * iv;

        return *terms_.front() / *d;
    }

    return NAryValue(clone(), iv.clone(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator/(SymbolValue& sv)
{
    return *this / *sv.evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator/(InfinityValue& lv)
{
    std::make_unique<IntegerValue>(0);
}

std::unique_ptr<AbstractValue> NAryValue::operator/(NAryValue& nv)
{
    if (nv.op_ == Division) {
        AV n = nv.terms_.back()->clone();
        AV d = nv.terms_.front()->clone();

        return *this * *(*n / *d);
    } else if (op_ == Division) {
        AV d = terms_.back()->clone();
        d = *d * nv;

        return NAryValue(terms_.front()->clone(), move(d), Division).evaluate();
    }

    return NAryValue(clone(), nv.clone(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator/(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator/(UnaryValue& ov)
{
    return NAryValue(clone(), ov.clone(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(AbstractValue &av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return (*this >> iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return (*this >> *sv);
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return (*this >> lv);
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return (*this >> nv);
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this >> uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this >> ov);
    }
    default:
        break;
    }
    std::cout << "ERROR: NAryValue::operator>>(AbstractValue &)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(IntegerValue &iv)
{
    IntegerValue q(1 << iv.getValue());

    return *this / q;
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(SymbolValue &sv)
{
    return *this >> *sv.evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(InfinityValue &lv)
{
    alertUndefined(*this, ">>", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(NAryValue &nv)
{
    return std::make_unique<NAryValue>(clone(), nv.clone(), ShiftRight);
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator>>(UnaryValue& ov)
{
    return NAryValue(clone(), ov.clone(), Operation::ShiftRight).evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(AbstractValue &av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = dynamic_cast<IntegerValue&>(av);
        return (*this << iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return (*this << *sv);
    }
    case KInfinity: {
        InfinityValue& lv = dynamic_cast<InfinityValue&>(av);
        return (*this << lv);
    }
    case KNAry: {
        NAryValue& nv = dynamic_cast<NAryValue&>(av);
        return (*this << nv);
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this << uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this << ov);
    }
    default:
        break;
    }
    std::cout << "ERROR: NAryValue::operator<<(AbstractValue &)" << std::endl;
    exit(1);
    return nullptr;
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(IntegerValue &iv)
{
    IntegerValue q(1 << iv.getValue());

    return *this * q;
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(SymbolValue &sv)
{
    return *this << *sv.evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(InfinityValue &lv)
{
    alertUndefined(*this, "<<", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(NAryValue &nv)
{
    return std::make_unique<NAryValue>(clone(), nv.clone(), ShiftLeft);
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> NAryValue::operator<<(UnaryValue& ov)
{
    return NAryValue(clone(), ov.clone(), Operation::ShiftLeft).evaluate();
}

bool NAryValue::operator==(AbstractValue& av)
{
    return av == *this;
}

bool NAryValue::operator==(IntegerValue &)
{
    return false;
}

bool NAryValue::operator==(SymbolValue& sv)
{
    return  sv == *this;
}

bool NAryValue::operator==(InfinityValue &)
{
    return false;
}

bool NAryValue::operator==(NAryValue& lv)
{
    if (op_ != lv.op_ || lv.terms_.size() != terms_.size()) {
        return false;
    }

    if (op_ == Operation::Addition || op_ == Operation::Multiplication ||
        op_ == Operation::Minimum || op_ == Operation::Maximum) {
            std::unique_ptr<AbstractValue> aOp1 = this->clone();
            std::unique_ptr<AbstractValue> aOp2 = lv.clone();
            NAryValue* op1 = static_cast<NAryValue*>(aOp1.get());
            NAryValue* op2 = static_cast<NAryValue*>(aOp2.get());

            return removeEqual(op1, op2);
    } else if (op_ == Operation::Division || op_ == Operation::ShiftLeft ||
             op_ == Operation::ShiftRight) {
        AV n1Term1 = terms_.front()->clone();
        AV n1Term2 = terms_.back()->clone();

        AV n2Term1 = lv.terms_.front()->clone();
        AV n2Term2 = lv.terms_.back()->clone();

        return (*n1Term1 == *n2Term1) && (*n1Term2 == *n2Term2);
    } else
        return false;
}

bool NAryValue::operator==(UndefinedValue &)
{
    return false;
}

bool NAryValue::operator==(UnaryValue &)
{
    return false;
}

bool NAryValue::operator<(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this < *sv;
    }
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return *this < iv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this < lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this < nv;
    }
    case KUndefined: {
        UndefinedValue& uv = dynamic_cast<UndefinedValue&>(av);
        return (*this < uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this < ov);
    }
    default:
        std::cout << "ERROR: NAryValue::operator<(AbstractValue& av):: bad kind of value!" << std::endl;
        exit(1);
        break;
    }
    return false;
}

bool NAryValue::operator<(IntegerValue& iv)
{
    if (op_ == Division) {
        if (integerIsGreaterThanNAryDivision(iv, *this) == 1)
            return true;
        else
            return false;
    }

    return false;
}

bool NAryValue::operator<(SymbolValue& sv)
{
    return  *this < *sv.evaluate();
}

bool NAryValue::operator<(InfinityValue& lv)
{
    return lv.getSign() == Sign::Positive;
}

bool NAryValue::operator<(NAryValue& lv) 
{
    IntegerValue zero(0);
    std::unique_ptr<AbstractValue> aOp1;
    if (op_ != Addition)
        aOp1 = NAryValue(clone(), zero.clone(), Addition).clone();
    else
        aOp1 = this->clone();

    std::unique_ptr<AbstractValue> aOp2;
    if (lv.op_ != Addition)
        aOp2 = NAryValue(lv.clone(), zero.clone(), Addition).clone();
    else
        aOp2 = lv.clone();

    NAryValue* op1 = static_cast<NAryValue*>(aOp1.get());
    NAryValue* op2 = static_cast<NAryValue*>(aOp2.get());

    // verify if they are the same
    bool equal;
    if (op1->terms_.size() > op2->terms_.size())
        equal = removeAllEqual(op2, op1);
    else
        equal = removeAllEqual(op1, op2);

    if (equal)
        return false;

    // if they differ just by those integers return the comparation of the integers
    if (op1->terms_.size() <= 1 && op2->terms_.size() <= 1) {
        std::unique_ptr<AbstractValue> n1;
        std::unique_ptr<AbstractValue> n2;

        if (op1->terms_.size() == 0)
            n1 = IntegerValue(0).clone();
        else
            n1 = std::move(op1->terms_.back());

        if (op2->terms_.size() == 0)
            n2 = IntegerValue(0).clone();
        else
            n2 = std::move(op2->terms_.back());

        if (n1->getKindOfValue() == KindOfValue::KInteger && n2->getKindOfValue() == KindOfValue::KInteger)
            return *n1 < *n2;
    }

    return false;
}

bool NAryValue::operator<(UndefinedValue &)
{
    return false;
}

bool NAryValue::operator<(UnaryValue &)
{
    return false;
}

bool NAryValue::operator>(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this > *sv;
    }
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return *this > iv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this > lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this > nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return *this > uv;
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return *this > ov;
    }
    default:
        r(av.toString());
        cout << "ERROR: NAryValue::operator>(AbstractValue& av): bad kind of value!" << endl;
        exit(1);
        break;
    }
    return false;
}

bool NAryValue::operator>(IntegerValue& iv)
{
    if (op_ == Division) {
        if (integerIsGreaterThanNAryDivision(iv, *this) == 2)
            return true;
        else
            return false;
    }

    return false;
}

bool NAryValue::operator>(SymbolValue& sv)
{
    return  *this > *sv.evaluate();
}

bool NAryValue::operator>(InfinityValue& lv)
{
    return lv.getSign() == Sign::Negative;
}

bool NAryValue::operator>(NAryValue& lv)
{
    IntegerValue zero(0);
    std::unique_ptr<AbstractValue> aOp1;
    if (op_ != Addition)
        aOp1 = NAryValue(clone(), zero.clone(), Addition).clone();
    else
        aOp1 = this->clone();

    std::unique_ptr<AbstractValue> aOp2;
    if (lv.op_ != Addition)
        aOp2 = NAryValue(lv.clone(), zero.clone(), Addition).clone();
    else
        aOp2 = lv.clone();

    NAryValue* op1 = static_cast<NAryValue*>(aOp1.get());
    NAryValue* op2 = static_cast<NAryValue*>(aOp2.get());

    // verify if they are the same
    bool equal;
    if (op1->terms_.size() > op2->terms_.size())
        equal = removeAllEqual(op2, op1);
    else
        equal = removeAllEqual(op1, op2);

    if (equal)
        return false;

    // if they differ just by those integers return the comparation of the integers
    if (op1->terms_.size() <= 1 && op2->terms_.size() <= 1) {
        std::unique_ptr<AbstractValue> n1;
        std::unique_ptr<AbstractValue> n2;

        if (op1->terms_.size() == 0)
            n1 = IntegerValue(0).clone();
        else
            n1 = std::move(op1->terms_.back());

        if (op2->terms_.size() == 0)
            n2 = IntegerValue(0).clone();
        else
            n2 = std::move(op2->terms_.back());

        if (n1->getKindOfValue() == KindOfValue::KInteger && n2->getKindOfValue() == KindOfValue::KInteger)
            return *n1 > *n2;
    }

    return false;
}

bool NAryValue::operator>(UndefinedValue &)
{
    return false;
}

bool NAryValue::operator>(UnaryValue &)
{
    return false;
}

bool NAryValue::isDiscrete()
{
    if (op_ == Modulo || op_ == Minimum || op_ == Maximum)
        return true;
    return false;
}

bool NAryValue::isShift()
{
    if (op_ == ShiftRight || op_ == ShiftLeft)
        return true;
    return false;
}

// return true if the NAryValue is a multiplication of 1 by an symbol
bool NAryValue::isTimesOne()
{
    if ( (op_ == Multiplication) && (terms_.size() == 2) ) {
        AV aCopy = clone();
        NAryValue* rCopy = static_cast<NAryValue*>(aCopy.get());
        auto integer = extractIntegerFromNAry(*rCopy);
        if (integer.first && integer.second == 1 &&
                rCopy->terms_.back()->getKindOfValue() == KSymbol)
            return true;
    }

    return false;
}

// Print Range

namespace psyche {

std::ostream& operator<<(std::ostream& os, const Range& range)
{
    std::string l = range.lower_->toString();
    std::string u = range.upper_->toString();
    //std::string l = range.lower_->evaluate()->toString();
    //std::string u = range.upper_->evaluate()->toString();
    auto show = [&os] (std::string value, char sign) {
        if (value.empty())
            os << sign << std::numeric_limits<double>::infinity();
        else
            os << value;
    };
    os << "[";
    show(l, '-');
    os << ", ";
    show(u, '+');
    os << "]";
    return os;
}

std::unique_ptr<AbstractValue> AbstractValue::operator+(int v)
{
    IntegerValue iv(v);
    return *this + iv;
}

std::unique_ptr<AbstractValue> AbstractValue::operator-(int v)
{
    IntegerValue iv((-1)*v);
    return *this + iv;
}

unsigned Simbol::cnt_ = 0;

Simbol::Simbol(const char* name, CPlusPlus::TranslationUnit* p)
    : id_(new Identifier(name, sizeof(name)))
    , program_(p)
    , Symbol(program_, cnt_++, id_)
{
    setName(id_);
}

Simbol::~Simbol()
{
    delete id_;
}

// NAryValue::simplify

void NAryValue::simplify()
{
    switch (op_) {
    case Addition:
        simplifyAdd();
        break;
    case Multiplication:
        simplifyMul();
        break;
    case Division:
        simplifyDiv();
        break;
    case Minimum:
        simplifyMin();
        break;
    case Maximum:
        simplifyMax();
        break;
    case ShiftRight:
    case ShiftLeft:
        break;
    default:
        std::cout << "bad simplification choice:" << std::endl;
        printOperation(op_);
        exit(1);
        break;
    }
}

void NAryValue::simplifyAdd()
{
    simplifyInt();
    simplifyTerms();
}

void NAryValue::simplifyMul()
{
    simplifyInt(); // multiply all integers in the nary value. Ex.: n*4*5 = n*10
    simplifySumsInMul(); // distribute the multiplication if some term is an addition
}

// distribute the multiplication if some term is an addition
void NAryValue::simplifySumsInMul()
{
    // (a + b) * c = ac + bc
    // (a + b) * (c + d) = ac + ad + bc + bd
    if (op_ != Operation::Multiplication)
        return;

    list<AV> filtered;
    list<AV> remainingTerms;
    for (auto it = terms_.begin(); it != terms_.end(); it++) {
        if ((*it)->getKindOfValue() == KNAry) {
            const NAryValue* nv = static_cast<NAryValue*>((*it).get());
            if (nv->op_ == Operation::Addition) {
                filtered.push_back((*it)->clone());
                continue;
            }
        }

        remainingTerms.push_back((*it)->clone());
    }

    if (filtered.size() == 0)
        return;

    // ex.: (a + b) * (c + d) * e * min(f, g)
    // filter all additions
    // let filtered = [(a + b), (c + d)], remaining = (e*min(f, g))
    NAryValue remaining(remainingTerms, Operation::Multiplication);
    remainingTerms.clear();

    // multiply each pair of term in each number
    // tmp = ac + ad + bc + bd
    AV tmp;
    if (filtered.size() > 0) {
        tmp = filtered.front()->clone();
        filtered.pop_front();
    } else {
        list<AV> tmpList;
        tmpList.push_back(IntegerValue(1).clone());
        tmp = NAryValue(tmpList, Operation::Addition).clone();
    }
    list<AV> tempTerms;
    while (!filtered.empty()) {
        AV tmp2 = filtered.front()->clone();
        filtered.pop_front();

        NAryValue* add1 = static_cast<NAryValue*>(tmp.get());
        NAryValue* add2 = static_cast<NAryValue*>(tmp2.get());
        for (const auto &it : add1->terms_) { // for each n1 in add1
            for (const auto &it2 : add2->terms_) { // for each n2 in add2
                tempTerms.push_back(*it * *it2); // multiply each n1 with n2
            }
        }

        tmp = NAryValue(tempTerms, Operation::Addition).clone(); // save the current number
        tempTerms.clear();
    }

    // then multiply each resulting term by remaining ones
    // return remaining*tmp
    terms_.clear();
    NAryValue* add = static_cast<NAryValue*>(tmp.get());

    for (const auto &it : add->terms_) {
        AV av = *it * remaining;
        terms_.push_back(move(av));
    }
    op_ = Operation::Addition;
}

void NAryValue::simplifyDiv()
{
    if (terms_.size() != 2) {
        r("ERROR: NAryValue::simplifyDiv(): too many values!");
        exit(1);
    }

    // numerator equals to the denominator
    if (*terms_.front() == *terms_.back()) {
        terms_.clear();
        terms_.push_back(make_unique<IntegerValue>(1));
        return;
    }

    // numerator equals to one
    if (*terms_.back() == *IntegerValue(1).clone()) {
        terms_.pop_back();
        return;
    }

    AV n = terms_.front()->evaluate();
    AV d = terms_.back()->evaluate();

    KindOfValue k1 = n->getKindOfValue();
    KindOfValue k2 = d->getKindOfValue();

    if (k1 == KNAry && k2 == KNAry) {
        NAryValue* n1 = static_cast<NAryValue*>(n.get());
        NAryValue* n2 = static_cast<NAryValue*>(d.get());

        Operation op1 = n1->op_, op2 = n2->op_;
        if (op1 == Multiplication && op2 == Multiplication) {
            // try to simplify common terms in n1 and n2
            if(n1->terms_.size() > n2->terms_.size()) {
                simplifyCommonSymbols(*n2, *n1);
            } else {
                simplifyCommonSymbols(*n1, *n2);
            }
            simplifyIntegersInDivision(*n1, *n2);
            goto END_OF_SIMPLIFICATIONS;
        } else if (op1 == Multiplication) {
            // try to simplify n1 with the value in n2
            for (auto it = n1->terms_.begin(); it != n1->terms_.end(); it++) {
                if (*n2 == **it) {
                    n1->terms_.erase(it);
                    d = IntegerValue(1).evaluate();
                    goto END_OF_SIMPLIFICATIONS;
                }
            }

            if (op2 == Addition) {
                // we can try to extract common terms in n1 and then try
                // to simplify it with some term in n2
                // (3*a*b + 12*a*d)/(6*a) = (b + 4*d)/2
            }
            goto END_OF_SIMPLIFICATIONS;
        } else if (op2 == Multiplication) {
            // if n1 is equal to some term in n2 we can simplify it
            // (s+1)/(s2*(s+1)) = 1/s2
            for (auto it = n2->terms_.begin(); it != n2->terms_.end(); it++) {
                if (*n1 == **it) {
                    n2->terms_.erase(it);
                    n = IntegerValue(1).evaluate();
                    goto END_OF_SIMPLIFICATIONS;
                }
            }

            if (op1 == Addition) {
                // we can try to extract common terms in n1 and then try
                // to simplify it with some term in n2
                // (3*a*b + 12*a*d)/(6*a) = (b + 4*d)/2
            }
            goto END_OF_SIMPLIFICATIONS;
        }
        goto END_OF_SIMPLIFICATIONS;
    } else if (k1 == KNAry) {
        // try to simplify n1 with the value in n2
        NAryValue* n1 = static_cast<NAryValue*>(n.get());
        if (n1->isDiscrete() || n1->isShift())
            goto END_OF_SIMPLIFICATIONS;

        if (k2 == KInteger) {
            IntegerValue* n2 = static_cast<IntegerValue*>(d.get());
            int num2 = n2->getValue();

            if (num2 == 1) { // x/1 = x
                goto END_OF_SIMPLIFICATIONS;
            }

            if (num2 == 0) { // x/0 = Undefined
                n = UndefinedValue().evaluate();
                d = UndefinedValue().evaluate();

                goto END_OF_SIMPLIFICATIONS;
            }

            // if the denominator < 0, multiple the division by ((-1)/(-1))
            if (num2 < 0) {
                n = IntegerValue(-1) * *n;
                d = IntegerValue(std::abs(num2)).evaluate();

                goto END_OF_SIMPLIFICATIONS;
            }

            if (n1->op_ == Multiplication) {
                auto integerInN1 = extractIntegerFromNAry(*n1);
                bool find1 = integerInN1.first;
                int num1;
                if (!find1) num1 = 1;
                else num1 = integerInN1.second;

                int gcdValue = gcd(num1, num2);
                IntegerValue iv1(num1/gcdValue);
                n = *n * iv1;
                d = IntegerValue(num2/gcdValue).evaluate();

                goto END_OF_SIMPLIFICATIONS;
            }
        }

        goto END_OF_SIMPLIFICATIONS;
    } else if (k2 == KNAry) {
        // try to simplify n2 with the value in n1
        NAryValue* n2 = static_cast<NAryValue*>(d.get());

        if (n2->isDiscrete() || n2->isShift())
            goto END_OF_SIMPLIFICATIONS;

        if (k1 == KInteger) {
            IntegerValue* n1 = static_cast<IntegerValue*>(n.get());
            int num1 = n1->getValue();

            if (num1 == 0) {
                n = IntegerValue(0).evaluate();

                goto END_OF_SIMPLIFICATIONS;
            }

            if (n2->op_ == Multiplication) {
                auto integerInN2 = extractIntegerFromNAry(*n2);
                bool find2 = integerInN2.first;
                int num2;
                if (!find2) num2 = 1;
                else num2 = integerInN2.second;

                int gcdValue = gcd(num1, num2);
                n = IntegerValue(num1/gcdValue).evaluate();
                d = *IntegerValue(num2/gcdValue).evaluate() * *d;

                goto END_OF_SIMPLIFICATIONS;
            }
        }

        goto END_OF_SIMPLIFICATIONS;
    } else if (k1 == KInteger && k2 == KInteger) {
        // try to simplify n1 and n2 with the maximum common divisor between them

        // Extract the abstract values
        IntegerValue* i1 = static_cast<IntegerValue*>(n.get());
        IntegerValue* i2 = static_cast<IntegerValue*>(d.get());

        // find the greatest common divisor between them
        int in = i1->getValue();
        int id = i2->getValue();

        if (id == 1) {
            goto END_OF_SIMPLIFICATIONS;
        }
        // if the denominator < 0, multiple the division by ((-1)/(-1))
        if (id < 0) {
            n = IntegerValue(-1*in).clone();
            d = IntegerValue(-1*id).clone();

            goto END_OF_SIMPLIFICATIONS;
        }

        int gcdValue = gcd(in, id);
        if (gcdValue != 0) {
            in /= gcdValue;
            id /= gcdValue;
        }

        n = IntegerValue(in).clone();
        d = IntegerValue(id).clone();

        goto END_OF_SIMPLIFICATIONS;
    }
    END_OF_SIMPLIFICATIONS:
    terms_.clear();
    terms_.push_front(n->evaluate());
    terms_.push_back(d->evaluate());
}

void NAryValue::simplifyMin()
{
    simplifyRemoveMinMin();
    simplifyMaxMin();
    simplifyInf(Positive);
    simplifyEq();
    simplifyRemoveLargers();
}

void NAryValue::simplifyMax()
{
    simplifyRemoveMaxMax();
    simplifyMaxMin();
    simplifyInf(Negative);
    simplifyEq();
    simplifyRemoveMinors();
}

void NAryValue::simplifyInt()
// 2 + 2 + 3 = 7
// 2 * 2 * 3 = 12
{
    std::unique_ptr<IntegerValue> aux;
    if (op_ == Addition)
        aux = std::make_unique<IntegerValue>(0);
    else if (op_ == Multiplication)
        aux = std::make_unique<IntegerValue>(1);

    for (auto it = terms_.begin(); it != terms_.end();) {
        if ((*it)->getKindOfValue() == KInteger) {
            std::unique_ptr<AbstractValue> term = (*it)->clone();
            IntegerValue *iv = static_cast<IntegerValue*>(term.get());
            if (op_ == Addition)
                aux = std::make_unique<IntegerValue>(aux->getValue() + iv->getValue());
            else if (op_ == Multiplication)
                aux = std::make_unique<IntegerValue>(aux->getValue() * iv->getValue());
            it = terms_.erase(it);
        } else {
            it++;
        }
    }

    if (op_ == Addition && aux->getValue() == 0 && terms_.empty()) // when the result is zero we put it in the list as result of the simplification
        add(IntegerValue(0).evaluate());
    if (op_ == Addition && aux->getValue() != 0) // add integer, different than zero, to the list
        add(aux->clone());
    if (op_ == Multiplication) //if (op_ == Multiplication && aux->getValue() != 1)
        add(aux->clone());
}

// simplification used to sum the coefficients of same  variables/narys
// 2a + 3 + a = 3a + 3
void NAryValue::simplifyTerms()
{
    std::list<pair<std::unique_ptr<AbstractValue>,
            std::unique_ptr<AbstractValue>>> terms; // integer*nary -> [<integer, nary>]

    for (auto it = terms_.begin(); it != terms_.end(); ) {
        if ((*it)->getKindOfValue() == KNAry) {
            NAryValue *nv = static_cast<NAryValue*>((*it).get());
            if (nv->op_ == Multiplication) {
                std::unique_ptr<AbstractValue> iv;
                bool findOneIntegerValue = false; // if we don't find an coefficient we create one
                for (auto it2 = nv->terms_.begin(); it2 != nv->terms_.end();) {
                    if ((*it2)->getKindOfValue() == KInteger) {
                        iv = (*it2)->clone();
                        nv->terms_.erase(it2);
                        findOneIntegerValue = true;
                        break;
                    } else {
                        it2++;
                    }
                }
                if (!findOneIntegerValue) {
                    iv = make_unique<IntegerValue>(1);
                }
                terms.push_back(make_pair<std::unique_ptr<AbstractValue>,
                                std::unique_ptr<AbstractValue>>(move(iv), (*it)->evaluate()));
                it = terms_.erase(it);
            } else
                it++;
        } else
            it++;
    }

    SIMPLIFY:
    for (auto it = terms.begin(); it != terms.end(); it++) {
        for (auto it2 = terms.begin(); it2 != terms.end(); it2++) {
            if (it != it2 && *it->second == *it2->second) {
                terms.push_back(make_pair<std::unique_ptr<AbstractValue>,
                                std::unique_ptr<AbstractValue>>(*it->first + *it2->first,
                                                                move(it->second)));
                terms.erase(it);
                terms.erase(it2);
                goto SIMPLIFY;
            }
        }
    }

    for (const auto& it : terms) {
        std::unique_ptr<AbstractValue> x = *it.first * *it.second;
        terms_.push_back(*it.first * *it.second);
    }
}

void NAryValue::simplifyInf(Sign sign)
{
    for (auto it = terms_.begin(); it != terms_.end();) {
        if (it->get()->getKindOfValue() == KInfinity) {
            InfinityValue *lv = static_cast<InfinityValue*>(it->get());
            if (lv->getSign() == sign) {
                it = terms_.erase(it);
            } else {
                it++;
            }
        } else {
            it++;
        }
    }
}

void NAryValue::removeTimesOne()
{
    if (op_ == Multiplication && terms_.size() > 1) {
        // remove the term '1' if there is one
        SIMPLIFICATION:
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            if ((*it)->getKindOfValue() == KInteger) {
                IntegerValue* iv = static_cast<IntegerValue*>((*it).get());
                if (iv->getValue() == 1) {
                    terms_.erase(it);
                    goto SIMPLIFICATION;
                }
            }
        }
    }

    // call the simplification for the other NAry values
    for (auto it = terms_.begin(); it != terms_.end(); it++) {
        if ((*it)->getKindOfValue() == KNAry) {
            NAryValue* nv = static_cast<NAryValue*>((*it).get());
            nv->removeTimesOne();
        }
    }
}

void NAryValue::removeSumToZero()
{
    if (op_ == Addition) {
        // remove the zeros if they exists
        SIMPLIFICATION:
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            if ((*it)->getKindOfValue() == KInteger) {
                IntegerValue* iv = static_cast<IntegerValue*>((*it).get());
                if (iv->getValue() == 0) {
                    terms_.erase(it);
                    goto SIMPLIFICATION;
                }
            }
        }
    }

    // call the simplification for the other NAry values
    for (auto it = terms_.begin(); it != terms_.end(); it++) {
        if ((*it)->getKindOfValue() == KNAry) {
            NAryValue* nv = static_cast<NAryValue*>((*it).get());
            nv->removeSumToZero();
        }
    }
}

void NAryValue::simplifyEq()
{
    for (auto it = terms_.begin(); it != terms_.end(); it++) {
        for (auto it2 = std::next(it); it2 != terms_.end();) {
            if (*it->get() == *it2->get()) {
                it2 = terms_.erase(it2);
            } else {
                it2++;
            }
        }
    }
}

void NAryValue::simplifyRemoveMinors()
{
    SIMPLIFY:
    for (auto i = terms_.begin(); i != terms_.end(); i++) {
        for (auto j = terms_.begin(); j != terms_.end(); j++) {
            if (i == j)
                continue;
            if (*(i->get()) < *(j->get())) {
                terms_.erase(i);
                goto SIMPLIFY;
            }
        }
    }
}

void NAryValue::simplifyRemoveMaxMax()
{
    if (op_ == Maximum) {
        SIMPLIFY:
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            AV& av = *it;
            if (av->getKindOfValue() == KNAry) {
                NAryValue* nv = static_cast<NAryValue*>(av.get());
                if (nv->op_ == Maximum) {
                    for (auto it2 = nv->terms_.begin(); it2 != nv->terms_.end(); it2++) {
                        add(move(*it2));
                    }
                    terms_.erase(it);
                    goto SIMPLIFY;
                }
            }
        }
    }
}

void NAryValue::simplifyMaxMin()
{
    if (terms_.size() == 2) {
        if (op_ == Operation::Maximum) {
            if (terms_.front()->getKindOfValue() == KNAry) {
                NAryValue *nv = static_cast<NAryValue*>(terms_.front().get());

                if (nv->terms_.size() == 2 &&
                        nv->op_ == Operation::Minimum) {
                    // max(min(a, b), c) = c ; if c == a || c == b
                    const std::unique_ptr<AbstractValue>& a = nv->terms_.front();
                    const std::unique_ptr<AbstractValue>& b = nv->terms_.back();
                    const std::unique_ptr<AbstractValue>& c = terms_.back();

                    if (*a == *c) {
                        terms_.pop_front();
                        return;
                    } else if (*b == *c) {
                        terms_.pop_front();
                        return;
                    }
                }
            }

            if (terms_.back()->getKindOfValue() == KNAry) {
                NAryValue *nv = static_cast<NAryValue*>(terms_.back().get());

                if (nv->terms_.size() == 2 &&
                        nv->op_ == Operation::Minimum) {
                    // max(a, min(b, c))
                    const std::unique_ptr<AbstractValue>& a = terms_.front();
                    const std::unique_ptr<AbstractValue>& b = nv->terms_.front();
                    const std::unique_ptr<AbstractValue>& c = nv->terms_.back();

                    if (*a == *c) {
                        terms_.pop_back();
                        return;
                    } else if (*b == *a) {
                        terms_.pop_back();
                        return;
                    }
                }
            }
        } else if (op_ == Operation::Minimum) {
            if (terms_.front()->getKindOfValue() == KNAry) {
                NAryValue *nv = static_cast<NAryValue*>(terms_.front().get());

                if (nv->terms_.size() == 2 &&
                        nv->op_ == Operation::Maximum) {
                    // min(max(a, b), c)
                    const std::unique_ptr<AbstractValue>& a = nv->terms_.front();
                    const std::unique_ptr<AbstractValue>& b = nv->terms_.back();
                    const std::unique_ptr<AbstractValue>& c = terms_.back();

                    if (*a == *c) {
                        terms_.pop_front();
                        return;
                    } else if (*b == *c) {
                        terms_.pop_front();
                        return;
                    }
                }
            }

            if (terms_.back()->getKindOfValue() == KNAry) {
                NAryValue *nv = static_cast<NAryValue*>(terms_.back().get());

                if (nv->terms_.size() == 2 &&
                        nv->op_ == Operation::Maximum) {
                    // min(a, max(b, c))
                    const std::unique_ptr<AbstractValue>& a = terms_.front();
                    const std::unique_ptr<AbstractValue>& b = nv->terms_.front();
                    const std::unique_ptr<AbstractValue>& c = nv->terms_.back();

                    if (*a == *c) {
                        terms_.pop_back();
                        return;
                    } else if (*b == *a) {
                        terms_.pop_back();
                        return;
                    }
                }
            }
        }
    }
}

void NAryValue::simplifyRemoveLargers()
{
    SIMPLIFY:
    for (auto i = terms_.begin(); i != terms_.end(); i++) {
        for (auto j = terms_.begin(); j != terms_.end(); j++) {
            if (i == j)
                continue;
            if (*(i->get()) > *(j->get())) {
                terms_.erase(i);
                goto SIMPLIFY;
            }
        }
    }
}

void NAryValue::simplifyRemoveMinMin()
{
    if (op_ == Minimum) {
        SIMPLIFY:
        for (auto it = terms_.begin(); it != terms_.end(); it++) {
            AV& av = *it;
            if (av->getKindOfValue() == KNAry) {
                NAryValue* nv = static_cast<NAryValue*>(av.get());
                if (nv->op_ == Minimum) {
                    for (auto it2 = nv->terms_.begin(); it2 != nv->terms_.end(); it2++) {
                        add(move(*it2));
                    }
                    terms_.erase(it);
                    goto SIMPLIFY;
                }
            }
        }
    }
}

// Undefined Value

std::unique_ptr<AbstractValue> UndefinedValue::clone() const
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::evaluate()
{
    return clone();
}

string UndefinedValue::toString()
{
    return "Undefined";
}

KindOfValue UndefinedValue::getKindOfValue()
{
    return kindOfValue_;
}

std::set<std::unique_ptr<AbstractValue> > UndefinedValue::asSet()
{
    std::set<std::unique_ptr<AbstractValue> > s;
    s.insert(clone());
    return s;
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(AbstractValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(IntegerValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(SymbolValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(InfinityValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(NAryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator+(UnaryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(AbstractValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(IntegerValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(SymbolValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(InfinityValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(NAryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator-(UnaryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(AbstractValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(IntegerValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(SymbolValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(InfinityValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(NAryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator*(UnaryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(AbstractValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(IntegerValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(SymbolValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(InfinityValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(NAryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator/(UnaryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(AbstractValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(IntegerValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(SymbolValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(InfinityValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(NAryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator>>(UnaryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(AbstractValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(IntegerValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(SymbolValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(InfinityValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(NAryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UndefinedValue::operator<<(UnaryValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> AbstractValue::operator+(UndefinedValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> AbstractValue::operator-(UndefinedValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> AbstractValue::operator*(UndefinedValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> AbstractValue::operator/(UndefinedValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> AbstractValue::operator<<(UndefinedValue &)
{
    return make_unique<UndefinedValue>();
}

std::unique_ptr<AbstractValue> AbstractValue::operator>>(UndefinedValue &)
{
    return make_unique<UndefinedValue>();
}

// UnaryValue

std::unique_ptr<AbstractValue> UnaryValue::clone() const
{
    return make_unique<UnaryValue>(value_->clone(), op_);
}

string UnaryValue::toString()
{
    return "sqrt(" + value_->toString() + ")";
}

string UnaryValue::toCCode()
{
    return "msqrt(" + value_->toCCode() + ")";
}

KindOfValue UnaryValue::getKindOfValue()
{
    return kindOfValue_;
}

std::unique_ptr<AbstractValue> UnaryValue::evaluate()
{
    AV av = value_->evaluate();

    if (av->getKindOfValue() == KUndefined) {
        alertUndefined(*av, "sqrt()", *IntegerValue(2).clone());
        return UndefinedValue().evaluate();
    }

    if (av->getKindOfValue() == KInfinity) {
        InfinityValue* lv = static_cast<InfinityValue*>(av.get());
        if (lv->getSign() == Positive)
            return lv->clone();
        else {
            alertUndefined(*av, "sqrt()", *IntegerValue(2).clone());
            return UndefinedValue().evaluate();
        }
    }

    if (av->getKindOfValue() == KInteger) {
        IntegerValue* iv = static_cast<IntegerValue*>(av.get());
        if (iv->getValue() >= 0)
            return IntegerValue(sqrt(iv->getValue())).clone();
        else {
            alertUndefined(*av, "sqrt()", *IntegerValue(2).clone());
            return UndefinedValue().evaluate();
        }
    }

    //return *UnaryValue(move(av), op_).clone() * *IntegerValue(1).evaluate();
    return make_unique<UnaryValue>(move(av), op_);
}

std::unique_ptr<AbstractValue> UnaryValue::develop()
{
    return UnaryValue(value_->develop(), op_).evaluate();
}

void UnaryValue::buildSymbolDependence()
{
    symbolDep_.clear();
    value_->buildSymbolDependence();
    for (auto symbol : value_->symbolDep_) {
        symbolDep_.insert(symbol);
    }
}

std::list<std::unique_ptr<AbstractValue> > UnaryValue::termsClone()
{
    list<std::unique_ptr<AbstractValue>> l;
    l.push_back(value_->clone());

    return l;
}

std::set<std::unique_ptr<AbstractValue> > UnaryValue::asSet()
{
    std::set<std::unique_ptr<AbstractValue> > s;
    s.insert(clone());
    return s;
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(AbstractValue& av)
{
    return av + *this;
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(IntegerValue& iv)
{
    return NAryValue(evaluate(), iv.evaluate(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(SymbolValue& sv)
{
    return *this + *sv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(InfinityValue& lv)
{
    return lv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(NAryValue& nv)
{
    if ((op_ == SquareRoot) &&
            nv.op_ == Operation::Minimum || nv.op_ == Operation::Maximum) {
        list<AV> terms;
        for (auto it = nv.terms_.begin(); it != nv.terms_.end(); it++) {
            terms.push_back(*this + **it);
        }

        return NAryValue(terms, nv.op_).evaluate();
    }

    return NAryValue(evaluate(), nv.evaluate(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator+(UnaryValue& ov)
{
    return NAryValue(evaluate(), ov.evaluate(), Operation::Addition).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(AbstractValue& av)
{
    return *this + *timesMinusOne(av);
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(IntegerValue& iv)
{
    return *this + *timesMinusOne(iv);
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(SymbolValue& sv)
{
    return *this - *sv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(InfinityValue& lv)
{
    if (lv.getSign() == Positive)
        return InfinityValue(Negative).evaluate();
    return InfinityValue(Positive).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(NAryValue& nv)
{
    return *this + *timesMinusOne(nv);
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator-(UnaryValue& ov)
{
    return *this + *timesMinusOne(ov);
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(AbstractValue& av)
{
    return av * *this;
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(IntegerValue& iv)
{
    return NAryValue(evaluate(), iv.evaluate(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(SymbolValue& sv)
{
    return *this * *sv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(InfinityValue& lv)
{
    return lv.clone();
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(NAryValue& nv)
{
    return NAryValue(evaluate(), nv.evaluate(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator*(UnaryValue& ov)
{
    return NAryValue(evaluate(), ov.evaluate(), Operation::Multiplication).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this / iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this / *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this / lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this / nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this / uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this / ov);
    }
    default:
        break;
    }

    r("ERROR:: returning null pointer!");
    exit(1);

    return nullptr;
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(IntegerValue& iv)
{
    return NAryValue(evaluate(), iv.evaluate(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(SymbolValue& sv)
{
    return *this / *sv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(InfinityValue &)
{
    return IntegerValue(0).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(NAryValue& nv)
{
    return NAryValue(evaluate(), nv.evaluate(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator/(UnaryValue& ov)
{
    return NAryValue(evaluate(), ov.evaluate(), Operation::Division).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this >> iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this >> *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this >> lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this >> nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this >> uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this >> ov);
    }
    default:
        break;
    }

    r("ERROR:: returning null pointer!");
    exit(1);

    return nullptr;
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(IntegerValue& iv)
{
    IntegerValue q(1 << iv.getValue());

    return *this / q;
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(SymbolValue& sv)
{
    return *this >> *sv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(InfinityValue& lv)
{
    alertUndefined(*this, ">>", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(NAryValue& nv)
{
    return NAryValue(evaluate(), nv.evaluate(), Operation::ShiftRight).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator>>(UnaryValue& ov)
{
    return NAryValue(evaluate(), ov.evaluate(), Operation::ShiftRight).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this << iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this << *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this << lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this << nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this << uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this << ov);
    }
    default:
        break;
    }

    r("ERROR:: returning null pointer!");
    exit(1);

    return nullptr;
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(IntegerValue& iv)
{
    IntegerValue q(1 << iv.getValue());

    return *this * q;
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(SymbolValue& sv)
{
    return *this << *sv.evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(InfinityValue& lv)
{
    alertUndefined(*this, "<<", lv);
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(NAryValue& nv)
{
    return NAryValue(evaluate(), nv.evaluate(), Operation::ShiftLeft).evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(UndefinedValue &)
{
    return UndefinedValue().evaluate();
}

std::unique_ptr<AbstractValue> UnaryValue::operator<<(UnaryValue& ov)
{
    return NAryValue(evaluate(), ov.evaluate(), Operation::ShiftLeft).evaluate();
}

bool UnaryValue::operator==(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this == iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this == *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this == lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this == nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this == uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this == ov);
    }
    default:
        break;
    }

    return false;
}

bool UnaryValue::operator==(IntegerValue &)
{
    return false;
}

bool UnaryValue::operator==(SymbolValue &)
{
    return false;
}

bool UnaryValue::operator==(InfinityValue &)
{
    return false;
}

bool UnaryValue::operator==(NAryValue &)
{
    return false;
}

bool UnaryValue::operator==(UndefinedValue &)
{
    return false;
}

bool UnaryValue::operator==(UnaryValue& ov)
{
    if (op_ == ov.op_) {
        if (op_ == SquareRoot)
            return *value_ == *ov.value_;
    }

    return false;
}

bool UnaryValue::operator<(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this < iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this < *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this < lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this < nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return *this < uv;
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return *this < ov;
    }
    default:
        break;
    }

    return false;
}

bool UnaryValue::operator<(IntegerValue& iv)
{
    if (op_ == SquareRoot && iv.getValue() < 0)
        return false;

    return false;
}

bool UnaryValue::operator<(SymbolValue &)
{
    return false;
}

bool UnaryValue::operator<(InfinityValue& lv)
{
    if (lv.getSign() == Positive)
        return true;
    return false;
}

bool UnaryValue::operator<(NAryValue &)
{
    return false;
}

bool UnaryValue::operator<(UndefinedValue &)
{
    return false;
}

bool UnaryValue::operator<(UnaryValue& ov)
{
    if (op_ == ov.op_) {
        if (op_ == Operation::SquareRoot)
            return *value_ < *ov.value_;
    }

    return false;
}

bool UnaryValue::operator>(AbstractValue& av)
{
    switch (av.getKindOfValue()) {
    case KInteger: {
        IntegerValue& iv = static_cast<IntegerValue&>(av);
        return (*this > iv);
    }
    case KSymbol: {
        std::unique_ptr<AbstractValue> sv = av.evaluate();
        return *this > *sv;
    }
    case KInfinity: {
        InfinityValue& lv = static_cast<InfinityValue&>(av);
        return *this > lv;
    }
    case KNAry: {
        NAryValue& nv = static_cast<NAryValue&>(av);
        return *this > nv;
    }
    case KUndefined: {
        UndefinedValue& uv = static_cast<UndefinedValue&>(av);
        return (*this > uv);
    }
    case KUnary: {
        UnaryValue& ov = static_cast<UnaryValue&>(av);
        return (*this > ov);
    }
    default:
        break;
    }

    return false;
}

bool UnaryValue::operator>(IntegerValue& iv)
{
    if (op_ == SquareRoot && iv.getValue() < 0)
        return true;

    return false;
}

bool UnaryValue::operator>(SymbolValue& sv)
{
    return false;
}

bool UnaryValue::operator>(InfinityValue& lv)
{
    if (lv.getSign() == Negative)
        return true;
    return false;
}

bool UnaryValue::operator>(NAryValue &)
{
    return false;
}

bool UnaryValue::operator>(UndefinedValue &)
{
    return false;
}

bool UnaryValue::operator>(UnaryValue& ov)
{
    if (op_ == ov.op_) {
        if (op_ == Operation::SquareRoot)
            return *value_ > *ov.value_;
    }

    return false;
}

void printop(Operation op)
{
    switch (op) {
        case Operation::BadValue: {
            std::cout << "  BadValue  " << std::endl; break;
        }
        case Operation::Addition: {
            std::cout << "  Addition  " << std::endl; break;
        }
        case Operation::Subtraction: {
            std::cout << "  Subtraction  " << std::endl; break;
        }
        case Operation::Multiplication: {
            std::cout << "  Multiplication  " << std::endl; break;
        }
        case Operation::Division: {
            std::cout << "  Division  " << std::endl; break;
        }
        case Operation::SquareRoot: {
            std::cout << "  SquareRoot  " << std::endl; break;
        }
        // discrete things
        case Operation::Modulo: {
            std::cout << "  Modulo  " << std::endl; break;
        }
        case Operation::Minimum: {
            std::cout << "  Minimum  " << std::endl; break;
        }
        case Operation::Maximum: {
            std::cout << "  Maximum  " << std::endl; break;
        }
        // language things
        case Operation::ShiftRight: {
            std::cout << "  ShiftRight  " << std::endl; break;
        }
        case Operation::ShiftLeft: {
            std::cout << "  ShiftLeft  " << std::endl; break;
        }
        default: {
            r("  FUDEU  ");break;
        }
    }
}

}
