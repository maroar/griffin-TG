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

/*! \file */

#ifndef PSYCHE_DEPENDENCE_GRAPH_H__
#define PSYCHE_DEPENDENCE_GRAPH_H__

#include "ngraph.hpp"
#include "NodeDependenceGraph.h"

namespace psyche {

enum InitElement {
    RangeDefinition,
    SymbolDefinition,
    ArrayDefinition,
    SizeSymbolDefinition,
    RangeSizeDefinition
};

struct InitNode {
    NodeDependenceGraph* node;
    InitElement elt;
    const CPlusPlus::Symbol* symbol_;
    unsigned id_;

    std::string toString();
};

class DependenceGraph final {
public:
    DependenceGraph();
    ~DependenceGraph();
    void clear();

    //! Find a node with the same symbol in the graph
    NodeDependenceGraph* find(const CPlusPlus::Symbol* symb);
    //! Find a node with the same expression
    NodeDependenceGraph* find(AffineNode *expr);

    //! Add a node in the graph, possibly merging it with an existing one
    //! \warning Release the memory for the input node if merging
    //! \return The node added or merged with
    NodeDependenceGraph* addNode(NodeDependenceGraph *node);
    void addEdge(NodeDependenceGraph *parent, const CPlusPlus::Symbol* childSymb);
    void addLabeledEdge(NodeDependenceGraph *parent, const CPlusPlus::Symbol* childSymb, int dim);
    void addEdge(NodeDependenceGraph *parent, NodeDependenceGraph *child);
    //! Merge all data of the second node into the first one
    //! \warning Release the memory for the second node
    void merge(NodeDependenceGraph *first, NodeDependenceGraph *second);

    //! Remove useless nodes for the dependence analysis
    //! All expression nodes that do not have children are considered useless
    //! \note Only one step of cleaning is performed, chain of useless nodes are thus not removed
    void simplify();

    //! Starts sending range information.
    //! \return False if the spreading has failed
    bool spreadingTopDown();
    bool spreadingBottomUp();

    //! Return the declarations of array size as global variables as a string of C code
    std::string arraySizeVars();
    //! Return the stubs for each Undefined Function as a string of C code
    std::string stubs();
    //! Return the initialisation of each variable (global and input) as a string of C code
    std::string initVariables();
    //! Return the statements to free all arrays allocated
    std::string freeArrays();

    //! Return the header of the csv file
    std::string headerCSV();
    //! Return the line with all values for the csv file
    std::string valuesCSV();

    //! Write a dot file for this graph
    void writeDotFile(std::string filename, std::string suffix = "");

    int size();

private:
    //! The dependance graph currently generated
    NGraph::tGraph<NodeDependenceGraph*> graph_;
    std::map<NGraph::tGraph<NodeDependenceGraph*>::edge, std::set<int> > labelEdges_;

};

} // namespace psyche
#endif
