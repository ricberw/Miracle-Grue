/* 
 * File:   pather_optimizer_fastgraph.h
 * Author: Dev
 *
 * Created on October 9, 2012, 11:28 AM
 */

#ifndef MGL_PATHER_OPTIMIZER_FASTGRAPH_H
#define	MGL_PATHER_OPTIMIZER_FASTGRAPH_H

#include "pather_optimizer.h"
#include "simple_topology.h"
#include "basic_boxlist.h"
#include "intersection_index.h"
#include "Exception.h"
#include "configuration.h"
#include <iostream>
#include <list>

namespace mgl {

class GraphException : public Exception {
public:
    template <typename T>
    GraphException(const T& arg) : Exception(arg) {}
};

class pather_optimizer_fastgraph : public abstract_optimizer {
public:
    pather_optimizer_fastgraph(const GrueConfig& grueConf)
            : grueCfg(grueConf), historyPoint(std::numeric_limits<Scalar>::min(), 
            std::numeric_limits<Scalar>::min()) {}
    void addPath(const OpenPath& path, const PathLabel& label);
    void addPath(const Loop& loop, const PathLabel& label);
    void addBoundary(const OpenPath& path);
	void addBoundary(const Loop& loop);
    void clearBoundaries();
	void clearPaths();
    void repr_svg(std::ostream& out);
protected:
    void optimizeInternal(LabeledOpenPaths& labeledpaths);
private:
    void optimize1(LabeledOpenPaths& labeledopenpaths);
    bool optimize2(LabeledOpenPaths& labeledopenpaths, LabeledOpenPaths& intermediate);
    class Cost : public PathLabel {
    public:
        Cost(const PathLabel& label = PathLabel(), 
                Scalar distance = 0, 
                PointType normal = PointType()) 
                : PathLabel(label), m_distance(distance), m_normal(normal) {}
        Cost(const Cost& other) : PathLabel(other), 
                m_distance(other.m_distance), m_normal(other.m_normal) {}
        Scalar distance() const { return m_distance; }
        const PointType& normal() const { return m_normal; }
    private:
        Scalar m_distance;
        PointType m_normal;
    };
    class NodeData {
    public:
        NodeData(PointType position, 
                int priority, 
                bool entry) 
                : m_position(position), 
                m_priority(priority), 
                m_isentry(entry) {}
        NodeData() : m_priority(0), m_isentry(false) {}
        const PointType& getPosition() const { return m_position; }
        int getPriority() const { return m_priority; }
        bool isEntry() const { return m_isentry; }
    private:
        PointType m_position;
        int m_priority;
        bool m_isentry;
    };
    
    typedef basic_boxlist<libthing::LineSegment2> boundary_container;
    typedef topo::simple_graph<NodeData, Cost> graph_type;
    typedef graph_type::node node;
    typedef graph_type::node_index node_index;
    typedef std::pair<node_index, Scalar> probe_link_type;
    
    typedef std::vector<graph_type::node_index> node_index_list;
    typedef std::pair<boundary_container, graph_type> bucket;
    typedef std::list<bucket> bucket_list;
    
    typedef graph_type::forward_node_iterator node_iterator;
    
    typedef std::list<LabeledOpenPaths> multipath_type;
    
    class entry_iterator {
    public:
        
        friend class pather_optimizer_fastgraph;
        
        entry_iterator() {}
            
        entry_iterator& operator ++(); //pre
        entry_iterator operator ++(int); //post
        node& operator *();
        node* operator ->() { return &**this; }
        bool operator ==(const entry_iterator& other) const;
        bool operator !=(const entry_iterator& other) const
                { return !(*this==other); }
        
    private:
        explicit entry_iterator(node_iterator base, node_iterator end) 
                : m_base(base), m_end(end) {}
        node_iterator m_base;
        node_iterator m_end;
    };
    
    entry_iterator entryBegin(graph_type& graph);
    entry_iterator entryEnd(graph_type& graph);
    
    class probeCompare {
    public:
        probeCompare(node_index from, graph_type& basis) 
                : m_from(from), m_graph(basis) {}
        bool operator ()(const probe_link_type& lhs, 
                const probe_link_type& rhs) {
            return lhs.second < rhs.second;
        }
    private:
        node_index m_from;
        graph_type& m_graph;
    };
    
    node::forward_link_iterator bestLink(node& from); //can return node::forwardEnd()
    void buildLinks(node& from);
    
    static bool compareConnections(const node::connection& lhs, 
            const node::connection& rhs);
    static bool compareNodes(const node& lhs, const node& rhs);
    static bool comparePathLists(const LabeledOpenPaths& lhs, 
            const LabeledOpenPaths& rhs);
    static size_t countIntersections(libthing::LineSegment2& line, 
            boundary_container& boundContainer);
    
    class nodeComparator {
    public:
        nodeComparator(graph_type& graph, PointType point = 
                PointType(std::numeric_limits<Scalar>::min(), 
                std::numeric_limits<Scalar>::min()))
                : m_graph(graph), m_position(point) {}
        bool operator ()(const node& lhs, const node& rhs) const;
        bool operator ()(node_index lhs, node_index rhs) const;
    private:
        graph_type& m_graph;
        PointType m_position;
    };
    
    bool crossesBounds(const libthing::LineSegment2& line);
    
    void smartAppendPoint(PointType point, PathLabel label, 
            LabeledOpenPaths& labeledpaths, LabeledOpenPath& path);
    void smartAppendPath(LabeledOpenPaths& labeledpaths, LabeledOpenPath& path);
    
    Scalar splitPaths(multipath_type& destionation, const LabeledOpenPaths& source);
    bucket& pickBucket(PointType point);
    
    
    const GrueConfig& grueCfg;
    
    boundary_container boundaries;
    bucket_list buckets;
    graph_type m_graph;
    AABBox boundaryLimits;
    PointType historyPoint;
    
    
};

}

#endif	/* MGL_PATHER_OPTIMIZER_FASTGRAPH_H */
