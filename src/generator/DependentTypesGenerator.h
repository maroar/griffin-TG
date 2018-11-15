/******************************************************************************
 * Copyright (c) 2017 Marcus Rodrigues de Araújo (demaroar@gmail.com)
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

#ifndef PSYCHE_DEPENDENT_TYPES_GENERATOR_H__
#define PSYCHE_DEPENDENT_TYPES_GENERATOR_H__

#include "RangeAnalysis.h"
#include "TypeNameSpeller.h"

namespace psyche {

struct DependentType {
    DependentType(std::string baseType);
    virtual std::string toString();

    std::string baseType_;
};

struct DependentConst : public DependentType {
    DependentConst(std::string baseType, std::shared_ptr<AbstractValue> value);
    virtual std::string toString() override;

    std::shared_ptr<AbstractValue> value_;
};

struct DependentRange : public DependentType {
    DependentRange(std::string baseType, Range range);
    virtual std::string toString() override;

    Range range_;
};

struct DependentVector : public DependentType {
    DependentVector(std::string baseType, ArrayInfo& info);
    virtual std::string toString() override;

    std::list<std::shared_ptr<AbstractValue> > dimension_;
};

struct BuiltIn: public DependentType {
    BuiltIn(std::string baseType) : DependentType("") { baseType_ = baseType;}
};

struct DependentTypesGenerator {
    DependentTypesGenerator(RangeAnalysis& ra);

    void generate();
    void buildConst(const CPlusPlus::Symbol*, std::shared_ptr<AbstractValue> value);
    void buildRange(const CPlusPlus::Symbol*, Range& range);
    void buildVector(const CPlusPlus::Symbol*, ArrayInfo& info);
    void buildBuiltIn(const CPlusPlus::Symbol*);
    void insertInContext(const CPlusPlus::Symbol*, std::shared_ptr<DependentType>);

    RangeAnalysis& ra_;
    TypeNameSpeller typeSpeller_;
    std::map<const CPlusPlus::Symbol*, std::shared_ptr<DependentType> > typeContext_;
};

}

#endif
