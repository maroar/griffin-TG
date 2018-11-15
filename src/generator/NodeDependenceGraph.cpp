/******************************************************************************
 * Copyright (c) 2016 Solène Mirliaz (solene.mirliaz@ens-rennes.fr)
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

/*
 Contributors:
   - Marcus Rodrigues (demaroar@gmail.com)
*/

#include "NodeDependenceGraph.h"
#include "Name.h"
#include "Debug.h"
#include "Literals.h"
#include "Range.h"
#include "Assert.h"
#include "AST.h"
#include "Symbols.h"
#include "CoreTypes.h"

using namespace psyche;

unsigned NodeDependenceGraph::nextId_ = 0;

const std::string arrayStyle                = "color=\"#CC0000\",shape=record,";
const std::string affineStyle               = "color=\"#0000CC\",shape=record,";
const std::string productStyle              = "color=\"#006699\",shape=record,";
const std::string localVarStyle             = "shape=record,";
const std::string globalVarStyle            = "color=\"#CC00CC\",shape=record,";
const std::string inputStyle                = "color=\"#AA00AA\",shape=record,";
const std::string uncompletedFunctionStyle  = "color=\"#BB0022\",shape=record,";

const std::string array_size_header = "size_";
const std::string productVar = "product_";
const std::string anonymousNameArg = "arg";
const std::string lowerSuffixe = "__lower";
const std::string upperSuffixe = "__upper";

static const char* nodeString[] = {
    "NTArray",
    "NTAffine",
    "NTProduct",
    "NTGlobalVar",
    "NTInput",
    "NTUncompletedFunction"
};

std::string addDotEscape(std::string text) {
    size_t pos = text.find("<<");
    while (pos != std::string::npos) {
        text.replace(pos, 2, " \\<\\< ");
        pos = text.find("<<");
    }
    pos = text.find(">>");
    while (pos != std::string::npos) {
        text.replace(pos, 2, " \\>\\> ");
        pos = text.find(">>");
    }
    return text;
}

std::string basicForLoop(std::string var, std::string bound, std::string indent) {
    std::string ret;
    ret += indent;
    ret += "int ";
    ret += var;
    ret += ";\n";
    ret += indent;
    ret += "for (";
    ret += var;
    ret += " = 0; ";
    ret += var;
    ret += " <= ";
    ret += bound;
    ret += "; ";
    ret += var;
    ret += "++) {\n";
    return ret;
}

std::unique_ptr<AbstractValue> removeConstraint(std::unique_ptr<AbstractValue> val, std::set<const CPlusPlus::Symbol *> symbols) {
    switch (val->getKindOfValue()) {
    case KInteger:
    case KInfinity:
        return val;
        break;
    case KSymbol:
        for (auto s : symbols) {
            if (SymbolValue(s) == *val->clone().get())
                return NULL;
        }
        return val;
        break;
    case KNAry: {
        std::list<std::unique_ptr<AbstractValue> > terms = val->termsClone();
        std::list<std::unique_ptr<AbstractValue> > newTerms;
        bool usefulTerms = false;
        for (auto it = terms.begin(); it != terms.end(); ++it) {
            std::unique_ptr<AbstractValue> newTerm = removeConstraint((*it)->clone(), symbols);
            if (newTerm) {
                newTerms.push_back(newTerm->evaluate());
                usefulTerms = true;
            }
        }
        if (usefulTerms) {
            NAryValue* formula = static_cast<NAryValue *>(val->clone().get());
            NAryValue newNav(newTerms,
                             formula->op_);
            return newNav.evaluate();
        }
        return NULL;
    }
    }
}

void psyche::deleteNode(NodeDependenceGraph *nodeUndef)
{
    ArrayNode* nodeA;
    MutableNode* nodeM;
    AffineNode* nodeAf;
    ProductNode* nodeP;
    switch(nodeUndef->type()) {
    case NTArray:
        nodeA = static_cast<ArrayNode *>(nodeUndef);
        delete nodeA;
        break;
    case NTInput:
    case NTGlobalVar:
    case NTUncompletedFunction:
        nodeM = static_cast<MutableNode *>(nodeUndef);
        delete nodeM;
        break;
    case NTAffine:
        nodeAf = static_cast<AffineNode *>(nodeUndef);
        delete nodeAf;
        break;
    case NTProduct:
        nodeP = static_cast<ProductNode *>(nodeUndef);
        delete nodeP;
        break;
    default:
        delete nodeUndef;
        break;
    }
}

// -- ArrayNode --

ArrayNode::ArrayNode(const CPlusPlus::Symbol *symbol, SymbolType type, CPlusPlus::TranslationUnit *unit, TypeNameSpeller *typeSpeller, const CPlusPlus::Scope *scope)
    : NodeDependenceGraph(typeSpeller, scope),
      unit_(unit),
      sizeSymbol_(nullptr),
      arraySymbol_(symbol)
{
    switch (type) {
    case Input:
        inputSymbols_.insert(symbol);
        break;
    case Local:
        localSymbols_.insert(symbol);
        break;
    case ReturnOfFunction:
        functions_.insert(symbol);
        break;
    }

    // Creation of the size symbol radical
    std::string name = array_size_header + std::to_string(id_);

    CPlusPlus::FullySpecifiedType type;

    if (symbol->isArgument()) {
        if (symbol->asArgument()->type()->isPointerType()) {
            type = symbol->asArgument()->type()->asPointerType();
        }
        else if (symbol->asArgument()->type()->isArrayType()) {
            type = symbol->asArgument()->type()->asArrayType();
        }
    } else if (symbol->isDeclaration()) {
        if (symbol->asDeclaration()->type()->isPointerType()) {
            type = symbol->asDeclaration()->type()->asPointerType();
        }
        else if (symbol->asDeclaration()->type()->isArrayType()) {
            type = symbol->asDeclaration()->type()->asArrayType();
        }
    }

    // Extract each dimension type
    int i = 0;
    while (type->isArrayType() || type->isPointerType()) {
        if (type->isArrayType()) {
            type = type->asArrayType()->elementType();
        }
        else if (type->isPointerType()) {
            type = type->asPointerType()->elementType();
        }
        std::string nname = name;
        nname += "_";
        nname += std::to_string(i);
        CPlusPlus::Identifier *id = new CPlusPlus::Identifier(nname.c_str(), nname.length());
        CPlusPlus::Symbol* newSymb = new CPlusPlus::Declaration(unit_, 0, id);
        sizeSymbols_.push_back(newSymb);
        i++;
    }
}

ArrayNode::~ArrayNode()
{
    for (auto s : sizeSymbols_) {
        delete s->name()->asNameId();
        delete s;
    }
}

std::string ArrayNode::dotRepresentation() const
{
    std::string rep = std::to_string(id_);
    rep += " [" + arrayStyle;
    rep += "label=\"";

    // Input Symbols
    std::string inputSymbols = " ";
    for (auto it = inputSymbols_.begin(); it != inputSymbols_.end(); ++it) {
        inputSymbols += (*it)->name()->asNameId()->chars();
        inputSymbols += ",";
    }
    inputSymbols.erase(inputSymbols.end() - 1);

    // Local Symbols
    std::string localSymbols = " ";
    for (auto it = localSymbols_.begin(); it != localSymbols_.end(); ++it) {
        localSymbols += (*it)->name()->asNameId()->chars();
        localSymbols += ",";
    }
    localSymbols.erase(localSymbols.end() - 1);

    // Functions symbols
    std::string functions = " ";
    for (auto it = functions_.begin(); it != functions_.end(); ++it) {
        functions += (*it)->name()->asNameId()->chars();
        functions += ",";
    }
    functions.erase(functions.end() - 1);

    // Accesses
    std::string accesses = "{ ";
    for (auto it = maximumAccess_.begin(); it != maximumAccess_.end(); ++it) {
        accesses += (*it)->toString();
        accesses += "|";
    }
    if (accesses.length() == 2) {
        accesses += "No cst access known ";
    }
    accesses.erase(accesses.end() - 1);
    accesses += "}";

    // Alloc
    std::string allocs = "{ ";
    for (auto it = minimumSize_.begin(); it != minimumSize_.end(); ++it) {
        if (*it) {
            allocs += (*it)->toString();
        }
        allocs += "|";
    }
    if (allocs.length() == 2)
        allocs += array_size_header + std::to_string(id_);
    else
        allocs.erase(allocs.end() - 1);
    allocs += "}";
    allocs = " | " + allocs;

    rep += "{{" + inputSymbols + " | " + localSymbols + " | " + functions + "}| " + accesses + allocs + "}\"]";
    return rep;
}

void ArrayNode::minimumSizeCstrt(int dimension, std::unique_ptr<AbstractValue> val)
{
    while (minimumSize_.size() < dimension - 1) {
        minimumSize_.push_back(NULL);//std::make_unique<IntegerValue>(0));
    }
    minimumSize_.push_back(val->clone());
}

void ArrayNode::receiveDownMessage(Range *rg, ParentType *from)
{
    if (!rg)
        return;
    if (maximumAccess_.size() > from->dim - 1)
        maximumAccess_[from->dim - 1] = NAryValue(maximumAccess_[from->dim - 1]->clone(),
                                             rg->upper(),
                                             Maximum).evaluate();
    else {
        while (maximumAccess_.size() < from->dim - 1) {
            maximumAccess_.push_back(IntegerValue(0).evaluate());
        }
        maximumAccess_.push_back(rg->upper());
    }
}

Range *ArrayNode::upMessage(ParentType *to)
{
    const CPlusPlus::Symbol *sym = sizeSymbols_.at(to->dim - 1);
    return new Range(std::make_unique<IntegerValue>(0),
                     std::make_unique<SymbolValue>(sym));
}

std::string ArrayNode::declaration() const
{
    std::string decl = "";
    //decl += sizeSymbol_->name()->asNameId()->chars();
    //decl += ";\n";
    for (auto s : sizeSymbols_) {
        decl += "int ";
        decl += s->name()->asNameId()->chars();
        decl += ";\n";
    }
    return decl;
}

std::string ArrayNode::rangeDefinition() const
{
    std::string ret = "";
    int i = 0;
    for (auto s : sizeSymbols_) {
        std::string nameSize = s->name()->asNameId()->chars();
        ret += "    // dimension ";
        ret += std::to_string(i);
        ret += "\n    int ";
        ret += nameSize + lowerSuffixe;
        ret += " = ";
        if (minimumSize_.size() > i && minimumSize_[i]) {
            ret += minimumSize_[i]->toCCode();
        }
        else if (maximumAccess_.size() > i) {
            ret += maximumAccess_[i]->toCCode();
        }
        else {
            ret += "0";
        }
        ret += ";\n";
        ret += "    int ";
        ret += nameSize + upperSuffixe;
        ret += " = ";
        if (minimumSize_.size() > i && minimumSize_[i]) {
            ret += minimumSize_[i]->toCCode();
        } else {
            ret += maxArraySizeCst;
        }
        ret += ";\n";
        i++;
    }
    return ret;
}

std::string ArrayNode::rangeDefinition(const CPlusPlus::Symbol* symbol) const
{
    std::string ret = "";
    int i = 0;
    for (auto s : sizeSymbols_) {
        if (s == symbol) {
            std::string nameSize = s->name()->asNameId()->chars();
            ret += "    // dimension ";
            ret += std::to_string(i);
            ret += "\n    int ";
            ret += nameSize + lowerSuffixe;
            ret += " = ";
            if (minimumSize_.size() > i && minimumSize_[i]) {
                ret += minimumSize_[i]->toCCode();
            }
            else if (maximumAccess_.size() > i) {
                ret += maximumAccess_[i]->toCCode();
            }
            else {
                ret += minArraySizeCst;
            }
            ret += ";\n";
            ret += "    int ";
            ret += nameSize + upperSuffixe;
            ret += " = ";
            if (minimumSize_.size() > i && minimumSize_[i]) {
                ret += minimumSize_[i]->toCCode();
            } else {
                ret += maxArraySizeCst;
            }
            ret += ";\n";

            return ret;
        }
        i++;
    }
    return ret;
}

std::set<const CPlusPlus::Symbol *> ArrayNode::rangeRequiredForDefinition() const
{
    std::set<const CPlusPlus::Symbol *> newSet;
    for (auto s: sizeSymbols_)
        newSet.insert(s);
    return newSet;
}

std::string ArrayNode::definition() const
{
    std::string ret = "";

    // initializing the sizes (size_X_Y = rand_a_b(size_X_Y__lower, size_X_Y__upper))
    for (auto s : sizeSymbols_) {
        std::string cpltNameSize = s->name()->asNameId()->chars();
        ret += "    ";
        ret += cpltNameSize;
        ret += " = rand_a_b(";
        ret += cpltNameSize + lowerSuffixe;
        ret += ", ";
        ret += cpltNameSize + upperSuffixe;
        ret += ");\n";
    }

    for (auto s : inputSymbols_) {
        CPlusPlus::FullySpecifiedType type;
        std::string stars = "";
        std::string fixSize = "";
        int d = 0;
        if (s->isArgument() || s->isDeclaration()) {
            bool isArg;
            if (s->isArgument()) {
                isArg = true;
                if (s->asArgument()->type()->isPointerType()) {
                    type = s->asArgument()->type()->asPointerType();
                }
                else if (s->asArgument()->type()->isArrayType()) {
                    type = s->asArgument()->type()->asArrayType();
                }
            } else if (s->isDeclaration()) { // Global vars
                isArg = false;
                if (s->asDeclaration()->type()->isPointerType()) {
                    type = s->asDeclaration()->type()->asPointerType();
                }
                else if (s->asDeclaration()->type()->isArrayType()) {
                    type = s->asDeclaration()->type()->asArrayType();
                }
            }

            // Extract each dimension type
            while (type->isArrayType() || type->isPointerType()) {
                if (type->isArrayType()) {
                    type = type->asArrayType()->elementType();
                    fixSize += "[";
                    fixSize += sizeSymbols_.at(d)->name()->asNameId()->chars();
                    fixSize += " + 1]";
                    d++;
                }
                else if (type->isPointerType()) {
                    type = type->asPointerType()->elementType();
                    stars += "*";
                }
                // Need to be non-const to authorize free()
                type.setConst(false);
            }
            std::string typeName = typeSpeller_->spellTypeName(type, scope_);
            ret += "    ";

            // declaring the argument to the function in the main
            if (isArg) {
                ret += typeName;
                ret += " ";
                ret += stars;
                ret += " ";
                ret += s->name()->asNameId()->chars();
                ret += fixSize;
                ret += ";\n";
            }

            if (d < sizeSymbols_.size()) { // this condition will be true just if tihs is a pointer array
                ret += "    // Allocating each dynamic dimension\n";
                std::string indent = "    ";
                std::string pending = "";
                std::string indexation = "";
                // Loop for fix dimensions
                int index;
                for (index = 0; index < sizeSymbols_.size(); index++) {
                    std::string nameIndex = s->name()->asNameId()->chars();
                    nameIndex += "_i";
                    nameIndex += std::to_string(index);

                    if (index >= d) {
                        ret += indent;
                        ret += s->name()->asNameId()->chars();
                        ret += indexation;
                        ret += " = malloc(sizeof(";
                        ret += typeName;
                        ret += " *) * (";
                        ret += sizeSymbols_.at(index)->name()->asNameId()->chars();
                        ret += " + 1));\n";
                        ret += indent;
                        ret += "if (!";
                        ret += s->name()->asNameId()->chars();
                        ret += indexation;
                        ret += ") \n";
                        ret += indent;
                        ret += "  return 2;\n";
                    }

                    if (index != sizeSymbols_.size() - 1) {
                        ret += basicForLoop(nameIndex, sizeSymbols_.at(index)->name()->asNameId()->chars(), indent);
                        pending = indent + "}\n" + pending;
                        indent += "  ";
                        indexation += "[";
                        indexation += nameIndex,
                        indexation += "]";
                    }
                }
                ret += pending;

                // this for below is used to initialize the allocated arrays
                ret += "    // *** Initialization ***************************************** \n";
                std::string accIndent = indent;
                pending = "";
                indexation = "";
                std::string vetAccess = s->name()->asNameId()->chars();
                for (index = 0; index < sizeSymbols_.size(); index++) {
                    std::string nameIndex = s->name()->asNameId()->chars();;
                    nameIndex += "_it";
                    nameIndex += std::to_string(index);
                    ret += basicForLoop(nameIndex, sizeSymbols_.at(index)->name()->asNameId()->chars(), accIndent);
                    pending = accIndent + "}\n" + pending;
                    vetAccess += "[" + nameIndex + "]";
                    accIndent += indent;
                }
                vetAccess = accIndent + indent + vetAccess + " = rand_a_b(0, 100);\n";
                ret += vetAccess + pending;
                ret += "    // ************************************************************ \n";
            }
        }
    }
    return ret;
}

std::string ArrayNode::definition(const CPlusPlus::Symbol *) const
{
    std::string ret = "";

    for (auto s : inputSymbols_) {
        CPlusPlus::FullySpecifiedType type;
        std::string stars = "";
        std::string fixSize = "";
        int d = 0;
        if (s->isArgument() || s->isDeclaration()) {
            bool isArg;
            if (s->isArgument()) {
                isArg = true;
                if (s->asArgument()->type()->isPointerType()) {
                    type = s->asArgument()->type()->asPointerType();
                }
                else if (s->asArgument()->type()->isArrayType()) {
                    type = s->asArgument()->type()->asArrayType();
                }
            } else if (s->isDeclaration()) { // Global vars
                isArg = false;
                if (s->asDeclaration()->type()->isPointerType()) {
                    type = s->asDeclaration()->type()->asPointerType();
                }
                else if (s->asDeclaration()->type()->isArrayType()) {
                    type = s->asDeclaration()->type()->asArrayType();
                }
            }

            // Extract each dimension type
            while (type->isArrayType() || type->isPointerType()) {
                if (type->isArrayType()) {
                    type = type->asArrayType()->elementType();
                    fixSize += "[";
                    fixSize += sizeSymbols_.at(d)->name()->asNameId()->chars();
                    fixSize += " + 1]";
                    d++;
                }
                else if (type->isPointerType()) {
                    type = type->asPointerType()->elementType();
                    stars += "*";
                }
                // Need to be non-const to authorize free()
                type.setConst(false);
            }
            std::string typeName = typeSpeller_->spellTypeName(type, scope_);
            ret += "    ";

            // declaring the argument to the function in the main
            if (isArg) {
                ret += typeName;
                ret += " ";
                ret += stars;
                ret += " ";
                ret += s->name()->asNameId()->chars();
                ret += fixSize;
                ret += ";\n";
            }

            if (d < sizeSymbols_.size()) { // this condition will be true just if tihs is a pointer array
                ret += "    // Allocating each dynamic dimension\n";
                std::string indent = "    ";
                std::string pending = "";
                std::string indexation = "";
                // Loop for fix dimensions
                int index;
                for (index = 0; index < sizeSymbols_.size(); index++) {
                    std::string nameIndex = s->name()->asNameId()->chars();
                    nameIndex += "_i";
                    nameIndex += std::to_string(index);

                    if (index >= d) {
                        ret += indent;
                        ret += s->name()->asNameId()->chars();
                        ret += indexation;
                        ret += " = malloc(sizeof(";
                        ret += typeName;
                        ret += " *) * (";
                        ret += sizeSymbols_.at(index)->name()->asNameId()->chars();
                        ret += " + 1));\n";
                        ret += indent;
                        ret += "if (!";
                        ret += s->name()->asNameId()->chars();
                        ret += indexation;
                        ret += ") \n";
                        ret += indent;
                        ret += "  return 2;\n";
                    }

                    if (index != sizeSymbols_.size() - 1) {
                        ret += basicForLoop(nameIndex, sizeSymbols_.at(index)->name()->asNameId()->chars(), indent);
                        pending = indent + "}\n" + pending;
                        indent += "  ";
                        indexation += "[";
                        indexation += nameIndex,
                        indexation += "]";
                    }
                }
                ret += pending;

                // this for below is used to initialize the allocated arrays
                ret += "    // *** Initialization ***************************************** \n";
                std::string accIndent = indent;
                pending = "";
                indexation = "";
                std::string vetAccess = s->name()->asNameId()->chars();
                for (index = 0; index < sizeSymbols_.size(); index++) {
                    std::string nameIndex = s->name()->asNameId()->chars();;
                    nameIndex += "_it";
                    nameIndex += std::to_string(index);
                    ret += basicForLoop(nameIndex, sizeSymbols_.at(index)->name()->asNameId()->chars(), accIndent);
                    pending = accIndent + "}\n" + pending;
                    vetAccess += "[" + nameIndex + "]";
                    accIndent += indent;
                }
                vetAccess = accIndent + indent + vetAccess + " = rand_a_b(0, 100);\n";
                ret += vetAccess + pending;
                ret += "    // ************************************************************ \n";
            }
        }
    }
    return ret;
}

std::string ArrayNode::defineSizeSymbol(const CPlusPlus::Symbol* symbol) const
{
    std::string ret = "";
    for (auto s : sizeSymbols_) {
        if (s == symbol) {
            std::string cpltNameSize = s->name()->asNameId()->chars();
            ret += "    ";
            ret += cpltNameSize;
            ret += " = rand_a_b(";
            ret += cpltNameSize + lowerSuffixe;
            ret += ", ";
            ret += cpltNameSize + upperSuffixe;
            ret += ");\n";

            return ret;
        }
    }

    return ret;
}

std::string ArrayNode::csvName() const
{
    std::string r = "";
    for (int index = 0; index < sizeSymbols_.size(); index++) {
        r += sizeSymbols_[index]->name()->asNameId()->chars();
        r += ", ";
    }
    return r;
}

std::string ArrayNode::csvType() const
{
    std::string r = "";
    for (int index = 0; index < sizeSymbols_.size(); index++) {
        r += "%d, ";
    }
    return r;
}

std::set<const CPlusPlus::Symbol *> ArrayNode::defineSymbols() const
{
    std::set<const CPlusPlus::Symbol *> symbs;
    symbs.insert(sizeSymbols_.begin(), sizeSymbols_.end());
    return symbs;
}

std::string ArrayNode::stubs() const
{
    std::string definition = "";
    for (auto it = functions_.begin(); it != functions_.end(); it++) {
        const CPlusPlus::Function *func = (*it)->asDeclaration()->type()->asFunctionType();
        definition += typeSpeller_->spellTypeName(func->returnType(), scope_);
        definition += " ";
        definition += (*it)->name()->asNameId()->chars();
        definition += "(";
        if (func->hasArguments()) {
            // Scanning arguments
            const CPlusPlus::Symbol *currentSymbol = func->argumentAt(0);
            std::string typeArg = typeSpeller_->spellTypeName(currentSymbol->type(), scope_);
            std::string nameArg;
            if (currentSymbol->name()) {
                nameArg = currentSymbol->name()->asNameId()->identifier()->chars();
            }
            else {
                nameArg = anonymousNameArg + "0";
            }
            definition += typeArg + " " + nameArg;
            for (unsigned int i = 1; i < func->argumentCount(); i++) {
                currentSymbol = func->argumentAt(i);
                typeArg = typeSpeller_->spellTypeName(currentSymbol->type(), scope_);
                if (currentSymbol->name()) {
                    nameArg = currentSymbol->name()->asNameId()->identifier()->chars();
                }
                else {
                    nameArg = anonymousNameArg + std::to_string(i);
                }
                definition += ", " + typeArg + " " + nameArg;
            }
        }
        definition += ") {\n";
        // Body
        CPlusPlus::FullySpecifiedType type;
        std::string stars = "";
        std::string fixSize = "";
        int d = 0;
        if (func->returnType()->isPointerType()) {
            type = func->returnType()->asPointerType();
        }
        else if (func->returnType()->isArrayType()) {
            type = func->returnType()->asArrayType();
        }

        // Extract each dimension type
        while (type->isArrayType() || type->isPointerType()) {
            if (type->isArrayType()) {
                type = type->asArrayType()->elementType();
                fixSize += "[";
                fixSize += sizeSymbols_.at(d)->name()->asNameId()->chars();
                fixSize += " + 1]";
                d++;
            }
            else if (type->isPointerType()) {
                type = type->asPointerType()->elementType();
                stars += "*";
            }
            // Need to be non-const to authorize free()
            type.setConst(false);
        }
        std::string typeName = typeSpeller_->spellTypeName(type, scope_);
        definition += "  ";
        definition += typeName;
        definition += " ";
        definition += stars;
        definition += " ";
        definition += "returnPointer";
        definition += fixSize;
        definition += ";\n";
        if (d < sizeSymbols_.size()) {
            definition += "  // Allocating each dynamic dimension\n";
            std::string indent = "  ";
            std::string pending = "";
            std::string indexation = "";
            // Loop for fix dimensions
            int index;
            for (index = 0; index < sizeSymbols_.size(); index++) {
                std::string nameIndex = "returnPointer";
                nameIndex += "_i";
                nameIndex += std::to_string(index);

                if (index >= d) {
                    definition += indent;
                    definition += "returnPointer";
                    definition += indexation;
                    definition += " = malloc(sizeof(";
                    definition += typeName;
                    definition += " *) * (";
                    definition += sizeSymbols_.at(index)->name()->asNameId()->chars();
                    definition += " + 1));\n";
                }

                if (index != sizeSymbols_.size() - 1) {
                    definition += basicForLoop(nameIndex, sizeSymbols_.at(index)->name()->asNameId()->chars(), indent);
                    pending = indent + "}\n" + pending;
                    indent += "  ";
                    indexation += "[";
                    indexation += nameIndex,
                    indexation += "]";
                }
            }
            definition += pending;
        }
        definition += "  return returnPointer;\n";
        definition += "}\n";
    }
    return definition;
}

std::string ArrayNode::free() const
{
   std::string ret = "";
   for (auto s : inputSymbols_) {
       CPlusPlus::FullySpecifiedType type;
       std::string fixSize = "";
       int d = 0;
       if (s->isArgument() || s->isDeclaration()) { // TODO Remove
           if (s->isArgument()) {
               if (s->asArgument()->type()->isPointerType()) {
                   type = s->asArgument()->type()->asPointerType();
               }
               else if (s->asArgument()->type()->isArrayType()) {
                   type = s->asArgument()->type()->asArrayType();
               }
           } else if (s->isDeclaration()) { // Global vars
               if (s->asDeclaration()->type()->isPointerType()) {
                   type = s->asDeclaration()->type()->asPointerType();
               }
               else if (s->asDeclaration()->type()->isArrayType()) {
                   type = s->asDeclaration()->type()->asArrayType();
               }
           }

           // Extract each dimension type
           while (type->isArrayType() || type->isPointerType()) {
               if (type->isArrayType()) {
                   type = type->asArrayType()->elementType();
                   fixSize += "[";
                   fixSize += sizeSymbols_.at(d)->name()->asNameId()->chars();
                   fixSize += "+ 1]";
                   d++;
               }
               else if (type->isPointerType()) {
                   type = type->asPointerType()->elementType();
               }
           }
           std::string indent = "    ";
           std::string pending = "";
           std::string indexation = "";
           // Loop for fix dimensions
           int index;
           for (index = 0; index < sizeSymbols_.size(); index++) {
               std::string nameIndex = s->name()->asNameId()->chars();
               nameIndex += "_j";
               nameIndex += std::to_string(index);
               if (index != sizeSymbols_.size() - 1)
                ret += basicForLoop(nameIndex, sizeSymbols_.at(index)->name()->asNameId()->chars(), indent);

               if (index >= d) {
                   std::string newFree = indent;
                   newFree += "if (";
                   newFree += s->name()->asNameId()->chars();
                   newFree += indexation;
                   newFree += ") \n  ";
                   newFree += indent;
                   newFree += "free(";
                   newFree += s->name()->asNameId()->chars();
                   newFree += indexation;
                   newFree += ");\n";
                   pending = newFree + pending;
               }

               if (index != sizeSymbols_.size() - 1) {
                   pending = indent + "}\n" + pending;

                   indent += "  ";
                   indexation += "[";
                   indexation += nameIndex,
                   indexation += "]";
               }
           }
           ret += pending;
       }
   }
   return ret;
}

void ArrayNode::merge(NodeDependenceGraph &node)
{
    if (node.type() == type()) {
        ArrayNode *n = static_cast<ArrayNode*>(&node);
        merge(*n);
    }
}

void ArrayNode::merge(const ArrayNode &a)
{
    for (auto it = a.inputSymbols_.begin(); it != a.inputSymbols_.end(); it++) {
        inputSymbols_.insert(*it);
    }
    for (auto it = a.localSymbols_.begin(); it != a.localSymbols_.end(); it++) {
        localSymbols_.insert(*it);
    }
    for (auto it = a.functions_.begin(); it != a.functions_.end(); it++) {
        functions_.insert(*it);
    }

    for (int i  = sizeSymbols_.size(); a.sizeSymbols_.size() > sizeSymbols_.size(); i++) {
        std::string name = array_size_header + std::to_string(id_);
        name += "_";
        name += std::to_string(i);
        CPlusPlus::Identifier *id = new CPlusPlus::Identifier(name.c_str(), name.length());
        CPlusPlus::Symbol* newSymb = new CPlusPlus::Declaration(unit_, 0, id);
        sizeSymbols_.push_back(newSymb);
    }
    int commonSize = std::min(maximumAccess_.size(), a.maximumAccess_.size());
    for (unsigned i = 0; i < commonSize; i++) {
        maximumAccess_[i] = NAryValue(maximumAccess_[i]->clone(),
                                      a.maximumAccess_[i]->clone(),
                                      Maximum).evaluate();
    }
    if (maximumAccess_.size() < a.maximumAccess_.size()) {
        for (unsigned i = commonSize; i < a.maximumAccess_.size(); i++) {
            maximumAccess_.push_back(a.maximumAccess_[i]->clone());
        }
    }
    commonSize = std::min(minimumSize_.size(), a.minimumSize_.size());
    for (unsigned i = 0; i < commonSize; i++) {
        if (minimumSize_[i] && a.minimumSize_[i]) {
            minimumSize_[i] = NAryValue(minimumSize_[i]->clone(),
                                        a.minimumSize_[i]->clone(),
                                        Minimum).evaluate();
        } else if (a.minimumSize_[i]) {
            minimumSize_[i] = a.minimumSize_[i]->clone();
        }
    }
    if (minimumSize_.size() < a.minimumSize_.size()) {
        for (unsigned i = commonSize; i < a.minimumSize_.size(); i++) {
            if (a.minimumSize_[i])
                minimumSize_.push_back(a.minimumSize_[i]->clone());
            else
                minimumSize_.push_back(NULL);
        }
    }
}

bool ArrayNode::operator==(const NodeDependenceGraph &node) const
{
    if (node.type() == type()) {
        const ArrayNode *n = static_cast<const ArrayNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

bool ArrayNode::operator==(const ArrayNode & a) const
{
    for (auto it = inputSymbols_.begin(); it != inputSymbols_.end(); it++) {
        if (a.inputSymbols_.find(*it) != a.inputSymbols_.end())
            return true;
    }
    for (auto it = localSymbols_.begin(); it != localSymbols_.end(); it++) {
        if (a.localSymbols_.find(*it) != a.localSymbols_.end())
            return true;
    }
    for (auto it = functions_.begin(); it != functions_.end(); it++) {
        if (a.functions_.find(*it) != a.functions_.end())
            return true;
    }
    return false;
}

bool ArrayNode::is(const CPlusPlus::Symbol* s)
{
    if (inputSymbols_.find(s) != inputSymbols_.end())
        return true;
    if (localSymbols_.find(s) != localSymbols_.end())
        return true;
    if (functions_.find(s) != functions_.end())
        return true;
    return false;
}

const CPlusPlus::Symbol *ArrayNode::getSymbol()
{
    return arraySymbol_;
}

// -- ExpressionNode --

Range *ExpressionNode::downMessage()
{
    if (rangePostDesc_ && !receiveFree_)
        return new Range(rangePostDesc_->lower(), rangePostDesc_->upper());
    else
        return NULL;
}

std::set<const CPlusPlus::Symbol *> ExpressionNode::definitionsRequiredForRange() const
{
    std::set<const CPlusPlus::Symbol *> requirements;
    if (rangePostAsc_) {
        rangePostAsc_->lower_->buildSymbolDependence();
        rangePostAsc_->upper_->buildSymbolDependence();
        requirements.insert(rangePostAsc_->lower_->symbolDep_.begin(), rangePostAsc_->lower_->symbolDep_.end());
        requirements.insert(rangePostAsc_->upper_->symbolDep_.begin(), rangePostAsc_->upper_->symbolDep_.end());
    }
    return requirements;
}

Range ExpressionNode::getRangePostAsc()
{
    return rangePostAsc_->evaluate();
}

/*Range *ExpressionNode::rangePostAsc() const
{
    return rangePostAsc_;
}*/

// -- AffineNode --

AffineNode::AffineNode(std::unique_ptr<AbstractValue> formula, CPlusPlus::TranslationUnit *unit,
                       TypeNameSpeller *typeSpeller, const CPlusPlus::Scope *scope)
    : ExpressionNode(unit, typeSpeller, scope),
      formula_(std::move(formula))
{
    // Must extract the constant has the bootstrap of the descent
    PSYCHE_ASSERT(formula_->getKindOfValue() == KNAry, return, "Invalid formula for an AffineNode.");
    std::list<std::unique_ptr<AbstractValue> > terms = formula_->termsClone();
    PSYCHE_ASSERT(terms.front()->getKindOfValue() == KInteger, return, "Invalid formula for an AffineNode.");
    rangePostDesc_ = new Range(terms.front()->clone(), terms.front()->clone());
}

std::string AffineNode::dotRepresentation() const
{
    std::string rep = std::to_string(id_);
    rep += " [" + affineStyle;
    rep += "label=\"{";

    // Expression
    std::string expr = formula_->clone()->toString();
    expr = addDotEscape(expr);

    // Range info
    std::string range;
    if (rangePostAsc_) {
        range = "[";
        range += rangePostAsc_->lower_->toString();
        range += ", ";
        range += rangePostAsc_->upper_->toString();
        range += "]";
    }
    else if (rangePostDesc_) {
        range = "[";
        range += rangePostDesc_->lower_->toString();
        range += ", ";
        range += rangePostDesc_->upper_->toString();
        range += "]";
    }

    range = addDotEscape(range);

    rep += expr + " | " + range + "}\"]";
    return rep;
}

void AffineNode::receiveDownMessage(Range *rg, ParentType *from)
{
    if (!rg)
        receiveFree_ = true;
    else {
        // Find the symbol in the term
        IntegerValue zero(0);
        std::list<std::unique_ptr<AbstractValue> > terms = formula_->termsClone();
        PSYCHE_ASSERT(terms.front()->getKindOfValue() == KInteger, return, "Invalid formula for an AffineNode.");
        for (auto it = ++terms.begin(); it != terms.end(); it++) {
            // extract constant factor and symbol
            std::list<std::unique_ptr<AbstractValue> > term = (*it)->termsClone();
            AV front = terms.front()->clone();
            AV back = terms.back()->clone();
            IntegerValue *factor = static_cast<IntegerValue *>(front.get());
            SymbolValue *symb = static_cast<SymbolValue *>(back.get());
            if (*symb == *from->symb->clone().get()) {
                if (*factor < zero) {
                    rangePostDesc_ = new Range(*(rangePostDesc_->lower()) +
                                               *(*(rg->upper()) * *(factor->clone())),
                                               *(rangePostDesc_->upper()) +
                                               *(*(rg->lower()) * *(factor->clone())));
                } else {
                    rangePostDesc_ = new Range(*(rangePostDesc_->lower()) +
                                               *(*(rg->lower()) * *(factor->clone())),
                                               *(rangePostDesc_->upper()) +
                                               *(*(rg->upper()) * *(factor->clone())));
                }
            }
        }
    }
}

Range *AffineNode::upMessage(ParentType *to)
{
    // Coping with simple case
    std::list<std::unique_ptr<AbstractValue> > terms = formula_->termsClone();
    AV i = terms.front()->clone();
    IntegerValue *cst = static_cast<IntegerValue *>(i.get());
    if (terms.size() == 2) {// Just one symbol
        std::list<std::unique_ptr<AbstractValue> > term = terms.back()->termsClone();
        AV i2 = term.front()->clone();
        IntegerValue *factor = static_cast<IntegerValue *>(i2.get());
        Range *send(NULL);
        IntegerValue zero(0);
        if (*factor < zero) {
            send = new Range(*(*(rangePostAsc_->upper())  - *cst) / *(factor->clone()),*(*(rangePostAsc_->lower())  - *cst) / *(factor->clone()));
        } else {
            send = new Range(*(*(rangePostAsc_->lower())  - *cst) / *(factor->clone()),*(*(rangePostAsc_->upper())  - *cst) / *(factor->clone()));
        }

        return send;
    }

    return NULL;
}

void AffineNode::receiveUpMessage(Range *rg)
{
    if (rangePostAsc_ && rg) {
        Range* n = new Range(rg->rangeIntersection(rangePostAsc_));
        delete rangePostAsc_;
        rangePostAsc_ = n;
    }
    else if (!rangePostAsc_ && rg)
        rangePostAsc_ = new Range(*rg);

    if (rangePostAsc_) {
        Range* n = new Range(rangePostAsc_->evaluate());
        delete rangePostAsc_;
        rangePostAsc_ = n;
    }
}

std::string AffineNode::definition() const
{
    return ""; // =)
    if (!rangePostAsc_ || !rangePostDesc_) {
        return "";
    }
    std::string ret = "";
    IntegerValue zero(0);
    std::list<std::unique_ptr<AbstractValue> > terms = formula_->termsClone();
    if (terms.size() <= 2) {
        return ""; // No need to redefine since a up message as already been sent
    }
    // Restraint range using the range receive from non mutable nodes
    std::unique_ptr<AbstractValue> lastLower = *(rangePostAsc_->lower()) - *(rangePostDesc_->lower());
    std::unique_ptr<AbstractValue> lastUpper = *(rangePostAsc_->upper()) - *(rangePostDesc_->upper());
    for (auto it = ++terms.begin(); it != terms.end(); it++) {
        // extract constant factor and symbol
        std::list<std::unique_ptr<AbstractValue> > term = (*it)->termsClone();
        IntegerValue *factor = static_cast<IntegerValue *>(term.front()->clone().release());
        SymbolValue *symb = static_cast<SymbolValue *>(term.back()->clone().release());
        symb->buildSymbolDependence();
        // Create new symbols for the range bound
        // TODO: the node should own these symbols, only one version of them should exist and destruction of them should be handled
        std::string nameLow = (*symb->symbolDep_.begin())->name()->asNameId()->chars() + lowerSuffixe;
        std::string nameUp = (*symb->symbolDep_.begin())->name()->asNameId()->chars() + upperSuffixe;
        CPlusPlus::Identifier *idLow = new CPlusPlus::Identifier(nameLow.c_str(), nameLow.length());
        CPlusPlus::Identifier *idUp = new CPlusPlus::Identifier(nameUp.c_str(), nameUp.length());
        CPlusPlus::Symbol *lowSymbol = new CPlusPlus::Declaration(unit_, 0, idLow);
        CPlusPlus::Symbol *upSymbol = new CPlusPlus::Declaration(unit_, 0, idUp);
        ret += "    // Redefining the range of ";
        ret += (*symb->symbolDep_.begin())->name()->asNameId()->chars();
        ret += "\n    ";
        ret += nameLow;
        ret += " = max(2, ";
        ret += nameLow;
        ret += ", ";
        if (*factor < zero) { // Must exchange bounds
            ret += lastUpper->toCCode();
            ret += "/";
            ret += factor->toCCode();
            ret += ");\n    ";
            ret += nameUp;
            ret += " = min(2, ";
            ret += nameUp;
            ret += ", ";
            ret += lastLower->toCCode();
            lastLower = *lastLower - *(*(SymbolValue(upSymbol).evaluate()) * *factor);
            lastUpper = *lastUpper - *(*(SymbolValue(lowSymbol).evaluate()) * *factor);
        } else {
            ret += lastLower->toCCode();
            ret += "/";
            ret += factor->toCCode();
            ret += ");\n    ";
            ret += nameUp;
            ret += " = min(2, ";
            ret += nameUp;
            ret += ", ";
            ret += lastUpper->toCCode();
            lastLower = *lastLower - *(*(SymbolValue(lowSymbol).evaluate()) * *factor);
            lastUpper = *lastUpper - *(*(SymbolValue(upSymbol).evaluate()) * *factor);
        }
        ret += "/";
        ret += factor->toCCode();
        ret += ");\n";
    }
    return ret;
}

std::set<const CPlusPlus::Symbol *> AffineNode::definitionsControlled() const
{
    std::set<const CPlusPlus::Symbol *> requirements;
    // For each term except the first (which is the constant)
    std::list<std::unique_ptr<AbstractValue> > terms = formula_->termsClone();
    for (auto it = ++terms.begin(); it != terms.end(); it++) {
        std::list<std::unique_ptr<AbstractValue> > term = (*it)->termsClone();
        AV s = term.back()->clone();
        SymbolValue *symb = static_cast<SymbolValue *>(s.get());
        // Extract symbol and add them to the requirement
        symb->buildSymbolDependence();
        requirements.insert(symb->symbolDep_.begin(), symb->symbolDep_.end());
    }
    return requirements;
}

std::unique_ptr<AbstractValue> AffineNode::getFormula()
{
    return formula_->clone();
}

bool AffineNode::operator==(const ExpressionNode &node) const
{
    if (node.type() == type()) {
        const AffineNode *n = static_cast<const AffineNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

bool AffineNode::operator==(const NodeDependenceGraph &node) const
{
    if (node.type() == type()) {
        const AffineNode *n = static_cast<const AffineNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

// -- ProductNode --

ProductNode::ProductNode(const CPlusPlus::Symbol *ls, const NodeDependenceGraph *lp, const CPlusPlus::Symbol *rs, const NodeDependenceGraph *rp, CPlusPlus::TranslationUnit *unit, TypeNameSpeller *typeSpeller, const CPlusPlus::Scope *scope)
    : ExpressionNode(unit, typeSpeller, scope),
      leftSymbol_(ls),
      leftParent_(lp),
      rightSymbol_(rs),
      rightParent_(rp)
{
    // Create a new symbol
    std::string name = productVar ;
    name += leftSymbol_->name()->asNameId()->chars();
    name += "_";
    name += rightSymbol_->name()->asNameId()->chars();
    CPlusPlus::Identifier *id = new CPlusPlus::Identifier(name.c_str(), name.length());
    ownSymbol_ = new CPlusPlus::Declaration(unit_, 0, id);
}

std::string ProductNode::dotRepresentation() const
{
    std::string rep = std::to_string(id_);
    rep += " [" + productStyle;
    rep += "label=\"{";

    // Expression
    std::string expr = ownSymbol_->name()->asNameId()->chars();
    expr += " = ";
    expr += leftSymbol_->name()->asNameId()->chars();
    expr += " * ";
    expr += rightSymbol_->name()->asNameId()->chars();


    // Range info
    std::string range;
    if (rangePostAsc_) {
        range = "[";
        range += rangePostAsc_->lower_->toString();
        range += ", ";
        range += rangePostAsc_->upper_->toString();
        range += "]";
    }
    else if (rangePostDesc_) {
        range = "[";
        range += rangePostDesc_->lower_->toString();
        range += ", ";
        range += rangePostDesc_->upper_->toString();
        range += "]";
    }

    range = addDotEscape(range);

    rep += expr + " | " + range + "}\"]";
    return rep;
}

void ProductNode::receiveDownMessage(Range *rg, ParentType *from)
{
    if (rangePostDesc_ && rg) { // Range from both factor as been received
        // [A1, A2]*[B1, B2] = [min(A1*B1, A1*B2, A2*B1, A2*B2), max(A1*B1, A1*B2, A2*B1, A2*B2)]
        NAryValue a1b1 = NAryValue(rangePostDesc_->lower(), rg->lower(), Multiplication);
        NAryValue a1b2 = NAryValue(rangePostDesc_->lower(), rg->upper(), Multiplication);
        NAryValue a2b1 = NAryValue(rangePostDesc_->upper(), rg->lower(), Multiplication);
        NAryValue a2b2 = NAryValue(rangePostDesc_->upper(), rg->upper(), Multiplication);
        std::list<std::unique_ptr<AbstractValue> > terms1;
        terms1.push_back(a1b1.clone());
        terms1.push_back(a1b2.clone());
        terms1.push_back(a2b1.clone());
        terms1.push_back(a2b2.clone());
        NAryValue low = NAryValue(terms1, Minimum);
        std::list<std::unique_ptr<AbstractValue> > terms2;
        terms2.push_back(a1b1.clone());
        terms2.push_back(a1b2.clone());
        terms2.push_back(a2b1.clone());
        terms2.push_back(a2b2.clone());
        NAryValue up = NAryValue(terms2, Maximum);
        Range *newRange = new Range(low.evaluate(), up.evaluate());
        delete rangePostDesc_;
        rangePostDesc_ = newRange;
    }
    else if (!rangePostDesc_ && rg)
        rangePostDesc_ = new Range(*rg);
    else // !rg
        receiveFree_ = true;
}

Range *ProductNode::upMessage(ParentType *to)
{
    Range range(getRangePostAsc());
    AV size = UnaryValue(range.upper(), SquareRoot).evaluate();

    return new Range(IntegerValue(1).evaluate(), move(size));
}

void ProductNode::receiveUpMessage(Range *rg)
{
    if (rangePostAsc_ && rg) {
        rangePostAsc_ = new Range(rg->rangeIntersection(rangePostAsc_));
    }
    else if (!rangePostAsc_ && rg) {
        rangePostAsc_ = new Range(*rg);
    }

    if (rangePostAsc_) {
        rangePostAsc_ = new Range(rangePostAsc_->evaluate());
//std::cout << "          ***> " << *rangePostAsc_ << std::endl;
    } else {
        r("ERROR::ProductNode::receiveUpMessage(Range *rg): no range!");
    }
}

std::string ProductNode::declaration() const
{
    return ""; // =)
    std::string ret = "    ";
    ret += typeSpeller_->spellTypeName(ownSymbol_->type(), scope_);
    ret += " ";
    ret += ownSymbol_->name()->asNameId()->chars();
    ret += ";\n";
    return ret;
}

std::string ProductNode::rangeDefinition() const
{
    return ""; // =)
    std::string name = ownSymbol_->name()->asNameId()->chars();
    std::string ret = "";

    // Min and max of the range
    if (rangePostAsc_) {
        ret += "    int " + name + lowerSuffixe;
        ret += " = ";
        ret += rangePostAsc_->lower_->toCCode();
        ret += ";\n";
        ret += "    int " + name + upperSuffixe;
        ret += " = ";
        ret += rangePostAsc_->upper_->toCCode();
        ret += ";\n";
    }
    else {
        // TODO
        ret += "    int " + name + lowerSuffixe;
        ret += " = ";
        ret += "INT_MIN";
        ret += ";\n";
        ret += "    int " + name + upperSuffixe;
        ret += " = ";
        ret += "INT_MAX";
        ret += ";\n";
    }
    return ret;
}

std::string ProductNode::definition() const
{
    return ""; // =)
    std::string name = ownSymbol_->name()->asNameId()->chars();
    std::string ret = "    ";

    // Short-cuts
    std::string left_name = leftSymbol_->name()->asNameId()->chars();
    std::string left_low = left_name + lowerSuffixe;
    std::string left_up = left_name + upperSuffixe;
    std::string right_name = rightSymbol_->name()->asNameId()->chars();
    std::string right_low = right_name + lowerSuffixe;
    std::string right_up = right_name + upperSuffixe;

    ret += "fix_product_terms(&";
    ret += left_low;
    ret += ", &";
    ret += left_up;
    ret += ", &";
    ret += right_low;
    ret += ", &";
    ret += right_up;
    ret += ", ";
    ret += name + lowerSuffixe;
    ret += ", ";
    ret += name + upperSuffixe;
    ret += ");\n";
    return ret;
}

std::set<const CPlusPlus::Symbol *> ProductNode::definitionsControlled() const
{
    std::set<const CPlusPlus::Symbol *> requirements;
    requirements.insert(leftSymbol_);
    requirements.insert(rightSymbol_);
    return requirements;
}

std::set<const CPlusPlus::Symbol *> ProductNode::defineSymbols() const
{
    std::set<const CPlusPlus::Symbol *> s;
    s.insert(ownSymbol_);
    return s;
}

const CPlusPlus::Symbol *ProductNode::getSymbol()
{
    return ownSymbol_;
}

bool ProductNode::operator==(const NodeDependenceGraph &node) const
{
    if (node.type() == type()) {
        const ProductNode *n = static_cast<const ProductNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

bool ProductNode::operator==(const ExpressionNode &node) const
{
    if (node.type() == type()) {
        const ProductNode *n = static_cast<const ProductNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

// -- MutableNode --

void MutableNode::receiveUpMessage(Range *rg)
{
    if (rangePostAsc_ && rg) {
        Range *newRange = new Range(rg->rangeIntersection(rangePostAsc_));
        delete rangePostAsc_;
        rangePostAsc_ = newRange;
//std::cout << "          ---> " << *rangePostAsc_ << std::endl;
    }
    else if (!rangePostAsc_ && rg) {
        rangePostAsc_ = new Range(*rg);
//std::cout << "          ---> " << *rangePostAsc_ << std::endl;
    } else {
        g(declaration());
        r("ERROR::ProductNode::receiveUpMessage(Range *rg): no range!");
        //exit(1); // =)
    }
}

std::set<const CPlusPlus::Symbol *> MutableNode::definitionsRequiredForRange() const
{
    std::set<const CPlusPlus::Symbol *> requirements;
    if (rangePostAsc_) {
        rangePostAsc_->lower_->buildSymbolDependence();
        rangePostAsc_->upper_->buildSymbolDependence();
        requirements.insert(rangePostAsc_->lower_->symbolDep_.begin(), rangePostAsc_->lower_->symbolDep_.end());
        requirements.insert(rangePostAsc_->upper_->symbolDep_.begin(), rangePostAsc_->upper_->symbolDep_.end());
    }
    return requirements;
}

std::string MutableNode::rangeDefinition() const
{
    std::string name = symbol_->name()->asNameId()->chars();
    std::string ret = "";

    // Min and max of the range
    if (rangePostAsc_) {
        ret += "    int " + name + lowerSuffixe;
        ret += " = ";
        ret += rangePostAsc_->lower_->toCCode();
        ret += ";\n";
        ret += "    int " + name + upperSuffixe;
        ret += " = ";
        ret += rangePostAsc_->upper_->toCCode();
        ret += ";\n";
    }
    else {
        // TODO
        ret += "    int " + name + lowerSuffixe;
        ret += " = ";
        ret += "INT_MIN";
        ret += ";\n";
        ret += "    int " + name + upperSuffixe;
        ret += " = ";
        ret += "INT_MAX";
        ret += ";\n";
    }
    return ret;
}

std::string MutableNode::csvName() const {
    std::string ret = symbol_->name()->asNameId()->chars();
    ret += ", ";
    return ret;
}

std::string MutableNode::csvType() const
{
    CPlusPlus::FullySpecifiedType type;
    if (symbol_->isArgument()) {
        type = symbol_->asArgument()->type();
    } else if (symbol_->isDeclaration()) { // Global vars
        if (symbol_->asDeclaration()->type()->isFunctionType())
            type = symbol_->asDeclaration()->type()->asFunctionType()->returnType();
        else
            type = symbol_->asDeclaration()->type();
    }
    std::string speci, length;
    if (type.isUnsigned())
        speci = "u";
    else
        speci = "i";
    if (type.qualifiedType()->isIntegerType()) {
        switch (type.qualifiedType()->asIntegerType()->kind()) {
        case CPlusPlus::IntegerType::Char:
            length = "hh";
            break;
        case CPlusPlus::IntegerType::Short:
            length = "h";
            break;
        case CPlusPlus::IntegerType::Long:
            length = "l";
            break;
        case CPlusPlus::IntegerType::LongLong:
            length = "ll";
            break;
        default:
            break;
        }
    }
    if (type.qualifiedType()->isFloatType()) {
        speci = "";
        switch (type.qualifiedType()->asFloatType()->kind()) {
        case CPlusPlus::FloatType::Float:
            length = "f";
            break;
        case CPlusPlus::FloatType::Double:
            length = "e";
            break;
        case CPlusPlus::FloatType::LongDouble:
            length = "e";
            break;
        default:
            break;
        }
    }
    std::string ret = "%";
    ret += length + speci + ", ";
    return ret;
}

std::set<const CPlusPlus::Symbol *> MutableNode::defineSymbols() const
{
    std::set<const CPlusPlus::Symbol *> s;
    s.insert(symbol_);
    return s;
}

const CPlusPlus::Symbol *MutableNode::getSymbol()
{
    return symbol_;
}

std::string psyche::getName(NodeType type)
{
    switch(type) {
    case NTArray:
        return "Array";
        break;
    case NTAffine:
        return "Expression";
        break;
    case NTProduct:
        return "Product";
        break;
    case NTGlobalVar:
        return "GlobalVar";
        break;
    case NTInput:
        return "Input";
        break;
    case NTUncompletedFunction:
        return "UncompletedFunction";
        break;
    default:
        return "Unknown type";
        break;
    }
}

// -- GlobalVarNode --

std::string GlobalVarNode::dotRepresentation() const
{
    std::string rep = std::to_string(id_);
    rep += " [" + globalVarStyle;
    rep += "label=\"{";

    // Symbol
    std::string expr = symbol_->name()->asNameId()->chars();

    // Range info
    std::string range;
    if (rangePostAsc_) {
        range = "[";
        range += rangePostAsc_->lower_->toString();
        range += ", ";
        range += rangePostAsc_->upper_->toString();
        range += "]";
    }

    range = addDotEscape(range);

    rep += expr + " | " + range + "}\"]";
    return rep;
}

std::string GlobalVarNode::def() const
{
    if (symbol_->isDeclaration()) {
        std::string definition = typeSpeller_->spellTypeName(symbol_->asDeclaration()->type(), scope_);
        definition += " ";
        definition += symbol_->name()->asNameId()->chars();
        definition += ";\n";
        return definition;
    }
    return "";
}

std::string GlobalVarNode::definition() const
{
    if (symbol_->isDeclaration()) {
        std::string affectation = "";
        std::string name = symbol_->name()->asNameId()->chars();

        affectation += "    ";
        affectation += name;
        affectation += " = rand_a_b(";
        affectation += name + lowerSuffixe;
        affectation += ", ";
        affectation += name + upperSuffixe;
        affectation += ");\n";

        return affectation;
    }
    return "";
}

bool GlobalVarNode::operator==(const NodeDependenceGraph &node) const
{
    if (node.type() == type()) {
        const GlobalVarNode *n = static_cast<const GlobalVarNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

// -- InputNode --

std::string InputNode::dotRepresentation() const
{
    std::string rep = std::to_string(id_);
    rep += " [" + inputStyle;
    rep += "label=\"{";

    // Symbol
    std::string expr = symbol_->name()->asNameId()->chars();

    // Range info
    std::string range;
    if (rangePostAsc_) {
        range = "[";
        range += rangePostAsc_->lower_->toString();
        range += ", ";
        range += rangePostAsc_->upper_->toString();
        range += "]";
    }

    range = addDotEscape(range);

    rep += expr + " | " + range + "}\"]";
    return rep;
}

std::string InputNode::declaration() const
{
    std::string definition = "    ";
    if (symbol_->isArgument()) {
        definition += typeSpeller_->spellTypeName(symbol_->asArgument()->type(), scope_);
    }
    definition += " ";
    definition += symbol_->name()->asNameId()->chars();
    definition += ";\n";
    return definition;
}

std::string InputNode::definition() const
{
    if (symbol_->isArgument()) {
        std::string affectation = "";
        std::string name = symbol_->name()->asNameId()->chars();

        affectation += "    ";
        affectation += name;
        affectation += " = rand_a_b(";
        affectation += name + lowerSuffixe;
        affectation += ", ";
        affectation += name + upperSuffixe;
        affectation += ");\n";

        return affectation;
    }
    return "";
}

bool InputNode::operator==(const NodeDependenceGraph &node) const
{
    if (node.type() == type()) {
        const InputNode *n = static_cast<const InputNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

// -- UncompletedFunctionNode --

std::string UncompletedFunctionNode::dotRepresentation() const
{
    std::string rep = std::to_string(id_);
    rep += " [" + uncompletedFunctionStyle;
    rep += "label=\"{";

    // Symbol
    std::string expr = symbol_->name()->asNameId()->chars();

    // Range info
    std::string range;
    if (rangePostAsc_) {
        range = "[";
        range += rangePostAsc_->lower_->toString();
        range += ", ";
        range += rangePostAsc_->upper_->toString();
        range += "]";
    }

    range = addDotEscape(range);

    rep += expr + " | " + range + "}\"]";
    return rep;
}

std::string UncompletedFunctionNode::rangeDefinition() const
{
    std::string name = symbol_->name()->asNameId()->chars();
    std::string ret = "";

    // Min and max of the range
    if (rangePostAsc_) {
        ret += "    " + name + lowerSuffixe;
        ret += " = ";
        ret += rangePostAsc_->lower_->toCCode();
        ret += ";\n";
        ret += "    " + name + upperSuffixe;
        ret += " = ";
        ret += rangePostAsc_->upper_->toCCode();
        ret += ";\n";
    }
    return ret;
}

std::string UncompletedFunctionNode::declaration() const
{
    CPlusPlus::FullySpecifiedType type = symbol_->asDeclaration()->type()->asFunctionType()->returnType();
    if (type->isVoidType())
        return "";
    std::string name = symbol_->name()->asNameId()->chars();
    std::string ret = "";

    // Min and max of the range
    std::string typeName = typeSpeller_->spellTypeName(type, scope_);
    if (rangePostAsc_) {
        ret += typeName + " " + name + lowerSuffixe;
        ret += ";\n";
        ret += typeName + " " + name + upperSuffixe;
        ret += ";\n";
    }
    else {
        // Min and Max values, according to the type
        std::string min, max, pref = "INT_";
        if (type.qualifiedType()->isIntegerType()) {
            switch (type.qualifiedType()->asIntegerType()->kind()) {
            case CPlusPlus::IntegerType::Char:
                pref = "CHAR_";
                break;
            case CPlusPlus::IntegerType::Short:
                pref = "SHRT_";
                break;
            case CPlusPlus::IntegerType::Long:
                pref = "LONG_";
                break;
            case CPlusPlus::IntegerType::LongLong:
                pref = "LLONG_";
                break;
            default:
                break;
            }
        }
        if (type.isUnsigned()) {
            min = "0";
            max = "U" + pref + "MAX";
        } else {
            min = pref + "MIN";
            max = pref + "MAX";
        }
        ret += typeName + " " + name + lowerSuffixe;
        ret += " = ";
        ret += min;
        ret += ";\n";
        ret += typeName + " " + name + upperSuffixe;
        ret += " = ";
        ret += max;
        ret += ";\n";
    }
    return ret;
}

std::string UncompletedFunctionNode::csvName() const
{
    CPlusPlus::FullySpecifiedType type = symbol_->asDeclaration()->type()->asFunctionType()->returnType();
    if (type->isVoidType())
        return "";
    std::string name = symbol_->name()->asNameId()->chars();
    std::string ret = "";
    ret += name + lowerSuffixe;
    ret += ", ";
    ret += name + upperSuffixe;
    ret += ", ";
    return ret;
}

std::string UncompletedFunctionNode::csvType() const
{
    CPlusPlus::FullySpecifiedType type = symbol_->asDeclaration()->type()->asFunctionType()->returnType();
    if (type->isVoidType())
        return "";
    std::string speci, length;
    if (type.isUnsigned())
        speci = "u";
    else
        speci = "i";
    if (type.qualifiedType()->isIntegerType()) {
        switch (type.qualifiedType()->asIntegerType()->kind()) {
        case CPlusPlus::IntegerType::Char:
            length = "hh";
            break;
        case CPlusPlus::IntegerType::Short:
            length = "h";
            break;
        case CPlusPlus::IntegerType::Long:
            length = "l";
            break;
        case CPlusPlus::IntegerType::LongLong:
            length = "ll";
            break;
        default:
            break;
        }
    }
    if (type.qualifiedType()->isFloatType()) {
        speci = "";
        switch (type.qualifiedType()->asFloatType()->kind()) {
        case CPlusPlus::FloatType::Float:
            length = "f";
            break;
        case CPlusPlus::FloatType::Double:
            length = "e";
            break;
        case CPlusPlus::FloatType::LongDouble:
            length = "e";
            break;
        default:
            break;
        }
    }
    std::string ret = "%";
    ret += length + speci;
    ret = ret + ", " + ret + ", "; // Lower and upper bound
    return ret;
}

std::string UncompletedFunctionNode::def() const
{
    if (symbol_->isDeclaration() && symbol_->asDeclaration()->type()->isFunctionType()) {
        CPlusPlus::Function *func = symbol_->asDeclaration()->type()->asFunctionType();
        std::string definition = typeSpeller_->spellTypeName(func->returnType(), scope_);
        std::string name = symbol_->name()->asNameId()->chars();

        definition += " ";
        definition += name;
        definition += "(";
        if (func->hasArguments()) {
            // Scanning arguments
            const CPlusPlus::Symbol *currentSymbol = func->argumentAt(0);
            std::string typeArg = typeSpeller_->spellTypeName(currentSymbol->type(), scope_);
            std::string nameArg;
            if (currentSymbol->name()) {
                nameArg = currentSymbol->name()->asNameId()->identifier()->chars();
            }
            else {
                nameArg = anonymousNameArg + "0";
            }
            // TODO Naming is anonymous
            definition += typeArg + " " + nameArg;
            for (unsigned int i = 1; i < func->argumentCount(); i++) {
                currentSymbol = func->argumentAt(i);
                typeArg = typeSpeller_->spellTypeName(currentSymbol->type(), scope_);
                if (currentSymbol->name()) {
                    nameArg = currentSymbol->name()->asNameId()->identifier()->chars();
                }
                else {
                    nameArg = anonymousNameArg + std::to_string(i);
                }
                definition += ", " + typeArg + " " + nameArg;
            }
        }
        definition += ") {\n";
        // Body
        if (func->returnType().type()->isVoidType()) {
            definition += "  return;\n";
        }
        else {
            definition += "  return rand_a_b(" + name + lowerSuffixe + ", " + name + upperSuffixe + ");\n";
        }
        definition += "}\n";

        return definition;
    }
    return "";
}

bool UncompletedFunctionNode::operator==(const NodeDependenceGraph &node) const
{
    if (node.type() == type()) {
        const UncompletedFunctionNode *n = static_cast<const UncompletedFunctionNode*>(&node);
        return *(this) == *n;
    }

    return false;
}

std::string psyche::nodeToStr(NodeDependenceGraph *n)
{
    return nodeString[n->type()];
}

const CPlusPlus::Symbol *NodeDependenceGraph::getSymbol()
{
    return NULL;
}
