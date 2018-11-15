/******************************************************************************
 * Copyright (c) 2016 Marcus Rodrigues de Araújo (demaroar@gmail.com)
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

#ifndef PSYCHE_LOOPANALYSER__
#define PSYCHE_LOOPANALYSER__

#include "ASTVisitor.h"
#include "ngraph.hpp"
#include "Symbol.h"
#include "FullySpecifiedType.h"
#include "Range.h"
#include "TranslationUnit.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <list>
#include <utility>
#include <map>
#include <set>
#include <stack>
#include <string>

struct LoopAnalyser;

struct AbstractConstr {
    AbstractConstr (unsigned op,
                    LoopAnalyser& la)
        : op_(op),
          la_(la) {}

    //virtual std::string toString();
    virtual void print();
    virtual std::unique_ptr<psyche::AbstractValue> solve();
    LoopAnalyser& la_;
protected:
    unsigned op_;
};

struct Constr : public AbstractConstr {
    Constr(std::unique_ptr<psyche::AbstractValue> left,
           std::unique_ptr<psyche::AbstractValue> right,
           unsigned op,
           LoopAnalyser& la)
        : AbstractConstr(op, la),
          left_(std::move(left)),
          right_(std::move(right)) {}

    Constr(const Constr& c) :
        AbstractConstr(c.op_, c.la_),
        left_(c.left_->clone()),
        right_(c.right_->clone()) {}

    void print() override;
    std::unique_ptr<psyche::AbstractValue> solve() override;

    std::unique_ptr<psyche::AbstractValue> left_;
    std::unique_ptr<psyche::AbstractValue> right_;
};

struct Connector : public AbstractConstr {
    Connector(shared_ptr<AbstractConstr> left,
              shared_ptr<AbstractConstr> right,
              unsigned op,
              LoopAnalyser& la)
        : AbstractConstr(op, la),
          left_(left),
          right_(right) {}

    void print() override;
    std::unique_ptr<psyche::AbstractValue> solve() override;

    shared_ptr<AbstractConstr> left_;
    shared_ptr<AbstractConstr> right_;
};

struct LoopAnalyser final : public CPlusPlus::ASTVisitor {

    LoopAnalyser(CPlusPlus::TranslationUnit *unit,
                 CPlusPlus::StatementAST* loop,
                 const std::map<const CPlusPlus::Symbol*,
                                std::list<psyche::Range> >& history,
                 const CPlusPlus::Scope* scope);
    ~LoopAnalyser();

    void print();

    void run();

    // loop condition
    CPlusPlus::StatementAST* loop_;

    bool insideCondition_;

    // AST - PostfixDeclarator
    bool visit(CPlusPlus::ArrayAccessAST *ast) override;
    bool visit(CPlusPlus::PointerAST *ast) override;

    // AST - Expressions
    bool visit(CPlusPlus::BinaryExpressionAST* ast) override;
    bool visit(CPlusPlus::IdExpressionAST* ast) override;
    bool visit(CPlusPlus::MemberAccessAST* ast) override;
    bool visit(CPlusPlus::NumericLiteralAST* ast) override;
    bool visit(CPlusPlus::PostIncrDecrAST* ast) override;
    bool visit(CPlusPlus::CallAST* ast) override;
    bool visit(CPlusPlus::UnaryExpressionAST* ast) override;

    // AST - Statements
    bool visit(CPlusPlus::ExpressionStatementAST *ast) override;
    bool visit(CPlusPlus::WhileStatementAST *ast) override;
    bool visit(CPlusPlus::ForStatementAST *ast) override;
    bool visit(CPlusPlus::DoStatementAST *ast) override;

    // ranges of symbols
    const std::map<const CPlusPlus::Symbol*,
                   std::list<psyche::Range> >& ranges_;

    // verify if the loop modify variables that appear in loop
    // condition. The verification is made by considering the
    // values in 'history_'.
    void checkForModificationsInsideTheLoop();

    // symbols used in the loop condition
    std::set<const CPlusPlus::Symbol*> conditionVariables_;

    // symbols modified in the loop body
    std::set<const CPlusPlus::Symbol*> modifiedVariables_;

    // invariant symbols
    std::set<const CPlusPlus::Symbol*> loopInvariants_;

    // current symbol during expression traverse
    CPlusPlus::Symbol* symbol_;

    // current scope
    const CPlusPlus::Scope* scope_;
    const CPlusPlus::Scope* switchScope(const CPlusPlus::Scope *scope);

    // used to extract the names
    std::string extractId(const CPlusPlus::Name* name);
    void resolve(const CPlusPlus::Name *name);

    // contains the computed size of the loop
    std::unique_ptr<psyche::AbstractValue> loopSize_;

    // stack with the current expression
    std::stack<std::unique_ptr<psyche::AbstractValue>> exps_;

    // list of constraints obtained
    std::stack<shared_ptr<AbstractConstr>> constrs_;

    // solve collected constraint to derive an expression
    // representing the number of iterations of the loop
    void solveConstraints();
};

#endif
