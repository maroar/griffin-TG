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

#include "AST.h"
#include "Assert.h"
#include "Debug.h"
#include "Literals.h"
#include "LoopAnalyser.h"
#include "Lookup.h"
#include "Scope.h"
#include "Symbols.h"
#include "TranslationUnit.h"
#include "Utils.h"
#include <algorithm>
#include <iostream>

#define VISITOR_NAME "LoopAnalyser"

using namespace std;
using namespace CPlusPlus;
using namespace psyche;

LoopAnalyser::LoopAnalyser(TranslationUnit *unit,
                           CPlusPlus::StatementAST* loop,
                           const map<const Symbol*, list<Range> > &history,
                           const Scope *scope)
    : ASTVisitor(unit),
      loop_(loop),
      ranges_(history),
      scope_(scope),
      insideCondition_(false),
      loopSize_(nullptr)
{
    debugVisit = true;
    run();
    if (debugVisit)
        print();
    debugVisit = false;
}

LoopAnalyser::~LoopAnalyser()
{
}

const Scope *LoopAnalyser::switchScope(const Scope *scope)
{
    PSYCHE_ASSERT(scope, return nullptr, "scope must be valid");
    std::swap(scope_, scope);
    return scope;
}

std::string LoopAnalyser::extractId(const Name* name)
{
    PSYCHE_ASSERT(name && name->isNameId(),
                  return std::string(),
                  "expected simple name");

    const Identifier *id = name->asNameId()->identifier();
    return std::string (id->chars(), id->size());
}

void LoopAnalyser::resolve(const Name *name)
{
    PSYCHE_ASSERT(name && name->asNameId(), return, "expected simple name");

    std::string sym = extractId(name);
    if (debugVisit)
        std::cout << "resolve: " << sym << std::endl;
    symbol_ = lookupValueSymbol(name, scope_);
    PSYCHE_ASSERT(symbol_, return, "expected successful lookup");
}

void LoopAnalyser::solveConstraints()
{
    shared_ptr<AbstractConstr> mainConstr = constrs_.top();
    loopSize_ = mainConstr->solve();
}

bool LoopAnalyser::visit(BinaryExpressionAST *ast)
{
    DEBUG_VISIT(BinaryExpressionAST);

    accept(ast->left_expression);

    unsigned op = tokenKind(ast->binary_op_token);

    switch (op) {
        case T_LESS: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            constrs_.push(make_shared<Constr>(move(left), move(right), T_LESS, *this));

            break;
        }
        case T_LESS_EQUAL: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            constrs_.push(make_shared<Constr>(move(left), move(right), T_LESS_EQUAL, *this));

            break;
        }
        case T_GREATER: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            constrs_.push(make_shared<Constr>(move(left), move(right), T_GREATER, *this));

            break;
        }
        case T_GREATER_EQUAL: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            constrs_.push(make_shared<Constr>(move(left), move(right), T_GREATER_EQUAL, *this));

            break;
        }
        case T_EQUAL_EQUAL: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            constrs_.push(make_shared<Constr>(move(left), move(right), T_EQUAL_EQUAL, *this));

            break;
        }
        case T_EXCLAIM_EQUAL: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            constrs_.push(make_shared<Constr>(move(left), move(right), T_EXCLAIM_EQUAL, *this));

            break;
        }
        case T_AMPER_AMPER: {
            shared_ptr<AbstractConstr> left = constrs_.top();
            constrs_.pop();

            accept(ast->right_expression);
            shared_ptr<AbstractConstr> right = constrs_.top();
            constrs_.pop();

            constrs_.push(make_shared<Connector>(left, right, T_AMPER_AMPER, *this));

            break;
        }
        case T_PIPE_PIPE: {
            shared_ptr<AbstractConstr> left = constrs_.top();
            constrs_.pop();

            accept(ast->right_expression);
            shared_ptr<AbstractConstr> right = constrs_.top();
            constrs_.pop();

            constrs_.push(make_shared<Connector>(left, right, T_PIPE_PIPE, *this));

            break;
        }
        case T_PLUS: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            exps_.push(*left + *right);

            break;
        }
        case T_MINUS: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            exps_.push(*left - *right);

            break;
        }
        case T_STAR: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            exps_.push(*left * *right);

            break;
        }
        case T_SLASH: {
            AV left = exps_.top()->clone();
            exps_.pop();

            accept(ast->right_expression);
            AV right = exps_.top()->clone();
            exps_.pop();
            exps_.push(*left / *right);

            break;
        }
        default: {
            r("ERROR::bool LoopAnalyser::visit(BinaryExpressionAST *ast)::default");
            exit(1);
        }
    }

    return false;
}

bool LoopAnalyser::visit(IdExpressionAST *ast)
{
    DEBUG_VISIT(IdExpressionAST);

    resolve(ast->name->name);

    if (insideCondition_)
        conditionVariables_.insert(symbol_);

    exps_.push(SymbolValue(symbol_).evaluate());

    return false;
}

bool LoopAnalyser::visit(MemberAccessAST *ast)
{
    DEBUG_VISIT(MemberAccessAST);

    y("MemberAccessAST");
    PSYCHE_ASSERT(false, exit(1), "not supported yet!");
    // base_expression.member_name
    const Scope* prevScope = scope_;
    resolve(ast->member_name->name);
    switchScope(prevScope);

    if (insideCondition_)
        conditionVariables_.insert(symbol_);

    return false;
}

bool LoopAnalyser::visit(NumericLiteralAST *ast)
{
    DEBUG_VISIT(NumericLiteralAST);

    const NumericLiteral *numLit = numericLiteral(ast->literal_token);
    PSYCHE_ASSERT(numLit, return false, "numeric literal must exist");

    auto value = std::atoi(numLit->chars());

    exps_.push(IntegerValue(value).clone());

    return false;
}

bool LoopAnalyser::visit(PostIncrDecrAST *ast)
{
    DEBUG_VISIT(PostIncrDecrAST);

    accept(ast->base_expression);

    return false;
}

bool LoopAnalyser::visit(CallAST *ast)
{
    DEBUG_VISIT(CallAST);

    accept(ast->base_expression);

    return false;
}

bool LoopAnalyser::visit(UnaryExpressionAST *ast)
{
    DEBUG_VISIT(UnaryExpressionAST);

    accept(ast->expression);

    return false;
}

bool LoopAnalyser::visit(ExpressionStatementAST *ast)
{
    DEBUG_VISIT(ExpressionStatementAST);

    PSYCHE_ASSERT(false, exit(1), "not supported yet!");

    return false;
}

bool LoopAnalyser::visit(WhileStatementAST *ast)
{
    DEBUG_VISIT(WhileStatementAST);

    insideCondition_ = true;
    accept(ast->condition);
    insideCondition_ = false;

    return false;
}

bool LoopAnalyser::visit(ForStatementAST *ast)
{
    DEBUG_VISIT(ForStatementAST);

    insideCondition_ = true;
    accept(ast->condition);
    insideCondition_ = false;

    return false;
}

bool LoopAnalyser::visit(DoStatementAST *ast)
{
    DEBUG_VISIT(DoStatementAST);

    y("DoStatementAST");
    PSYCHE_ASSERT(false, exit(1), "not supported yet!");

    return false;
}

void LoopAnalyser::checkForModificationsInsideTheLoop()
{
    for (const auto& it : conditionVariables_) {
        const list<Range>& historyOfSymbol = ranges_.find(it)->second;
        if (historyOfSymbol.size() == 2) {
            Range r1 = historyOfSymbol.front();
            Range r2 = historyOfSymbol.back();
            if (r1 == r2) {
                loopInvariants_.insert(it);
            } else {
                modifiedVariables_.insert(it);
            }
        } else {
            cout << "ERROR: void LoopAnalyser::checkForModificationsInsideTheLoop()::bad history" << endl;
            exit(1);
        }
    }
}

void LoopAnalyser::print()
{
    b(" ********************************** ");
    b(" symbols in condition: ");
    for (const auto& it : conditionVariables_) {
        const Identifier *id = it->name()->asNameId()->identifier();
        std::string ret(id->begin(), id->end());
        cout << " -> " << ret << endl;
    }

    b("symbols modified in the loop body:");
    for (const auto& it : modifiedVariables_) {
        const Identifier *id = it->name()->asNameId()->identifier();
        std::string ret(id->begin(), id->end());
        cout << " -> " << ret << endl;
    }

    b("loop invariants:");
    for (const auto& it : loopInvariants_) {
        const Identifier *id = it->name()->asNameId()->identifier();
        std::string ret(id->begin(), id->end());
        cout << " -> " << ret << endl;
    }

    b("history of ranges");
    for (auto &a : ranges_) {
        std::cout << a.first->name()->asNameId()->identifier()->chars() << ": ";
        for (auto &b : a.second) {
            std::cout << b << " ";
        }
        std::cout << std::endl;
    }

    b("constraints");
    constrs_.top()->print();

    b("solution");
    cout << loopSize_->toString() << endl;

    b(" ********************************** ");
}

void LoopAnalyser::run()
{
    // Loop size: (n - a)/b
    //   - for (i = a; i < n; i += b)

    // pick the symbols used in the condition of the loop
    accept(loop_);

    // check for what can be the loop variants and the
    // loop invariants
    checkForModificationsInsideTheLoop();

    // solve collected constraints and try
    // to find a number of iterations for the
    // loop
    solveConstraints();
}

bool LoopAnalyser::visit(ArrayAccessAST *ast)
{
    DEBUG_VISIT(ArrayAccessAST);

    // base[expression]
    accept(ast->base_expression);
    accept(ast->expression);

    return false;
}

bool LoopAnalyser::visit(PointerAST *ast)
{
    DEBUG_VISIT(PointerAST);

    y("Pointer");
    PSYCHE_ASSERT(false, exit(1), "not supported yet!");

    return false;
}

string opToString(unsigned op)
{
    switch(op) {
        case T_LESS: {
            return "<";
        }
        case T_LESS_EQUAL: {
            return "<=";
        }
        case T_GREATER: {
            return ">";
        }
        case T_GREATER_EQUAL: {
            return ">=";
        }
        case T_EQUAL_EQUAL: {
            return "==";
        }
        case T_EXCLAIM_EQUAL: {
            return "!=";
        }
        case T_AMPER_AMPER: {
            return "&&";
        }
        case T_PIPE_PIPE: {
            return "||";
        }
        case T_PLUS: {
            return "+";
        }
        case T_MINUS: {
            return "-";
        }
        case T_STAR: {
            return "*";
        }
        case T_SLASH: {
            return "/";
        }
        default: {
            return "<other>";
        }
    }
}

// AbstractConstr

void AbstractConstr::print()
{}

std::unique_ptr<psyche::AbstractValue> AbstractConstr::solve()
{}

// Constr

void Constr::print()
{
    g("Constr <BEGIN>");
    string left = left_->toString();
    string right = right_->toString();
    cout << left << " " << opToString(op_) << " " << right << endl;
    g("Constr <END>");
}

std::unique_ptr<psyche::AbstractValue> loopSizeFormula(AV a, AV b, AV n) {
    AV nMinusA = *n - *a;
    return (*nMinusA / *b);
}

std::unique_ptr<psyche::AbstractValue> Constr::solve()
{
    // we can solve the size of the loop in the cases listed bellow:
    //   if we have two symbols:
    //     S1 | S2 | Solve?
    //      I |  V | ok
    //      V |  I | ok
    //      V |  V | ---
    //      I |  I | ---

    // collect all symbols used in the expression on the left
    left_->buildSymbolDependence();
    list<const CPlusPlus::Symbol*>
            leftSymbols(left_->symbolDep_.begin(), left_->symbolDep_.end());

    // collect all symbols used in the expression on the right
    right_->buildSymbolDependence();
    list<const CPlusPlus::Symbol*>
            rightSymbols(right_->symbolDep_.begin(), right_->symbolDep_.end());

    // if have a relational operator with two expressions containing
    // more then one symbol we are not able to solve it
    // exp1 `op` exp2
    if (rightSymbols.size() > 1 and leftSymbols.size() > 1)
        return InfinityValue(Positive).evaluate();


    // transform the set of invariants int to a list
    list<const CPlusPlus::Symbol*>
            loopInvariants(la_.loopInvariants_.begin(), la_.loopInvariants_.end());

    // transform the set of modified symbols int to a list
    list<const CPlusPlus::Symbol*>
            modifiedVariables(la_.modifiedVariables_.begin(), la_.modifiedVariables_.end());

    // intersection between left symbols and invariant symbols
    // show us if we have a variant in the left side of the operator
    list<const CPlusPlus::Symbol*> leftInvar;
    set_intersection(leftSymbols.begin(), leftSymbols.end(),
              loopInvariants.begin(), loopInvariants.end(), back_inserter(leftInvar));
    // intersection between left symbols and variant symbols
    // show us if we have invariants in the left side of the operator
    list<const CPlusPlus::Symbol*> leftVar;
    set_intersection(leftSymbols.begin(), leftSymbols.end(),
              modifiedVariables.begin(), modifiedVariables.end(), back_inserter(leftVar));
    // intersection between right symbols and invariant symbols
    // show us if we have a variant in the right side of the operator
    list<const CPlusPlus::Symbol*> rightInvar;
    set_intersection(rightSymbols.begin(), rightSymbols.end(),
              loopInvariants.begin(), loopInvariants.end(), back_inserter(rightInvar));
    // intersection between right symbols and variant symbols
    // show us if we have invariants in the right side of the operator
    list<const CPlusPlus::Symbol*> rightVar;
    set_intersection(rightSymbols.begin(), rightSymbols.end(),
              modifiedVariables.begin(), modifiedVariables.end(), back_inserter(rightVar));

    AV a, b, n;
    const CPlusPlus::Symbol* var;
    bool canCalculateLoopSize = false;
    // we just apply the formula when we have exactly one variant
    // either in the left or in the right
    if ( (leftVar.size() == 1) && (rightVar.size() == 0) ) {
        var = leftVar.front();

        // for now we just handle simple cases. (when we have only
        // one symbol in the expression that contains the variant
        if (!(*(SymbolValue(var).evaluate()) == *left_))
            return InfinityValue(Positive).evaluate();

        canCalculateLoopSize = true;
        n = right_->evaluate();
    } else if ( (rightVar.size() == 1) && (leftVar.size() == 0) ) {
        var = rightVar.front();

        // for now we just handle simple cases. (when we have only
        // one symbol in the expression that contains the variant
        if (!(*(SymbolValue(var).evaluate()) == *right_))
            return InfinityValue(Positive).evaluate();

        canCalculateLoopSize = true;
        n = left_->evaluate();
    }

    if (canCalculateLoopSize) {
        auto it = la_.ranges_.find(var);
        if (it == la_.ranges_.end())
            return InfinityValue(Positive).evaluate();

        Range oldRangeOfVarinat(it->second.front());
        Range lastRangeOfVarinat(it->second.back());

        a = oldRangeOfVarinat.lower_->evaluate();
        b = *(lastRangeOfVarinat.upper_->evaluate()) - *a;
        if ((op_ == T_LESS_EQUAL) || (op_ == T_GREATER_EQUAL))
            return (IntegerValue(1) + *loopSizeFormula(move(a), move(b), move(n)));
        else
            return loopSizeFormula(move(a), move(b), move(n));
    }

    // we could not find the size of the loop
    return InfinityValue(Positive).evaluate();
    /*
    for (const auto& it : all) {
        const Identifier *id = it->name()->asNameId()->identifier();
        std::string ret(id->begin(), id->end());
        cout << " -> " << ret << endl;
    }
    */
}

// Connector

void Connector::print()
{
    y("Connector <BEGIN>");
    left_->print();
    string op = opToString(op_);
    cout << "Connector OP = " << op << endl;
    right_->print();
    y("Connector <END>");
}

std::unique_ptr<psyche::AbstractValue> Connector::solve()
{
    AV left = left_->solve();
    AV right = right_->solve();

    if (op_ == T_AMPER_AMPER) {
        return NAryValue(left->clone(), right->clone(), Maximum).clone();
    } else if (op_ == T_PIPE_PIPE) {
        return NAryValue(left->clone(), right->clone(), Minimum).clone();
    } else {
        r("ERROR::void Connector::solve()::bad operator");
        exit(1);
        return nullptr;
    }
}
