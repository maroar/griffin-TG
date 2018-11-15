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

#include "RangeAnalysis.h"
#include "Debug.h"
#include "Literals.h"
#include "TypeOfExpr.h"
#include "DependentTypesGenerator.h"
#include <algorithm>

using namespace psyche;
using namespace CPlusPlus;

// DependentTypesGenerator

DependentTypesGenerator::DependentTypesGenerator(psyche::RangeAnalysis& ra)
    : ra_(ra)
    , typeSpeller_(nullptr)
{
}

void DependentTypesGenerator::generate()
{
    for (auto p : ra_.rangeMap_) {
        if (p.first->name()
                && p.first->name()->asNameId()) {
            Range range = p.second;
            auto pointerIsArrayIt = ra_.pointerIsArray_.find(p.first);
            if (pointerIsArrayIt == ra_.pointerIsArray_.end()) {
                buildRange(p.first, range);
            } else {
                if (pointerIsArrayIt->second) {
                    auto infoIt = ra_.arrayInfoMap_.find(p.first);
                    if (infoIt == ra_.arrayInfoMap_.end()) {
                        buildBuiltIn(p.first);
                    } else
                        buildVector(p.first, infoIt->second);
                } else {
                    buildBuiltIn(p.first);
                }
            }
        }
    }
    /*std::cout << "Type Context:" << std::endl;
    for (auto& it : typeContext_) {
        std::cout << it.first->name()->asNameId()->identifier()->chars()
                  << ":" << it.second->toString() << std::endl;
    }*/
}

void DependentTypesGenerator::buildConst(const Symbol* sym, std::shared_ptr<AbstractValue> value)
{
    std::string type = typeSpeller_.spellTypeName(sym->type(), sym->enclosingScope());
    insertInContext(sym, std::make_shared<DependentConst>(type, value));
}

void DependentTypesGenerator::buildRange(const Symbol* sym, Range& range)
{
    if (range.isConst()) {
        buildConst(sym, range.upper());
        return;
    }
    std::string type = typeSpeller_.spellTypeName(sym->type(), sym->enclosingScope());
    insertInContext(sym, std::make_shared<DependentRange>(type, range));
}

void DependentTypesGenerator::buildVector(const Symbol* sym, ArrayInfo& info)
{
    std::string type = typeSpeller_.spellTypeName(sym->type(), sym->enclosingScope());
    insertInContext(sym, std::make_shared<DependentVector>(type, info));
}

void DependentTypesGenerator::buildBuiltIn(const Symbol* sym)
{
    std::string type = typeSpeller_.spellTypeName(sym->type(), sym->enclosingScope());
    insertInContext(sym, std::make_shared<BuiltIn>(type));
}

void DependentTypesGenerator::insertInContext(const Symbol* sym, std::shared_ptr<DependentType> dType)
{
    typeContext_.insert(std::make_pair(sym, dType));
}

// DependentType

DependentType::DependentType(std::string baseType)
    : baseType_(baseType)
{
    baseType_.erase(std::remove(baseType_.begin(), baseType_.end(), '*'), baseType_.end());
}

std::string DependentType::toString()
{
    return baseType_;
}

// DependentConst

DependentConst::DependentConst(std::string baseType,
                              std::shared_ptr<AbstractValue> value)
    : DependentType(baseType), value_(value)
{}

std::string DependentConst::toString()
{
    std::string ret = "Const ";
    ret += baseType_;
    ret += " ";
    ret += value_->toString();

    return ret;
}

// DependentRange

DependentRange::DependentRange(std::string baseType, Range range)
    : DependentType(baseType), range_(range)
{}

std::string DependentRange::toString()
{
    std::string ret = "Range ";
    ret += baseType_;
    ret += " ";
    ret += range_.lower()->toString();
    ret += " ";
    ret += range_.upper()->toString();

    return ret;
}

// DependentVector

DependentVector::DependentVector(std::string baseType, ArrayInfo& info)
    : DependentType(baseType)
{
    for (auto& it : info.dimensionRange_) {
        dimension_.push_back(info.dimensionLength(it.first));
    }
}

std::string DependentVector::toString()
{
    std::string ret = "Vector ";
    ret += baseType_;
    ret += " [";

    if (dimension_.size() > 0) {
        ret += dimension_.front().get()->toString();
        for (auto it = std::next(dimension_.begin()); it != dimension_.end(); ++it) {
            ret += ", ";
            ret += it->get()->toString();
        }
    }

    ret += "]";

    return ret;
}

