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

#include "FunctionGenerator.h"
#include "AST.h"
#include "Assert.h"
#include "Debug.h"
#include "Lookup.h"
#include "Scope.h"
#include "Symbols.h"
#include "Utils.h"
#include "Bind.h"
#include "Control.h"

#include <fstream>

#define VISITOR_NAME "FunctionGenerator"

using namespace CPlusPlus;
using namespace psyche;

namespace psyche {

//! Prefix added to the basic name for the main file (can be placed in a folder)
const std::string mainFileNamePrefix = "/mains/";
const std::string csvFileNamePrefix = "../csv/";
//! Extension added to the basic name for the main file
const std::string mainFileNameSuffix = "_main.c";
//! \brief Name of the file containing various tool definition
//! It must contain the definition of the function min and max
//! and the macro of maximal array size
//! \see NodeDependenceGraph.h
const std::string includeFileName = "../../headerStub.c";

}

FunctionGenerator::FunctionGenerator(TranslationUnit *unit,
                                     DependentTypesGenerator &dependentTypesGenerator)
    : ASTVisitor(unit),
      dependentTypesGenerator_(dependentTypesGenerator),
      computedNode_(NULL)
{
}

const Scope* FunctionGenerator::switchScope(const Scope* scope)
{
    PSYCHE_ASSERT(scope, return nullptr, "scope must be valid");
    std::swap(scope_, scope);
    return scope;
}

bool FunctionGenerator::isVisible(const Symbol *symbol, const Scope *scopeExtern, const Scope *scopeFunction)
{
    if (!symbol)
        return false;

    // Check extern scope
    if (scopeExtern) {
        for (unsigned int i = 0; i != scopeExtern->memberCount(); i++) {
            if (scopeExtern->memberAt(i) == symbol) {
                return true;
            }
        }
    }
    // Check function scope
    if (scopeFunction) {
        for (unsigned int i = 0; i != scopeFunction->memberCount(); i++) {
            if (scopeFunction->memberAt(i) == symbol) {
                return true;
            }
            else if (scopeFunction->memberAt(i)->isScope()) {
                if (isVisible(symbol, NULL, scopeFunction->memberAt(i)->asScope())) {
                    return true;
                }
            }
        }
    }

    return false;
}

std::string FunctionGenerator::extractId(const Name* name)
{
    PSYCHE_ASSERT(name && name->isNameId(),
                  return std::string(),
                  "expected simple name");

    const Identifier *id = name->asNameId()->identifier();
    return std::string (id->chars(), id->size());
}

const Symbol *FunctionGenerator::findSymbol(const Name *name)
{
    // We expect to see only simple names. Also, since range analysis assumes
    // well-formed code, symbol lookup must succeed.
    PSYCHE_ASSERT(name && name->asNameId(), return NULL, "expected simple name");
    std::string sym = extractId(name);
    if (debugVisit)
        std::cout << "resolve: " << sym << std::endl;
    const Symbol* symb = lookupValueSymbol(name, scope_);
    if (!symb) { // ???
        for (auto it = dependentTypesGenerator_.typeContext_.begin();
             it != dependentTypesGenerator_.typeContext_.end(); ++it) {
            if (it->first->name()->asNameId() == name->asNameId()) {
                symb = it->first;
            }
        }
    }
    PSYCHE_ASSERT(symb, return NULL, "expected successful lookup");
    return symb;
}

void FunctionGenerator::generate(TranslationUnitAST* ast, Namespace* global)
{
    debugVisit = false;
    currentUnit_ = translationUnit();

    switchScope(global);
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next)
        accept(it->value);
}

bool FunctionGenerator::visit(FunctionDefinitionAST *ast)
{
    DEBUG_VISIT(FunctionDefinitionAST);

    Function* func = ast->symbol;
    std::string funName = func->name()->asNameId()->chars();

    const Scope *previousScope = switchScope(func->asScope());
    accept(ast->declarator->asDeclarator());

    RangeAnalysis* ra = &dependentTypesGenerator_.ra_;

    // Step 1. Extract all function symbols for which a stub is necessary
    // Done during the visit of simple declarations

    // Step 2. Create the nodes for the input symbol
    // Testing that all informations are available
    if (func->hasArguments()) {
        if (func->argumentAt(0)->name() == 0) {
            std::cout << ANSI_COLOR_RED << "[ERROR] FunctionGenerator: No information on the arguments is available." << std::endl
                      << ANSI_COLOR_YELLOW << "[WARNING] FunctionGenerator: The creation of the main() function is cancelled." << ANSI_COLOR_RESET << std::endl;
            return false;
        }
        // Scanning arguments
        for (unsigned int i = 0; i < func->argumentCount(); i++) {
            CPlusPlus::Symbol *currentSymbol = func->argumentAt(i);
            // Determine type
            if (ra->pointerIsArray_[currentSymbol]) {
                NodeDependenceGraph* node = new ArrayNode(currentSymbol, Input, currentUnit_, &typeSpeller_, scope_);
                depGraph_.addNode(node);
            }
            else {
                NodeDependenceGraph* node = new InputNode(currentSymbol, currentUnit_, &typeSpeller_, scope_);
                depGraph_.addNode(node);
            }
        }
    }
    std::cout << "[FunctionGenerator] Number of Inputs in the Dependence Graph: " << depGraph_.size() << std::endl;

    // Step 3. Create the nodes for the arrays (from the definition statements and from the array info)
    for (auto it = ra->arrayDefinitions_.begin(); it != ra->arrayDefinitions_.end(); it++) {
        const Symbol* currentArray = it->first;
        bool isFunction = false;
        if (currentArray->asDeclaration())
            isFunction = currentArray->asDeclaration()->type()->isFunctionType();
        NodeDependenceGraph* node = NULL;
        if (isFunction)
            node = new ArrayNode(currentArray, ReturnOfFunction, currentUnit_, &typeSpeller_, scope_);
        else {
            node = depGraph_.find(currentArray); // Is there an input array with this symbol?
            // If not it is a local array
            if (!node)
                node = new ArrayNode(currentArray, Local, currentUnit_, &typeSpeller_, scope_);
        }
        node = depGraph_.addNode(node);
        // For each pair of <expression, statement>
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            ExpressionAST* exp = it2->first;
            if (exp->isCallAST() || exp->isIdExpressionAST()) {
                // Add the expression node to the graph
                currentStatement_ = it2->second;
                currentValues_.clear();
                currentNodes_.clear();
                visitExpression(it2->first);
                if (computedNode_)
                    computedNode_ = depGraph_.addNode(computedNode_);
                if (computedNode_ && computedNode_->type() == NTArray)
                    depGraph_.merge(node, computedNode_);
                else if (!computedNode_ || computedNode_->type() != NTAffine) {
                    std::cout << ANSI_COLOR_YELLOW << "[FunctionGenerator] Fail to extract ArrayNode from definition expression (ignored)."
                              << ANSI_COLOR_RESET << std::endl;
                }
            }
        }
    }

    for (auto itPointerIsArray : ra->pointerIsArray_) {
        if (itPointerIsArray.second) {
            auto it = ra->arrayInfoMap_.find(itPointerIsArray.first);
            if (it != ra->arrayInfoMap_.end()) {
                const Symbol* currentArray = it->first;
                if (isVisible(currentArray, previousScope, scope_)) {
                    NodeDependenceGraph* node;
                    node = depGraph_.find(currentArray); // Is there an input array with this symbol?
                    // If not it is a local array
                    if (!node)
                        node = new ArrayNode(currentArray, Local, currentUnit_, &typeSpeller_, scope_);
                    if (node->type() != NTArray) {
                        std::cout << ANSI_COLOR_RED << "[FunctionGenerator] Invalid constraint on a non array node (" << getName(node->type()) << ")"
                                  << ANSI_COLOR_RESET << std::endl;
                        exit(1);
                    }
                    ArrayNode* arrayNode = static_cast<ArrayNode*>(node);
                    ArrayInfo *info = &(it->second);
                    for (unsigned int i = 0; i < info->dimensionIsFixed_.size(); i++) {
                        // Add the length to the size if fixed
                        if (info->dimensionIsFixed_[i]) {
                            arrayNode->minimumSizeCstrt(i, *(info->dimensionLength(i)->evaluate()) - *(IntegerValue(1).evaluate()));
                        }
                    }
                    NodeDependenceGraph* n = dynamic_cast<NodeDependenceGraph *>(arrayNode);
                    depGraph_.addNode(n);
                }
            }
        }
    }

    // Step 4. Create the nodes for each array access (node of the index)
    // Link them to the corresponding array node
    for (auto it = ra->arrayAccesses_.begin(); it != ra->arrayAccesses_.end(); it++) {
        // it->first == pair<Symbol*, int dimension>
        const Symbol* currentArray = it->first.first;
        unsigned dim = it->first.second;
        if (isVisible(currentArray, previousScope, scope_)) {
            // it->second == list< pair<Expression*, Statement*> >
            // For each pair of <expression, statement>
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                // it2 == pair<Expression*, Statement*>
                // Add the expression node to the graph
                currentStatement_ = it2->second;
                currentValues_.clear();
                currentNodes_.clear();
                visitExpression(it2->first);
                for (auto node : currentNodes_) {
                    depGraph_.addLabeledEdge(node, currentArray, dim);
                }
            }
        }
    }

    depGraph_.writeDotFile(translationUnit()->fileName(), "_" + funName + "_init");
    depGraph_.simplify();
    depGraph_.writeDotFile(translationUnit()->fileName(), "_" + funName + "_simpl");

    // Step 5. First phase, spread information from root to leave, from
    // Numerical nodes to Array ones
    if (!depGraph_.spreadingTopDown()) {
        std::cout << ANSI_COLOR_RED << "[ERROR] FunctionGenerator: The Top Down phase has failed" << ANSI_COLOR_RESET << std::endl;
        exit(1);
    }
    depGraph_.writeDotFile(translationUnit()->fileName(), "_" + funName + "_topdown");

    // Step 6. Second phase, spread information from array to numerical nodes
    if (!depGraph_.spreadingBottomUp()) {
        std::cout << ANSI_COLOR_RED << "[ERROR] FunctionGenerator: The Bottom Up phase has failed" << ANSI_COLOR_RESET << std::endl;
        exit(1);
    }
    depGraph_.writeDotFile(translationUnit()->fileName(), "_" + funName + "_bottomup");

    std::cout << "[FunctionGenerator] Number of nodes in the Dependence Graph: " << depGraph_.size() << std::endl;

    // -- At this point all constraints have been calculated --

    // Create the main file that will contained the stubs, variable initialisations and call of the function
    ofstream outputFile;
    string fileName = translationUnit()->fileName();
    string path = "";
    if (fileName.find_last_of("/") != string::npos) {
        path = fileName.substr(0, fileName.find_last_of("/"));
        fileName = fileName.substr(fileName.find_last_of("/") + 1);
    }
    if (fileName.find_last_of(".") != string::npos)
        fileName = fileName.substr(0, fileName.find_last_of("."));
    string newFile = path + mainFileNamePrefix + fileName + "_" + funName + mainFileNameSuffix;
    outputFile.open(newFile);
    cout << "Main file written in " << newFile << endl;

    // Include the source file and the header file
    outputFile << "#include \"" << includeFileName << "\"" << endl
               << "#include \"../" << fileName << ".c\"" << endl
               << endl;

    // Array size limit
    outputFile << "#define " << maxArraySizeCst << " " << maxArraySizeValue << endl;
    outputFile << "#define " << minArraySizeCst << " " << minArraySizeValue << endl;
    outputFile << "#define " << nbTestsCst << " " << nbTestsValue << endl;
    outputFile << "#define " << nbCallCst << " 10" << endl;

    outputFile << "// Declare array size variables" << endl
               << depGraph_.arraySizeVars() << endl;

    // Add stubs
    outputFile << "// Stubs" << endl
               << depGraph_.stubs() << endl;

    // Main
    outputFile << "int main(int argc, const char* argv[]) {" << endl
               << "  srand(time(NULL));" << endl
               << "  int savingVar;" << endl
               << "  int currentTest;" << endl;

    if (generateCSV) {
        outputFile << "  FILE *csv_result = fopen(\"" << csvFileNamePrefix << fileName << "result.csv\", \"w\" );" << endl;
        outputFile << "  fprintf(csv_result, \"" << depGraph_.headerCSV() << "INVALID_RAND, execution time (%d calls)\\n\", " << nbCallCst << ");" << endl;
    }

    outputFile << "  for (currentTest = 0; currentTest < " << nbTestsCst << "; currentTest++) {" << endl
               << "    INVALID_RAND = 0;"
               << endl
               << depGraph_.initVariables() // construct the initialization graph, sort the things and return a string with the definitions
               << endl;

    if (generateCSV)
        outputFile << "    clock_t begin = clock();" << endl
                   << "    int it_call;" << endl
                   << "    for (it_call = 0; it_call < " << nbCallCst << "; it_call++) {" << endl;

    outputFile << "      if (INVALID_RAND != 1) {" << endl
               << "        " << func->name()->asNameId()->chars()
               << "(";
    // Arguments
    if (func->hasArguments()) {
        outputFile << func->argumentAt(0)->name()->asNameId()->chars();
        // Scanning arguments
        for (unsigned int i = 1; i < func->argumentCount(); i++) {
            outputFile << ", " << func->argumentAt(i)->name()->asNameId()->chars();
        }
    }

    outputFile << ");" << endl
               << "      }" << endl; // if (INVALID_RAND != 1)

    if (generateCSV)
        outputFile << "    }" << endl // for (it_call = 0; it_call ...
                   << "    clock_t end = clock();" << endl
                   << "    float time_spent = ((float)(end - begin))/(float)(CLOCKS_PER_SEC);" << endl
                   << "    fprintf(csv_result, " << depGraph_.valuesCSV() << ");" << endl;

    outputFile << endl
               << depGraph_.freeArrays()
               << "  }" << endl;

    if (generateCSV)
        outputFile << "  fclose(csv_result);" << endl;

    outputFile << "  return 0;" << endl;
    outputFile << "}" << endl;

    outputFile.close();

    std::cout << "Stub successfully written." << std::endl;

    depGraph_.clear();

    switchScope(previousScope);

    return false;
}

bool isBuiltIn(std::string& name)
{
    if (name == "printf")
        return true;

    return false;
}

bool FunctionGenerator::visit(SimpleDeclarationAST *ast)
{
    DEBUG_VISIT(SimpleDeclarationAST);

    for (const List<Symbol*> *symIt = ast->symbols; symIt; symIt = symIt->next) {
        // Function (without a body)
        if (symIt->value->type()->isFunctionType()) {

            string functionName = symIt->value->name()->asNameId()->chars();
            if (isBuiltIn(functionName))
                continue;

            Function *fct = symIt->value->type()->asFunctionType();
            if (fct->returnType()->isPointerType() ||
                fct->returnType()->isArrayType()) {
                depGraph_.addNode(new ArrayNode(symIt->value, ReturnOfFunction, currentUnit_, &typeSpeller_, scope_));
            }
            else {
                depGraph_.addNode(new UncompletedFunctionNode(symIt->value, currentUnit_, &typeSpeller_, scope_));
            }
        }
        // Global variable
        else {
            if (symIt->value->type()->isPointerType() ||
                symIt->value->type()->isArrayType()) {
                depGraph_.addNode(new ArrayNode(symIt->value, Input, currentUnit_, &typeSpeller_, scope_));
            }
            else {
                depGraph_.addNode(new GlobalVarNode(symIt->value, currentUnit_, &typeSpeller_, scope_));
            }
        }
    }

    return false;
}

bool FunctionGenerator::visit(ArrayDeclaratorAST *ast)
{
    DEBUG_VISIT(ArrayDeclaratorAST);

    PSYCHE_ASSERT(false, return false, "ArrayDeclarator not handled yet");
    return false;
}

void FunctionGenerator::visitExpression(ExpressionAST *ast)
{
    DEBUG_VISIT(ExpressionAST);

    currentValue_.reset(nullptr);
    currentValues_.clear();
    currentNodes_.clear();

    accept(ast); // Generate a currentValue_ from the ast expression OR an ArrayNode in computedNode

    if (!currentValues_.empty())
        currentNodes_.clear();

    for (auto valIt = currentValues_.begin(); valIt != currentValues_.end(); ++valIt) {
        currentValue_.reset((*valIt)->clone().release()); // currentValue_ = (*valIt)->clone()
        simplifyToAffine();
        if (currentValue_) { // If the simplification succeed
            AffineNode *expr = new AffineNode(currentValue_->clone(), currentUnit_, &typeSpeller_, scope_);
            computedNode_ = depGraph_.addNode(expr);
            currentNodes_.insert(computedNode_);
            linkAbstractValue(currentValue_->clone(), computedNode_);
            currentValue_.reset(nullptr);
        } else {
            std::cout << ANSI_COLOR_YELLOW
                      << "[FunctionGenerator] Fail to simplify to affine form the expression: "
                      << (*valIt)->toString()
                      << ". It is ignored."
                      << ANSI_COLOR_RESET << std::endl;
        }
    }

    return;
}


bool FunctionGenerator::visit(BinaryExpressionAST* ast)
{
    DEBUG_VISIT(BinaryExpressionAST);

    unsigned op = tokenKind(ast->binary_op_token);

    std::unique_ptr<AbstractValue> neutral;
    switch(op) {
    case T_PLUS:
    case T_PLUS_EQUAL:
    case T_MINUS:
    case T_MINUS_EQUAL:
    case T_GREATER_GREATER:
    case T_LESS_LESS:
        neutral = IntegerValue(0).clone();
        break;
    case T_STAR:
    case T_STAR_EQUAL:
    case T_SLASH:
    case T_SLASH_EQUAL:
        neutral = IntegerValue(1).clone();
        break;
    }
    accept(ast->left_expression);
    std::set<std::unique_ptr<AbstractValue> > leftValues;
    if (currentValues_.empty()) {
        PSYCHE_ASSERT(neutral, return false, "invalid left expression");
        leftValues.insert(neutral->clone());
    } else {
        for (auto valIt = currentValues_.begin(); valIt != currentValues_.end(); valIt++) {
            leftValues.insert((*valIt)->clone());
        }
    }

    currentValues_.clear();
    accept(ast->right_expression);
    std::set<std::unique_ptr<AbstractValue> > rightValues;
    if (currentValues_.empty()) {
        PSYCHE_ASSERT(neutral, return false, "invalid right expression");
        rightValues.insert(neutral->clone());
    } else {
        for (auto valIt = currentValues_.begin(); valIt != currentValues_.end(); valIt++) {
            rightValues.insert((*valIt)->clone());
        }
    }


    currentValues_.clear();
    switch(op) {
    case T_PLUS:
    case T_PLUS_EQUAL:
        for (auto lav = leftValues.begin(); lav != leftValues.end(); lav++) {
            for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
                currentValues_.insert((**lav + **rav)->evaluate());
            }
        }
        break;
    case T_MINUS:
    case T_MINUS_EQUAL:
        for (auto lav = leftValues.begin(); lav != leftValues.end(); lav++) {
            for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
                currentValues_.insert((**lav - **rav)->evaluate());
            }
        }
        break;
    case T_STAR:
    case T_STAR_EQUAL:
        for (auto lav = leftValues.begin(); lav != leftValues.end(); lav++) {
            for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
                currentValues_.insert((**lav * **rav)->evaluate());
            }
        }
        break;
    case T_SLASH:
    case T_SLASH_EQUAL:
        for (auto lav = leftValues.begin(); lav != leftValues.end(); lav++) {
            for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
                currentValues_.insert((**lav / **rav)->evaluate());
            }
        }
        break;
    case T_GREATER_GREATER:
        for (auto lav = leftValues.begin(); lav != leftValues.end(); lav++) {
            for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
                currentValues_.insert((**lav >> **rav)->evaluate());
            }
        }
        break;
    case T_LESS_LESS:
        for (auto lav = leftValues.begin(); lav != leftValues.end(); lav++) {
            for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
                currentValues_.insert((**lav << **rav)->evaluate());
            }
        }
        break;
    case T_EQUAL:
        for (auto rav = rightValues.begin(); rav != rightValues.end(); rav++) {
            currentValues_.insert((*rav)->evaluate());
        }
        break;
    default:
        PSYCHE_ASSERT(false, return false, "binary operator " + std::string(CPlusPlus::Token::name(op)) + " not handled");
        currentValues_.clear();
        break;
    }

    return false;
}

bool FunctionGenerator::visit(IdExpressionAST* ast)
{
    DEBUG_VISIT(IdExpressionAST);

    const Name* name = ast->name->name;

    const Symbol* symb = findSymbol(name);
    if (!symb)
        return false;

    NodeDependenceGraph* n = depGraph_.find(symb);
    if (!n && (symb->type()->isPointerType() ||
               symb->type()->isArrayType())) {
        computedNode_ = depGraph_.find(symb);
        if (!computedNode_) {
            computedNode_ = new ArrayNode(symb, Local, currentUnit_, &typeSpeller_, scope_);
            computedNode_ = depGraph_.addNode(computedNode_);
        }
    }
    else if (!n) { // Looking at the variable in the range analysis result
        Range* rg(NULL);
        for (auto it = dependentTypesGenerator_.ra_.rangeAnalysis_[currentStatement_].begin();
                it != dependentTypesGenerator_.ra_.rangeAnalysis_[currentStatement_].end(); ++it) {
            if (it->first == symb) {
                delete rg;
                rg = new Range(it->second);
            }
        }
        PSYCHE_ASSERT(rg, return false, "local variable as no range\n");
        addLocalVarComponents(symb, rg);

        delete rg;
    }
    else { // Already exists
        if (n->type() == NTArray) {
            computedNode_ = n;
        }
        else {
            computedNode_ = NULL;
            currentValues_.insert(SymbolValue(symb).clone());
        }
    }
    return false;
}
bool FunctionGenerator::visit(MemberAccessAST* ast)
{
    DEBUG_VISIT(MemberAccessAST);
    // TODO
    return false;
}
bool FunctionGenerator::visit(NumericLiteralAST* ast)
{
    DEBUG_VISIT(NumericLiteralAST);

    const NumericLiteral *numLit = numericLiteral(ast->literal_token);
    PSYCHE_ASSERT(numLit, return false, "numeric literal must exist");

    // We're interested only on natural numbers. In the case this literal
    // is a floating-point, our interpretation is to truncate it.
    auto value = std::atoi(numLit->chars());

    computedNode_ = NULL;
    currentValues_.insert(std::make_unique<IntegerValue>(value));

    return false;
}
bool FunctionGenerator::visit(PostIncrDecrAST* ast)
{
    DEBUG_VISIT(PostIncrDecrAST);

    visitExpression(ast->base_expression);

    return false;
}
bool FunctionGenerator::visit(CallAST* ast)
{
    DEBUG_VISIT(CallAST);

    visitExpression(ast->base_expression);

    return false;
}
bool FunctionGenerator::visit(UnaryExpressionAST* ast)
{
    DEBUG_VISIT(UnaryExpressionAST);

    visitExpression(ast->expression);

    std::unique_ptr<IntegerValue> one = make_unique<IntegerValue>(1);
    std::unique_ptr<IntegerValue> zero = make_unique<IntegerValue>(0);
    switch(tokenKind(ast->unary_op_token)) {
    case T_PLUS_PLUS:
        for (auto av = currentValues_.begin(); av != currentValues_.end(); av++) {
            currentValues_.insert((**av + *one)->evaluate());
        }
        break;
    case T_MINUS_MINUS:
        for (auto av = currentValues_.begin(); av != currentValues_.end(); av++) {
            currentValues_.insert((**av - *one)->evaluate());
        }
        break;
    case T_MINUS:
        for (auto av = currentValues_.begin(); av != currentValues_.end(); av++) {
            currentValues_.insert((*zero - **av)->evaluate());
        }
        break;
    default:
        unsigned op = tokenKind(ast->unary_op_token);
        PSYCHE_ASSERT(false, return false, "unary operator " + std::string(CPlusPlus::Token::name(op)) + " not handled");
        currentValues_.clear();
        break;
    }

    return false;
}

void FunctionGenerator::linkAbstractValue(std::unique_ptr<AbstractValue> value, NodeDependenceGraph *node)
{
    value->buildSymbolDependence();
    auto listOfDep = value->symbolDep_;
    for (auto s = listOfDep.begin(); s != listOfDep.end(); s++) {
        NodeDependenceGraph* prt = depGraph_.find(*s);
        if (node && prt != node) {
            depGraph_.addEdge(prt, node);
        }
    }
}

std::unique_ptr<NAryValue> FunctionGenerator::extractOneTerm(std::unique_ptr<AbstractValue> formula,
                                                             bool opposed) {
    // Simple Symbol
    if (formula->getKindOfValue() == KSymbol) {
        return std::make_unique<NAryValue>(
                            std::make_unique<IntegerValue>(opposed?-1:1),
                            formula->clone(),
                            Multiplication);
    }
    // An expression
    else if (formula->getKindOfValue() == KNAry) {
        NAryValue* nv = static_cast<NAryValue *>(formula.get());
        Operation op = nv->op_;
        // One term
        if (op == psyche::Operation::Multiplication) {
            // Copy terms
            IntegerValue fact(opposed?-1:1);
            NAryValue f(fact.clone(),
                        formula->clone(),
                        Multiplication);
            f.simplify();
            // There must be at least two terms
            if (f.terms_.size() <= 1)
                return NULL;
            // Else we search the integer and symbol (there should be only one)
            // If several, the symbols produce a new ProductNode and Symbol to represent them
            else {
                AV factor = IntegerValue(1).clone();
                SymbolValue *symb = NULL;
                for (auto it = f.terms_.begin(); it != f.terms_.end(); it++) {
                    if ((*it)->getKindOfValue() == KInteger) {
                        factor = (*it)->clone();
                    }
                    else if ((*it)->getKindOfValue() == KSymbol && !symb) {
                        symb = static_cast<SymbolValue *>((*it)->clone().release());
                    }
                    else if ((*it)->getKindOfValue() == KSymbol) {
                        // Must add a symbol
                        symb->buildSymbolDependence();
                        (*it)->buildSymbolDependence();
                        auto leftSymb = *(symb->symbolDep_.begin());
                        auto leftNode = depGraph_.find(leftSymb);
                        auto rightSymb = *((*it)->symbolDep_.begin());
                        auto rightNode = depGraph_.find(rightSymb);
                        PSYCHE_ASSERT(leftNode, return NULL, "Unknown symbol");
                        PSYCHE_ASSERT(rightNode, return NULL, "Unknown symbol");
                        NodeDependenceGraph *newNode = new ProductNode(leftSymb, leftNode, rightSymb, rightNode, currentUnit_, &typeSpeller_, scope_);
                        newNode = depGraph_.addNode(newNode);
                        depGraph_.addEdge(leftNode, newNode);
                        depGraph_.addEdge(rightNode, newNode);
                        delete symb;
                        symb = new SymbolValue(*(newNode->defineSymbols().begin()));
                    }
                }
                if (!symb)
                    return NULL;
                return std::make_unique<NAryValue>(factor->clone(),
                                                   symb->clone(),
                                                   Multiplication);
            }
        }
    }
    // Any other case is not handled
    return NULL;
}

void FunctionGenerator::simplifyToAffine()
{ // TODO : error messages
    if (!currentValue_)
        return;
    std::list<std::unique_ptr<AbstractValue> > terms;

    currentValue_.reset(currentValue_->evaluate()->develop().release());

    if (currentValue_->getKindOfValue() == KInteger) {
        terms.push_back(currentValue_->clone());
    }
    else if (currentValue_->getKindOfValue() == KSymbol) {
        terms.push_back(std::make_unique<IntegerValue>(0)); // Need of a constant so we add 0 at the beginning
        std::unique_ptr<NAryValue> trm = extractOneTerm(currentValue_->clone());
        if (trm)
            terms.push_back(trm->clone());
        else {
            currentValue_.reset(nullptr);
            return;
        }
    }
    else if (currentValue_->getKindOfValue() == KNAry) {
        NAryValue *formula2 = static_cast<NAryValue *>(currentValue_.get());
        // One term
        if (formula2->op_ == Multiplication) {
            terms.push_back(std::make_unique<IntegerValue>(0)); // Zero constant
            std::unique_ptr<NAryValue> trm = extractOneTerm(formula2->clone());
            if (trm)
                terms.push_back(trm->clone());
            else {
                currentValue_.reset(nullptr);
                return;
            }
        }
        else if (formula2->op_ == Addition || formula2->op_ == Subtraction) {
            bool opposed = (formula2->op_ == Subtraction);
            NAryValue f(std::make_unique<IntegerValue>(0),
                        currentValue_->clone(),
                        formula2->op_);
            f.simplify();
            // Find constant (there should be only one after simplification)
            for (auto it = f.terms_.begin(); it != f.terms_.end(); it++) {
                if (it->get()->getKindOfValue() == KInteger) {
                    terms.push_back(it->get()->clone());
                    break;
                }
            }
            if (terms.empty())
                terms.push_back(std::make_unique<IntegerValue>(0));
            // For each term
            for (auto it = f.terms_.begin(); it != f.terms_.end(); it++) {
                // Note: term will return NULL for an IntegerValue thus ignoring the constant
                std::unique_ptr<NAryValue> trm = extractOneTerm(it->get()->clone(), opposed);
                if (trm) {
                    terms.push_back(trm->clone());
                }
                else if (it->get()->getKindOfValue() != KInteger && it->get()->getKindOfValue() != KEmpty) { // Exraction failed for a non-IntegerValue
                    currentValue_.reset(nullptr);
                    return;
                }
            }
        }
        else { // Another operator, not handled
            currentValue_.reset(nullptr);
            return;
        }
    }

    // We have the list of terms, we can now produce the affine NAryValue
    if (!terms.empty()) {
        currentValue_.reset(new NAryValue(terms, Addition));
    }
    else {
        currentValue_.reset(nullptr);
    }
}

void FunctionGenerator::addLocalVarComponents(const Symbol *symbol, Range *rg)
{
    // Build the AffineNode
    // Lower
    if (rg->lower()->getKindOfValue() == KNAry) {
        AV l = rg->lower();
        NAryValue *formula2 = static_cast<NAryValue *>(l.get());
        Operation op = formula2->op_;
        if (op == Maximum || op == Minimum) {
            std::list<std::unique_ptr<AbstractValue> > terms = rg->lower()->termsClone();
            for (auto it = terms.begin(); it != terms.end(); it++) {
                currentValues_.insert((*it)->evaluate());
            }
        } else {
            currentValues_.insert(rg->lower()->evaluate());
        }
    } else {
        currentValues_.insert(rg->lower()->evaluate());
    }
    // Upper
    if (rg->upper()->getKindOfValue() == KNAry) {
        AV u = rg->upper();
        NAryValue *formula2 = static_cast<NAryValue *>(u.get());
        Operation op = formula2->op_;
        if (op == Maximum || op == Minimum) {
            std::list<std::unique_ptr<AbstractValue> > terms = rg->upper()->termsClone();
            for (auto it = terms.begin(); it != terms.end(); it++) {
                currentValues_.insert((*it)->evaluate());
            }
        } else {
            currentValues_.insert(rg->upper()->evaluate());
        }
    } else {
        currentValues_.insert(rg->upper()->evaluate());
    }
}

