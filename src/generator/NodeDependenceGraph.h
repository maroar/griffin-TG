/******************************************************************************
 * Copyright (c) Solène Mirliaz (solene.mirliaz@ens-rennes.fr)
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

/*! \file */

#ifndef PSYCHE_NODE_DEPENDENCE_GRAPH_H__
#define PSYCHE_NODE_DEPENDENCE_GRAPH_H__

#include "ASTVisitor.h"
#include "Range.h"
#include "Literals.h"
#include "TypeNameSpeller.h"

#include <map>

namespace psyche {

//! Constant name for the maximal size of an array
//! It must be declared in header file
//! \see FunctionGenerator.cpp
const std::string minArraySizeCst = "MIN_ARRAY_SIZE";
const std::string minArraySizeValue = "100";
const std::string maxArraySizeCst = "MAX_ARRAY_SIZE";
const std::string maxArraySizeValue = "500";
const std::string nbTestsCst = "NB_TESTS";
const std::string nbTestsValue = "2000";
const std::string nbCallCst = "NB_CALLS";

class DependenceGraph;

class NodeDependenceGraph;
class ArrayNode;
class NumericalNode;
class ExpressionNode;
class AffineNode;
class ProductNode;
class MutableNode;
class GlobalVarNode;
class InputNode;
class UncompletedFunctionNode;

struct ParentType {
    int dim;
    std::unique_ptr<SymbolValue> symb;
};

enum NodeType {NTArray, NTAffine, NTProduct, NTGlobalVar, NTInput, NTUncompletedFunction};
std::string nodeToStr(NodeDependenceGraph* n);

//! Translate a NodeType into a string for debug
std::string getName(NodeType type);

//! Correct deletion of node
void deleteNode(NodeDependenceGraph *nodeUndef);

class NodeDependenceGraph
{
public:
    NodeDependenceGraph(TypeNameSpeller* typeSpeller,
                        const CPlusPlus::Scope* scope)
        : id_(nextId_++),
          typeSpeller_(typeSpeller),
          scope_(scope)
    {}
    ~NodeDependenceGraph() {}
    virtual NodeType type() const = 0;
    virtual std::string dotRepresentation() const = 0;

    virtual Range *downMessage() { return NULL; }
    virtual void receiveDownMessage(Range *rg, ParentType *from) { return; }
    virtual Range *upMessage(ParentType *to) { return NULL; }
    virtual void receiveUpMessage(Range *rg) { return; }

    virtual std::string declaration() const = 0;
    virtual std::set<const CPlusPlus::Symbol *> definitionsRequiredForRange() const
        { return std::set<const CPlusPlus::Symbol *>(); }
    virtual std::string rangeDefinition() const = 0;
    //! \note It is not necessary to specify own range
    virtual std::set<const CPlusPlus::Symbol *> rangeRequiredForDefinition() const
        { return std::set<const CPlusPlus::Symbol *>(); }
    virtual std::string definition() const = 0;
    virtual std::set<const CPlusPlus::Symbol *> definitionsControlled() const
        { return std::set<const CPlusPlus::Symbol *>(); }

    virtual std::string csvName() const
        { return std::to_string(id_) + ", "; }
    virtual std::string csvType() const
        { return "%d, "; }

    virtual std::set<const CPlusPlus::Symbol *> defineSymbols() const
        { return std::set<const CPlusPlus::Symbol *>(); }

    //! All information of the argument node are integrated to
    //! the information of the current node
    virtual void merge(NodeDependenceGraph&) {return; }
    virtual void merge(const ArrayNode&) { return; }
    virtual void merge(const NumericalNode&) { return; }
    virtual void merge(const AffineNode&) { return; }
    virtual void merge(const ProductNode&) { return; }
    virtual void merge(const MutableNode&) { return; }
    virtual void merge(const GlobalVarNode&) { return; }
    virtual void merge(const InputNode&) { return; }
    virtual void merge(const UncompletedFunctionNode&) { return; }

    //! Two nodes are considered equal if they can be merge,
    //! based on their common information (same symbol, same range, etc)
    virtual bool operator==(const NodeDependenceGraph&) const {return false;}
    virtual bool operator==(const ArrayNode&) const {return false;}
    virtual bool operator==(const NumericalNode&) const {return false;}
    virtual bool operator==(const ExpressionNode&) const {return false;}
    virtual bool operator==(const AffineNode&) const {return false;}
    virtual bool operator==(const ProductNode&) const {return false;}
    virtual bool operator==(const MutableNode&) const {return false;}
    virtual bool operator==(const GlobalVarNode&) const {return false;}
    virtual bool operator==(const InputNode&) const {return false;}
    virtual bool operator==(const UncompletedFunctionNode&) const {return false;}
    virtual bool is(const CPlusPlus::Symbol*) {return false;}

    friend class DependenceGraph;

    virtual const CPlusPlus::Symbol* getSymbol();
    virtual bool isMutable() { return false; }
    virtual bool isArray() { return false; }

    static unsigned nextId_;
protected:
    unsigned id_;

    // To have information about the type
    TypeNameSpeller* typeSpeller_;
    const CPlusPlus::Scope* scope_;
};

enum SymbolType {
    Input, Local, ReturnOfFunction
};

class ArrayNode final : public NodeDependenceGraph
{
public:
    //! \arg unit : used to create a new symbol
    //! \arg fctast : idem unit
    ArrayNode(const CPlusPlus::Symbol *symbol,
              SymbolType type,
              CPlusPlus::TranslationUnit *unit,
              TypeNameSpeller* typeSpeller,
              const CPlusPlus::Scope* scope);
    ~ArrayNode();
    NodeType type() const override { return NTArray; }
    std::string dotRepresentation() const override;
    void minimumSizeCstrt(int dimension, std::unique_ptr<AbstractValue> val);

    void receiveDownMessage(Range *rg, ParentType *from) override;
    Range *upMessage(ParentType *to) override;

    //! Only declare the sizeSymbol
    std::string declaration() const override;
    std::string rangeDefinition() const override;
    std::string rangeDefinition(const CPlusPlus::Symbol*) const;
    std::set<const CPlusPlus::Symbol*> rangeRequiredForDefinition() const override;
    //! Assign a value to the sizeSymbol
    //! and alloc memory to the array if it is an input
    std::string definition() const override;
    //! Alloc memory to the array if it is an input
    std::string definition(const CPlusPlus::Symbol*) const;
    //! Assign a value to the sizeSymbol
    std::string defineSizeSymbol(const CPlusPlus::Symbol*) const;

    std::string csvName() const override;
    std::string csvType() const override;

    std::set<const CPlusPlus::Symbol *> defineSymbols() const override;
    std::string stubs() const;
    std::string free() const;

    void merge(NodeDependenceGraph&) override;
    void merge(const ArrayNode&) override;

    bool operator==(const NodeDependenceGraph&) const override;
    bool operator==(const ArrayNode&) const override;
    bool is(const CPlusPlus::Symbol*) override;

    const CPlusPlus::Symbol* getSymbol() override;
    bool isArray() override { return true; }

private:
    //! Set of input array symbols that shares the same constraints
    std::set <const CPlusPlus::Symbol *> inputSymbols_;
    //! Set of local array symbols that shares the same constraints
    //! (They won't be defined)
    std::set <const CPlusPlus::Symbol *> localSymbols_;
    //! Function symbols
    std::set <const CPlusPlus::Symbol *> functions_;
    //! Maximum value that access each dimension
    std::vector<std::unique_ptr<AbstractValue> > maximumAccess_;
    //! \brief Minimal size affected to each dimension
    //!
    //! The dimensions of the array fix with a declaration.
    //! For example: v[][x][y]
    //! \note Only the first dimension may be unknown
    std::vector<std::unique_ptr<AbstractValue> > minimumSize_;

    const CPlusPlus::Symbol *arraySymbol_;
    CPlusPlus::Symbol *sizeSymbol_;

    CPlusPlus::TranslationUnit *unit_;
public:
    std::vector<const CPlusPlus::Symbol *> sizeSymbols_;
};

class NumericalNode : public NodeDependenceGraph
{
public:
    NumericalNode(TypeNameSpeller* typeSpeller,
                  const CPlusPlus::Scope* scope)
        : NodeDependenceGraph(typeSpeller, scope)
    {}
};

class ExpressionNode : public NumericalNode
{
public:
    ExpressionNode(CPlusPlus::TranslationUnit *unit,
                   TypeNameSpeller* typeSpeller,
                   const CPlusPlus::Scope* scope)
        : NumericalNode(typeSpeller, scope),
          unit_(unit),
          rangePostDesc_(NULL),
          rangePostAsc_(NULL),
          receiveFree_(false)
    {}
    ~ExpressionNode()
        {delete rangePostDesc_; delete rangePostAsc_;}

    Range *downMessage() override;

    std::set<const CPlusPlus::Symbol *> definitionsRequiredForRange() const override;

    Range getRangePostAsc();

protected:
    Range *rangePostDesc_;
    Range *rangePostAsc_;

    bool receiveFree_;

    CPlusPlus::TranslationUnit *unit_;
};

class AffineNode : public ExpressionNode
{
public:
    AffineNode(std::unique_ptr<AbstractValue> formula,
               CPlusPlus::TranslationUnit *unit,
               TypeNameSpeller* typeSpeller,
               const CPlusPlus::Scope* scope);
    ~AffineNode()
        {}
    NodeType type() const override { return NTAffine; }
    std::string dotRepresentation() const override;

    void receiveDownMessage(Range *rg, ParentType *from) override;
    Range *upMessage(ParentType *to) override;
    void receiveUpMessage(Range *rg) override;

    std::string declaration() const override
        { return ""; }
    std::string rangeDefinition() const override
        { return ""; }
    std::set<const CPlusPlus::Symbol *> rangeRequiredForDefinition() const override
        { return definitionsControlled(); }
    std::string definition() const override;
    std::set<const CPlusPlus::Symbol *> definitionsControlled() const override;

    bool operator==(const NodeDependenceGraph&) const override;
    bool operator==(const ExpressionNode&) const override;
    bool operator==(const AffineNode& e) const override { return *formula_->clone()->evaluate() == *(e.formula_->clone()->evaluate());}

    std::unique_ptr<AbstractValue> getFormula();

private:
    std::unique_ptr<AbstractValue> formula_;

};

class ProductNode : public ExpressionNode
{
public:
    ProductNode(const CPlusPlus::Symbol* ls,
                const NodeDependenceGraph* lp,
                const CPlusPlus::Symbol* rs,
                const NodeDependenceGraph* rp,
                CPlusPlus::TranslationUnit *unit,
                TypeNameSpeller* typeSpeller,
                const CPlusPlus::Scope* scope);
    ~ProductNode()
        {delete ownSymbol_;}
    NodeType type() const override { return NTProduct; }
    std::string dotRepresentation() const override;

    void receiveDownMessage(Range *rg, ParentType *from) override;
    Range *upMessage(ParentType *to) override;
    void receiveUpMessage(Range *rg) override;

    std::string declaration() const override;
    std::string rangeDefinition() const override;
    std::set<const CPlusPlus::Symbol *> rangeRequiredForDefinition() const override
        { return definitionsControlled(); }
    std::string definition() const override;
    std::set<const CPlusPlus::Symbol *> definitionsControlled() const override;

    std::set<const CPlusPlus::Symbol *> defineSymbols() const override;

    bool operator==(const NodeDependenceGraph&) const override;
    bool operator==(const ExpressionNode&) const override;
    bool operator==(const ProductNode& e) const override
        { return   ((*leftParent_ == *(e.leftParent_)) && (*rightParent_ == *(e.rightParent_)))
                || ((*leftParent_ == *(e.rightParent_)) && (*rightParent_ == *(e.leftParent_)));}
    bool is(const CPlusPlus::Symbol* s) override {return ownSymbol_->name() == s->name();}

    const CPlusPlus::Symbol* getSymbol() override;

private:
    const CPlusPlus::Symbol* ownSymbol_;
    const CPlusPlus::Symbol* leftSymbol_;
    const NodeDependenceGraph* leftParent_;
    const CPlusPlus::Symbol* rightSymbol_;
    const NodeDependenceGraph* rightParent_;

};

class MutableNode : public NumericalNode
{
public:
    MutableNode(CPlusPlus::Symbol *symbol,
                CPlusPlus::TranslationUnit *unit,
                TypeNameSpeller* typeSpeller,
                const CPlusPlus::Scope* scope)
        : NumericalNode(typeSpeller, scope),
          symbol_(symbol),
          unit_(unit),
          rangePostAsc_(NULL)
    {}
    ~MutableNode()
        {delete rangePostAsc_;}

    void receiveUpMessage(Range *rg) override;

    std::set<const CPlusPlus::Symbol *> definitionsRequiredForRange() const override;
    std::string rangeDefinition() const override;

    std::string csvName() const override;
    std::string csvType() const override;

    std::set<const CPlusPlus::Symbol *> defineSymbols() const override;

    bool is(const CPlusPlus::Symbol* s) override { return symbol_->name() == s->name(); }

    const CPlusPlus::Symbol* getSymbol() override;
    bool isMutable() override { return true; }

protected:
    CPlusPlus::Symbol * symbol_;
    Range *rangePostAsc_;
private:

    CPlusPlus::TranslationUnit *unit_;
};

class GlobalVarNode final : public MutableNode
{
public:
    GlobalVarNode(CPlusPlus::Symbol *symbol,
                  CPlusPlus::TranslationUnit *unit,
                  TypeNameSpeller* typeSpeller,
                  const CPlusPlus::Scope* scope)
        : MutableNode(symbol, unit, typeSpeller, scope)
    {}
    NodeType type() const override { return NTGlobalVar; }
    std::string dotRepresentation() const override;

    std::string declaration() const override
        { return ""; }
    std::string definition() const override;

    std::string def() const;

    bool operator==(const NodeDependenceGraph&) const override;
    bool operator==(const GlobalVarNode& g) const override { return symbol_ == g.symbol_;}
};

class InputNode final : public MutableNode
{
public:
    InputNode(CPlusPlus::Symbol *symbol,
              CPlusPlus::TranslationUnit *unit,
              TypeNameSpeller* typeSpeller,
              const CPlusPlus::Scope* scope)
        : MutableNode(symbol, unit, typeSpeller, scope)
    {}
    NodeType type() const override { return NTInput; }
    std::string dotRepresentation() const override;

    std::string declaration() const override;
    std::string definition() const override;

    bool operator==(const NodeDependenceGraph&) const override;
    bool operator==(const InputNode& i) const override { return symbol_ == i.symbol_;}
};

class UncompletedFunctionNode final : public MutableNode
{
public:
    UncompletedFunctionNode(CPlusPlus::Symbol *symbol,
                            CPlusPlus::TranslationUnit *unit,
                            TypeNameSpeller* typeSpeller,
                            const CPlusPlus::Scope* scope)
        : MutableNode(symbol, unit, typeSpeller, scope)
    {}
    NodeType type() const override { return NTUncompletedFunction; }
    std::string dotRepresentation() const override;

    std::string rangeDefinition() const override;
    std::string definition() const override
        { return ""; } // Actually, the definition is performed by def()
    std::string declaration() const override;

    std::string csvName() const override;
    std::string csvType() const override;

    std::string def() const;

    bool operator==(const NodeDependenceGraph&) const override;
    bool operator==(const UncompletedFunctionNode& uf) const override { return symbol_ == uf.symbol_;}
};
} // namespace psyche

#endif // NODEDEPENDENCEGRAPH_H
