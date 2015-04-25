/*
 * PortionGraph.h
 * Created on: April 23, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */

#include "ModelObjects.h"
#include "OovString.h"


// A portion graph is an exploded graph of a class.  It displays all of the
// functions and member variables, and the relations between them.

#ifndef PORTION_GRAPH_H
#define PORTION_GRAPH_H


class PortionDrawOptions
    {
    };

enum PortionNodeTypes { PNT_Attribute, PNT_Operation };
class PortionNode
    {
    public:
        PortionNode(OovStringRef name, PortionNodeTypes type):
            mName(name), mNodeType(type)
            {}
        OovString const &getName() const
            { return mName; }
        PortionNodeTypes getNodeType() const
            { return mNodeType; }

    private:
        OovString mName;
        PortionNodeTypes mNodeType;
    };

struct PortionConnection
    {
    PortionConnection(size_t supplierNodeIndex, size_t consumerNodeIndex):
	mSupplierNodeIndex(supplierNodeIndex), mConsumerNodeIndex(consumerNodeIndex)
	{}
    size_t mSupplierNodeIndex;
    size_t mConsumerNodeIndex;
    };

typedef std::vector<PortionConnection> PortionConnections;

class PortionGraph
    {
    public:
	PortionGraph():
	    mModel(nullptr)
	    {}
	void clearGraph()
	    {
	    mNodes.clear();
	    mConnections.clear();
	    }
	// The model must exist for the lifetime of the graph.
        void clearAndAddClass(const ModelData &model, OovStringRef classname);
        PortionNode const *getNode(OovStringRef name) const;
        size_t getNodeIndex(PortionNode const *node) const
            { return(node - &mNodes[0]); }
        std::vector<PortionNode> const &getNodes() const
            { return mNodes; }
        PortionConnections const &getConnections() const
            { return mConnections; }
	bool isModified() const;
	PortionDrawOptions const &getDrawOptions() const
	    { return mDrawOptions; }
	void setDrawOptions(PortionDrawOptions const &options);

    private:
	std::vector<PortionNode> mNodes;
	PortionConnections mConnections;
	const ModelData *mModel;
	PortionDrawOptions mDrawOptions;

	void addConnections(ModelClassifier const *cls);
	// Add connections between all operations of this class.
	void addOperationConnections(ModelClassifier const *classifier,
		ModelStatements const &statements, PortionNode const *operNode);
    };

#endif