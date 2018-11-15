/******************************************************************************
 * Copyright (c) 2017 Solène Mirliaz (solene.mirliaz@ens-rennes.fr)
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

#include "DependenceGraph.h"
#include "AST.h"
#include "Bind.h"
#include "Assert.h"
#include "Debug.h"
#include "Literals.h"
#include "Lookup.h"
#include "Scope.h"
#include "Symbols.h"
#include "TranslationUnit.h"
#include "NodeDependenceGraph.h"
#include "Utils.h"
#include <queue>
#include <fstream>
#include <sstream>

#define VISITOR_NAME "NodeDepGraph"

using namespace psyche;

DependenceGraph::DependenceGraph()
{
    graph_.clear();
}

DependenceGraph::~DependenceGraph()
{
    for (auto it = graph_.begin(); it != graph_.end(); ++it){
        deleteNode(it->first);
    }
    graph_.clear();
}

void DependenceGraph::clear()
{
    for (auto it = graph_.begin(); it != graph_.end(); ++it){
        deleteNode(it->first);
    }
    graph_.clear();
    std::cout << ANSI_COLOR_GREEN << "[DependenceGraph] Graph cleared."
              << ANSI_COLOR_RESET << std::endl;
}

NodeDependenceGraph *DependenceGraph::find(const CPlusPlus::Symbol *symb)
{
    if (!symb->name())
        return NULL;
    // Starting from the end to have the most recent node
    for (auto it = graph_.rbegin(); it != graph_.rend(); ++it){
        if (it->first->is(symb))
            return it->first;
    }
    return NULL;
}

NodeDependenceGraph *DependenceGraph::find(AffineNode *expr)
{
    if (!expr)
        return NULL;
    // Starting from the end to have the most recent node
    for (auto it = graph_.rbegin(); it != graph_.rend(); ++it){
        if (*expr == *(it->first))
            return it->first;
    }
    return NULL;
}

NodeDependenceGraph* DependenceGraph::addNode(NodeDependenceGraph *node)
{
    auto exist = graph_.find(node);
    if (exist != graph_.end()) {
        return exist->first;
    }
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (*(it->first) == *node) {
            auto oldNode = it->first;
            oldNode->merge(*node);
            graph_.remove_vertex(node);
            deleteNode(node);
            return oldNode;
        }
    }
    if (node)
        graph_.insert_vertex(node);
    return node;
}

void DependenceGraph::addEdge(NodeDependenceGraph *parent, const CPlusPlus::Symbol *childSymb)
{
    if (!parent || !childSymb)
        return;
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->is(childSymb) && (it->first) != parent) {
            graph_.insert_edge(parent, it->first);
            return;
        }
    }
}

void DependenceGraph::addLabeledEdge(NodeDependenceGraph *parent, const CPlusPlus::Symbol *childSymb, int dim)
{
    if (!parent || !childSymb)
        return;
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->is(childSymb) && (it->first) != parent) {
            graph_.insert_edge(parent, it->first);
            labelEdges_[std::make_pair(parent, it->first)].insert(dim);
            return;
        }
    }
}

void DependenceGraph::addEdge(NodeDependenceGraph *parent, NodeDependenceGraph *child)
{
    if (!parent || !child || parent == child)
        return;
    parent = addNode(parent);
    child = addNode(child);
    graph_.insert_edge(parent, child);
}

void DependenceGraph::merge(NodeDependenceGraph *first, NodeDependenceGraph *second)
{
    PSYCHE_ASSERT(first, return, "trying to merge a non-existing node");
    PSYCHE_ASSERT(second, return, "trying to merge a non-existing node");

    const NGraph::tGraph<NodeDependenceGraph*>::vertex_set children = graph_.out_neighbors(second);
    for (auto it = children.begin(); it != children.end(); it++) {
        addEdge(first, *it);
    }
    const NGraph::tGraph<NodeDependenceGraph*>::vertex_set parents = graph_.in_neighbors(second);
    for (auto it = parents.begin(); it != parents.end(); it++) {
        addEdge(*it, first);
    }
    first->merge(*second);
    graph_.remove_vertex(second);
    deleteNode(second);
}

void DependenceGraph::simplify()
{
    for (auto it = graph_.begin(); it != graph_.end();) {
        unsigned int n = (NGraph::tGraph<NodeDependenceGraph*>::out_neighbors(it).size());
        if (n == 0 && (it->first->type() == NTAffine
                       || it->first->type() == NTProduct)) {
            deleteNode(it->first);
            graph_.remove_vertex(it++);
        }
        else
            it++;
    }



    // remove unecessary parents
    NGraph::tGraph<NodeDependenceGraph*>::vertex_set allParents;
    list<pair<NodeDependenceGraph*, AV>> parentValuesOfNodes;
    list<AV> parentValues;

    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        NodeDependenceGraph* node = it->first;
        if (node->type() == NTArray) {
            NGraph::tGraph<NodeDependenceGraph*>::vertex_set parents = graph_.in_neighbors(node);
            for (auto inIt = parents.begin(); inIt != parents.end(); inIt++) {
                NodeDependenceGraph *pnode = *inIt;
                if (pnode->type() == NTAffine) {
                    allParents.insert(pnode);
                    AffineNode* anode = static_cast<AffineNode*>(pnode);
                    parentValues.push_back(anode->getFormula());
                }
            }

            parentValuesOfNodes.push_back(pair<NodeDependenceGraph*, AV>
                                          (node, NAryValue(parentValues, Maximum).clone()));
        }
    } //*/
}

bool DependenceGraph::spreadingTopDown()
{
    std::map<NodeDependenceGraph*, unsigned int> waitFrom;
    std::queue<NodeDependenceGraph*> ready;
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        unsigned int n = (NGraph::tGraph<NodeDependenceGraph*>::in_neighbors(it).size());
        waitFrom[it->first] = n;
        if (n == 0) {
            ready.push(it->first);
        }
    }

    while (!ready.empty()) {
        NodeDependenceGraph* currentNode = ready.front();
        ready.pop();
        const NGraph::tGraph<NodeDependenceGraph*>::vertex_set children = graph_.out_neighbors(currentNode);
        Range *rg = currentNode->downMessage();
        for (auto it = children.begin(); it != children.end(); it++) {
            auto find = labelEdges_.find(std::make_pair(currentNode, (*it)));
            if (find != labelEdges_.end()) {
                ParentType *from = new ParentType;
                for (int v : find->second) {
                    from->dim = v;
                    (*it)->receiveDownMessage(rg, from);
                }
                delete from;
            } else {
                ParentType *from = new ParentType;
                from->symb = std::make_unique<SymbolValue>(*(currentNode->defineSymbols().begin()));
                (*it)->receiveDownMessage(rg, from);
                delete from;
            }
            if (--waitFrom[*it] == 0) {
                ready.push(*it);
            }
        }
        delete rg;
    }
    for (auto it = waitFrom.begin(); it != waitFrom.end(); it++) {
        if (it->second > 0) {
            std::cout << ANSI_COLOR_YELLOW
                      << "[DependenceGraph] A " << getName(it->first->type())
                      << " still waits for " << it->second << " of its parents."
                      << ANSI_COLOR_RESET << std::endl;
            return false;
        }
    }
    return true;
}

bool DependenceGraph::spreadingBottomUp()
{
    dg("bool DependenceGraph::spreadingBottomUp()");

    std::map<NodeDependenceGraph*, unsigned int> waitFrom;
    std::queue<NodeDependenceGraph*> ready;
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        // it = <vertex, pair<set_of_in, set_of_out> >
        unsigned int n = (NGraph::tGraph<NodeDependenceGraph*>::out_neighbors(it).size());
        waitFrom[it->first] = n;
        if (n == 0) {
            ready.push(it->first);
        }
    }

    while (!ready.empty()) {
        AV afExpression; // store the expression inside the affine node
        AV size = nullptr; // value representing the size of the array
        AV minSize = IntegerValue(0).evaluate();; // value representing the size of the array
        AV slice; // length of the range of each term in a affine expression

        NodeDependenceGraph* currentNode = ready.front();
        ready.pop();

        const NGraph::tGraph<NodeDependenceGraph*>::vertex_set parents = graph_.in_neighbors(currentNode);
        IntegerValue numberOfParents(parents.size());

        // affine.expression = 2 + 2a & affine.range = [0, size]
        // size = size - 2
        // affine2.expression = 2a
        if (currentNode->type() == psyche::NodeType::NTAffine) {
            // if the node is an Affine node get its size minus the constant value
            AffineNode* afNode = static_cast<AffineNode*>(currentNode);
            Range rangeOfSize(afNode->getRangePostAsc());
            size = rangeOfSize.upper();
            afExpression = afNode->getFormula();
            NAryValue* n = static_cast<NAryValue*>(afExpression.get());

            AV integer = IntegerValue(0).evaluate();

            for (auto i = n->terms_.begin(); i != n->terms_.end(); i++) {
                if ((*i)->getKindOfValue() == KInteger) {
                    integer = (*i)->clone();
                    break;
                }
            }

            IntegerValue* iv = static_cast<IntegerValue*>(integer.get());
            minSize = IntegerValue(std::abs(iv->getValue())).clone();
            afExpression = *afExpression - *integer;
            size = *size - *integer;
            slice = *size / numberOfParents;
        }

        for (auto it = parents.begin(); it != parents.end(); it++) {
            auto find = labelEdges_.find(std::make_pair((*it), currentNode));
            if (find != labelEdges_.end()) {
                // if the node has a label, then it is used to access some
                // dimension in the vector
                ParentType *to = new ParentType;
                for (int v : find->second) {
                    to->dim = v;
                    Range *rg = currentNode->upMessage(to);
                    (*it)->receiveUpMessage(rg);
                    delete rg;
                }
                delete to;
            } else {
                // this other nodes have no label because they are expressions or
                // used to build another expressions
                if (currentNode->type() == psyche::NodeType::NTAffine) {
                    ParentType *to = new ParentType; // add
                    to->symb = std::make_unique<SymbolValue>(*((*it)->defineSymbols().begin())); // add
                    Range *rg; // range (msg) sended
                    //ProductNode* pnode = static_cast<ProductNode*>(*it);
                    SymbolValue sym = (*it)->getSymbol();//(pnode->getOwnSymbol());
                    NAryValue* n = static_cast<NAryValue*>(afExpression.get());
                    bool send = false;

                    bool find = false;

                    AV msg;

                    for (const auto& it : n->terms_) {
                        if (it->getKindOfValue() == KNAry) {
                            NAryValue* nv = static_cast<NAryValue*>(it.get());

                            if (nv->op_ != Multiplication) {
                                r("ERROR::bool DependenceGraph::spreadingBottomUp():: expecting a multiplication! (" + nv->toString()  + ")");
                                exit(1);
                            }

                            if (nv->terms_.size() == 2) {
                                AV number = IntegerValue(1).clone();
                                if (nv->terms_.front()->getKindOfValue() == KInteger &&
                                        nv->terms_.back()->getKindOfValue() == KSymbol &&
                                        *nv->terms_.back() == sym) {
                                    number = nv->terms_.front()->clone();
                                    find = true;
                                } else if (nv->terms_.back()->getKindOfValue() == KInteger &&
                                           nv->terms_.front()->getKindOfValue() == KSymbol &&
                                           *nv->terms_.front() == sym) {
                                    number = nv->terms_.back()->clone();
                                    find = true;
                                }

                                if (find) {
                                    send = true;
                                    msg = *slice / *number;
                                    //g("      1)send: " + msg->toString() + " for " + sym.toString());
                                    break;
                                }

                            } else {
                                r("ERROR::bool DependenceGraph::spreadingBottomUp():: problem with the affine form! (" + nv->toString()  + ")");
                                exit(1);
                            }
                        } else if (it->getKindOfValue() == KSymbol) {
                            find = true;
                            msg = slice->clone();
                            break;
                        } else {
                            r("ERROR::bool DependenceGraph::spreadingBottomUp():: bad term! (" + it->toString()  + ")");
                            exit(1);
                        }
                    }

                    if (find) {
                        // if we were able to find an upper bound, we  build a range with this limit
                        rg = new Range(minSize->clone(), msg->clone());
                    }
                    else {
                        // this condition ensure that we always asign the 'rg' with something
                        r("ERROR::bool DependenceGraph::spreadingBottomUp()::not able to find a range!");
                        rg = new Range(minSize->clone(), IntegerValue(1).clone());
                        exit(1);
                    }
                    (*it)->receiveUpMessage(rg);
                    delete rg;
                    delete to;
                }
                else {
                    ParentType *to = new ParentType;
                    to->symb = std::make_unique<SymbolValue>(*((*it)->defineSymbols().begin()));
                    Range *rg = currentNode->upMessage(to);
                    (*it)->receiveUpMessage(rg);
                    delete rg;
                    delete to;
                }
            }

            if (--waitFrom[*it] == 0) {
                ready.push(*it);
            }
        }
    }

    // this for below is used basically to check for problems
    for (auto it = waitFrom.begin(); it != waitFrom.end(); it++) {
        if (it->second > 0) {
            std::cout << ANSI_COLOR_YELLOW
                      << "[DependenceGraph] A " << getName(it->first->type())
                      << " still waits for " << it->second << " of its children."
                      << ANSI_COLOR_RESET << std::endl;
            return false;
        }
    }

    return true;
}

string DependenceGraph::arraySizeVars()
{
    std::string ret = "";
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->type() == NTArray) {
            ArrayNode* node = static_cast<ArrayNode *>(it->first);
            ret += node->declaration(); // Declare the size as a global variable
        }
        if (it->first->type() == NTUncompletedFunction) {
            UncompletedFunctionNode* node = static_cast<UncompletedFunctionNode *>(it->first);
            ret += node->declaration();
        }
    }
    return ret;
}

string DependenceGraph::stubs()
{
    std::string ret = "";
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->type() == NTUncompletedFunction) {
            UncompletedFunctionNode* node = static_cast<UncompletedFunctionNode *>(it->first);
            ret += node->def();
        }
        else if (it->first->type() == NTArray) {
            ArrayNode* node = static_cast<ArrayNode *>(it->first);
            ret += node->stubs();
        }
    }
    return ret;
}

bool contains(std::set<const CPlusPlus::Symbol *> symbSet, const CPlusPlus::Symbol* sym) {
    for (auto s : symbSet) {
        if (s->name() == sym->name())
            return true;
    }
    return false;
}

string DependenceGraph::initVariables()
{
    // .dot graph
    std::ofstream ofs("initGraph.dot");
    ofs << "strict digraph DepGraph {" << std::endl;


    // The initialisation dependence graph
    NGraph::tGraph<InitNode*> initGraph_;

    // Add different vertex for each node of the dependence graph
    // valueNode -> SymbolDefinition
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        NGraph::tGraph<NodeDependenceGraph*>::vertex v = it->first;
        if (v->isMutable()) {
            // R -> V
            InitNode *vert = new InitNode, *vert2 = new InitNode;
            vert->node = v;
            vert2->node = v;
            vert->elt = RangeDefinition;
            vert2->elt = SymbolDefinition;
            vert->symbol_ = v->getSymbol();
            vert2->symbol_ = v->getSymbol();
            vert->id_ = v->nextId_++;
            vert2->id_ = v->id_;

            initGraph_.insert_edge(vert, vert2); // R -> S

            string symStr = "no_name";
            if (vert2->symbol_)
                symStr = vert2->symbol_->name()->asNameId()->chars();

            // .dot graph
            ofs << vert->id_ << " [color=\"#006699\",shape=record,label=\"{Range | " + symStr + "}\"];" << endl;
            ofs << vert2->id_ << " [color=\"green\",shape=record,label=\"{Symbol | " + symStr + "}\"];" << endl;
            ofs << vert->id_ << " -> " << vert2->id_ << ";" << endl;
        } else if (v->isArray()) {
            InitNode *vert = new InitNode;
            vert->node = v;
            vert->symbol_ = v->getSymbol();
            vert->elt = ArrayDefinition;
            vert->id_ = v->id_;

            string symbolStr = "no_name";
            if (vert->symbol_)
                symbolStr = vert->symbol_->name()->asNameId()->chars();

            // .dot graph
            ofs << vert->id_ << " [color=\"red\",shape=record,label=\"{Array | " + symbolStr + "}\"];" << endl;

            NGraph::tGraph<ArrayNode*>::vertex av = static_cast<ArrayNode*>(it->first);

            for (auto sizeSymbol : av->sizeSymbols_) {
                InitNode *vert1 = new InitNode, *vert2 = new InitNode;
                vert1->node = v;
                vert2->node = v;
                vert1->elt = RangeSizeDefinition;
                vert2->elt = SizeSymbolDefinition;
                vert1->symbol_ = sizeSymbol;
                vert2->symbol_ = sizeSymbol;
                vert1->id_ = v->nextId_++;
                vert2->id_ = v->nextId_++;

                initGraph_.insert_edge(vert1, vert2); // Ra -> Sa
                initGraph_.insert_edge(vert2, vert); // Sa -> A

                string symStr = "no_name";
                if (vert2->symbol_)
                    symStr = vert2->symbol_->name()->asNameId()->chars();

                // .dot graph
                ofs << vert1->id_ << " [color=\"#006699\",shape=record,label=\"{Range | " + symStr + "}\"];" << endl;
                ofs << vert2->id_ << " [color=\"#006699\",shape=record,label=\"{Symbol | " + symStr + "}\"];" << endl;
                ofs << vert1->id_ << " -> " << vert2->id_ << ";" << endl;
                ofs << vert2->id_ << " -> " << v->id_ << ";" << endl;
            }
        }
    }

    // Connect
    for (auto it = initGraph_.begin(); it != initGraph_.end(); it++) {
        // RANGE REQUIREMENT
        if (it->first->elt == RangeDefinition && it->first->node) {
            std::set<const CPlusPlus::Symbol *> require = it->first->node->definitionsRequiredForRange();
            // Find them
            for (auto itSymb = require.begin(); itSymb != require.end(); itSymb++) {
                for (auto it2 = initGraph_.begin(); it2 != initGraph_.end(); it2++) {
                    if ((it2->first->elt == SymbolDefinition || it2->first->elt == SizeSymbolDefinition) &&
                        it2->first->symbol_ == *itSymb) { // ask if I need this symbol to define my range
                        // V -> R
                        initGraph_.insert_edge(it2->first, it->first);

                        // .dot graph
                        ofs << it2->first->id_ << " -> " << it->first->id_ << ";" << endl;
                    }
                }
            }
        }
    }
    ofs << "  \n}" << std::endl;
    ofs.close();

    dg(" Initialization graph ready! ");

    std::string ret = "";
    // -- Start resolution of the initialisation
    // Initialize each node's waiting counter (Top down)
    // and declare all Input variables
    std::map<InitNode*, unsigned int> waitFrom;
    std::queue<InitNode*> ready;
    for (auto it = initGraph_.begin(); it != initGraph_.end(); it++) {
//b(it->first->toString());
        unsigned int n = (NGraph::tGraph<InitNode*>::in_neighbors(it).size());
        waitFrom[it->first] = n;
//cout << "  " << n << endl;
        if (n == 0) {
            ready.push(it->first);
        }
        // declare inputs
        if (it->first->node && it->first->node->type() == NTInput &&
            it->first->elt == SymbolDefinition) {
            InputNode* node = static_cast<InputNode *>(it->first->node);
            ret += node->declaration();
        }
    }

    dg(" Data Structure for traverse the Initialization graph is ready! ");

    while (!ready.empty()) {
        InitNode* currentNode = ready.front();
        ready.pop();

        std::string def = "";

        switch (currentNode->elt) {
            case RangeDefinition: {
                def += currentNode->node->rangeDefinition();
                break;
            }
            case SymbolDefinition: {
                def += currentNode->node->definition();
                break;
            }
            case ArrayDefinition: {
                NGraph::tGraph<ArrayNode*>::vertex av = static_cast<ArrayNode*>(currentNode->node);
                def += av->definition(currentNode->symbol_);
                break;
            }
            case SizeSymbolDefinition: {
                NGraph::tGraph<ArrayNode*>::vertex av = static_cast<ArrayNode*>(currentNode->node);
                def += av->defineSizeSymbol(currentNode->symbol_);
                break;
            }
            case RangeSizeDefinition: {
                NGraph::tGraph<ArrayNode*>::vertex av = static_cast<ArrayNode*>(currentNode->node);
                def += av->rangeDefinition(currentNode->symbol_);
                break;
            }
            default:
                def += "Bad choice!\n";
                break;
        }

        if (def != "") {
            ret += "//\n" + def;
        }
        // Release children
        auto children = initGraph_.out_neighbors(currentNode);
        for (auto it = children.begin(); it != children.end(); it++) {
            if (--waitFrom[*it] == 0) {
                ready.push(*it);
            }
        }
        initGraph_.remove_vertex(currentNode);
        delete currentNode;
    }

    dg(" Ordem de inicialização definida! ");

    PSYCHE_ASSERT(initGraph_.begin() == initGraph_.end(), return "// FAIL: loop dependence\n", "Looping dep.");
    return ret;
}

string DependenceGraph::freeArrays()
{
    std::string ret = "";
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->type() == NTArray) {
            ArrayNode* node = static_cast<ArrayNode *>(it->first);
            ret += node->free();
        }
    }
    return ret;
}

string DependenceGraph::headerCSV()
{
    std::string ret = "";
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->type() != NTProduct && it->first->type() != NTAffine) {
            ret += it->first->csvName();
        }
    }
    return ret;
}

string DependenceGraph::valuesCSV()
{
    std::string retLeft = "\"", retRight = ", ";
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
        if (it->first->type() != NTProduct && it->first->type() != NTAffine) {
            retLeft += it->first->csvType();
            retRight += it->first->csvName();
        }
    }
    retLeft += "%d, %f \\n\"";
    retRight += "INVALID_RAND, time_spent";
    return retLeft + retRight;
}

void DependenceGraph::writeDotFile(string filename, string suffix)
{
    std::string basename = filename;
    basename.erase(basename.end()-2, basename.end());
    basename.append(suffix);
    basename.append(".depGraph.dot");
    std::ofstream ofs(basename);

    ofs << "strict digraph DepGraph {" << std::endl;
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
      NGraph::tGraph<NodeDependenceGraph*>::vertex v = it->first;
      ofs << v->dotRepresentation() << ";\n";
      const NGraph::tGraph<NodeDependenceGraph*>::vertex_set out = graph_.out_neighbors(it);
      if (out.size() != 0) {
         for (auto it2=out.begin(); it2 != out.end(); it2++) {
             auto find = labelEdges_.find(std::make_pair(v, (*it2)));
             if (find != labelEdges_.end()) {
                 ofs << v->id_ << " -> " << (*it2)->id_ << "[taillabel=\"";
                 for (int val : find->second) {
                     ofs << val << " ";
                 }
                 ofs << "\"];\n";
             } else {
                 ofs << v->id_ << " -> " << (*it2)->id_ << ";\n";
             }
         }
      }
    }

    // Align all Affine nodes
    ofs << "subgraph { \n  rank = same;";
    for (auto it = graph_.begin(); it != graph_.end(); it++) {
      if (it->first->type() == NTAffine)
      {
        ofs << it->first->id_ << ";";
      }
    }
    ofs << "  }\n}" << std::endl;

    std::cout << "[DependenceGraph] Graph dot file written in " << basename << std::endl;

    ofs.close();
}

int DependenceGraph::size()
{
    return graph_.num_nodes();
}

string InitNode::toString()
{
    string ret = "";

    switch (elt) {
        case RangeDefinition: {
            ret += "RangeDefinition";
            break;
        }
        case SymbolDefinition: {
            ret += "SymbolDefinition";
            break;
        }
        case ArrayDefinition: {
            ret += "ArrayDefinition";
            break;
        }
        case SizeSymbolDefinition: {
            ret += "SizeSymbolDefinition";
            break;
        }
        case RangeSizeDefinition: {
            ret += "RangeSizeDefinition";
            break;
        }
        default:
            ret += "noInitElement";
            break;
    }

    ret += ": ";

    if (symbol_)
        ret += symbol_->name()->asNameId()->chars();
    else
        ret += "no_symbol";

    return ret;
}
