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

/*! \file */

#ifndef PSYCHE_FUNCTION_GENERATOR_H__
#define PSYCHE_FUNCTION_GENERATOR_H__

#include "DependentTypesGenerator.h"
#include "DependenceGraph.h"

namespace psyche {

class FunctionGenerator final : public CPlusPlus::ASTVisitor
{
public:
    FunctionGenerator(CPlusPlus::TranslationUnit *unit,
                      DependentTypesGenerator& dependentTypesGenerator);
    void generate(CPlusPlus::TranslationUnitAST *ast, CPlusPlus::Namespace *global);

private:
    std::string extractId(const CPlusPlus::Name* name);
    const CPlusPlus::Symbol* findSymbol(const CPlusPlus::Name *name);

    const CPlusPlus::Scope* switchScope(const CPlusPlus::Scope *scope);
    //! Determine if a symbol is visible in the given scope of the function
    //! (for local variables or arguments) or in the extern scope (for global
    //! variables). Prevents the treatment of symbols of other functions.
    bool isVisible(const CPlusPlus::Symbol *symbol,
                   const CPlusPlus::Scope *scopeExtern,
                   const CPlusPlus::Scope *scopeFunction);

    //! Analyse the function and build a main C file that calls it with correct inputs
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override;
    //! Extract global variables and functions that need a stub.
    bool visit(CPlusPlus::SimpleDeclarationAST *ast) override;

    bool visit(CPlusPlus::ArrayDeclaratorAST *ast);

    void visitExpression(CPlusPlus::ExpressionAST *ast);

    // Expressions
    bool visit(CPlusPlus::BinaryExpressionAST* ast) override;
    bool visit(CPlusPlus::IdExpressionAST* ast) override;
    bool visit(CPlusPlus::MemberAccessAST* ast) override;
    bool visit(CPlusPlus::NumericLiteralAST* ast) override;
    bool visit(CPlusPlus::PostIncrDecrAST* ast) override;
    bool visit(CPlusPlus::CallAST* ast) override;
    bool visit(CPlusPlus::UnaryExpressionAST* ast) override;

    // AbstractValue
    //! Create nodes required to compute the abstract value
    //! Link them to the computedNode_
    void linkAbstractValue(std::unique_ptr<AbstractValue> value, NodeDependenceGraph *node);

    //! \brief Transform the formula into a term Literal*Symbol.
    //! If we have Literal*Symbol*Symbol, we create a new symbol S
    //! for Symbol*Symbol and a new Product Node that represents it.
    //! \param opposed If set to true, the opposed value of the Literal will be taken.
    std::unique_ptr<NAryValue> extractOneTerm(std::unique_ptr<AbstractValue> formula, bool opposed = false);
    //! \brief Transform the currentValue_ into an affine expression
    //! (call extractOneTerm)
    void simplifyToAffine();

    void addLocalVarComponents(const CPlusPlus::Symbol *symbol, Range *rg);

private:
    //! can be used to transform a type into a string (see DependentTypesGenerator.cpp)
    TypeNameSpeller typeSpeller_;

    //! dependent types of symbols
    DependentTypesGenerator& dependentTypesGenerator_;

    //! Scope we're in.
    const CPlusPlus::Scope* scope_;

    //! The dependance graph currently generated
    DependenceGraph depGraph_;

    //! The last node computed by visiting an AST expression
    NodeDependenceGraph *computedNode_;
    std::set<NodeDependenceGraph *> currentNodes_;
    //! The AbtractValue of the current AST expression
    std::unique_ptr<AbstractValue> currentValue_;
    std::set<std::unique_ptr<AbstractValue> > currentValues_;
    const CPlusPlus::StatementAST *currentStatement_;

    CPlusPlus::TranslationUnit *currentUnit_;
};

}

#endif
