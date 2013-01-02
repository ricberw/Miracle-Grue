/* 
 * File:   pather_hierarchical_impl.h
 * Author: Dev
 *
 * Created on December 14, 2012, 12:06 PM
 */

#ifndef MGL_PATHER_HIERARCHICAL_IMPL_H
#define	MGL_PATHER_HIERARCHICAL_IMPL_H

#include "pather_hierarchical_decl.h"
#include "basic_boxlist.h"

namespace mgl {

template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
void pather_hierarchical::InsetTree::traverse(LabeledOpenPaths& result, 
        OptimizerState& entryPoint, const GrueConfig& grueCfg, 
        const LABEL_COMPARE& labeler, 
        const BOUNDARY_TEST& bounder) {
    parent_class::iterator childIter = end();
    while(!empty()) {
        childIter = selectBestChild(entryPoint, labeler, bounder);
        if(childIter == end() || 
                labeler.compare(m_label, childIter->m_label) == BETTER) {
            //we want to do this node before all children worse than it
            if(childIter == end()) {
            } else {
            }
            break;
        }
        childIter->traverse(result, entryPoint, grueCfg, labeler, bounder);
        erase(childIter);
    }
    //traverse self
    traverseInternal(result, entryPoint, grueCfg, labeler, bounder);
    while(!empty()) {
        childIter = selectBestChild(entryPoint, labeler, bounder);
        childIter->traverse(result, entryPoint, grueCfg, labeler, bounder);
        erase(childIter);
    }
}
template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
pather_hierarchical::InsetTree::parent_class ::iterator
        pather_hierarchical::InsetTree::selectBestChild(OptimizerState& entryPoint, 
        const LABEL_COMPARE& labeler, const BOUNDARY_TEST& bounder) {
    //we basically do std::min_element, but store some data to avoid 
    //recomputing things like minimum distance and crossing test results
    parent_class::iterator bestIter = end();
    PathLabel bestLabel;
    bestLabel.myValue = std::numeric_limits<int>::min();
    Scalar bestSquaredDistance = std::numeric_limits<Scalar>::max();
    bool bestBounderResult = false;
    for(parent_class::iterator iter = begin(); 
            iter != end(); 
            ++iter) {
        InsetTree& currentChild = *iter;
        int labelResult = labeler.compare(currentChild.m_label, bestLabel);
        if(labelResult == WORSE) {
            //we don't even look at worse labels
            continue;
        } else if(labelResult == BETTER) {
            //discard all old data, this is our new best
            bestSquaredDistance = std::numeric_limits<Scalar>::max();
            bestBounderResult = false;
            bestLabel = currentChild.m_label;
        }
        Scalar squaredDistance = std::numeric_limits<Scalar>::max();
        bool bounderResult = false;
        for(Loop::const_finite_cw_iterator loopIter = 
                currentChild.boundary().clockwiseFinite(); 
                loopIter != currentChild.boundary().clockwiseEnd(); 
                ++loopIter) {
            Segment2Type testSegment(entryPoint, *loopIter);
            Scalar sd = (entryPoint - *loopIter).squaredMagnitude();
            if(!bounderResult) {
                //no point has yet passed bounder
                if(bounder(testSegment)) {
                    //first point in this loop that passes bounder
                    bounderResult = true;
                    squaredDistance = sd;
                } else {
                    //no point has passed bounder, record shortest distance
                    if(sd < squaredDistance)
                        squaredDistance = sd;
                }
            } else {
                //another point passed bounder
                if(bounder(testSegment)) {
                    //we have a point that passed bounder, consider distances
                    if(sd < squaredDistance)
                        squaredDistance = sd;
                }
            }
        }
        if(!bestBounderResult) {
            //no loop has yet passed bounder
            if(bounderResult) {
                //first loop to pass bounder
                bestBounderResult = true;
                bestSquaredDistance = squaredDistance;
                bestIter = iter;
            } else {
                //no loop has passed bounder, get best distance
                if(squaredDistance < bestSquaredDistance) {
                    bestSquaredDistance = squaredDistance;
                    bestIter = iter;
                }
            }
        } else {
            //another loop passed bounder
            if(bounderResult) {
                //we have a loop that passed bounder, consider distances
                if(squaredDistance < bestSquaredDistance) {
                    bestSquaredDistance = squaredDistance;
                    bestIter = iter;
                }
            }
        }
    }
    return bestIter;
}
template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
void pather_hierarchical::InsetTree::traverseInternal(
        LabeledOpenPaths& result, OptimizerState& entryPoint, 
        const GrueConfig& grueCfg, 
        const LABEL_COMPARE& labeler, 
        const BOUNDARY_TEST& bounder) {
    bool graphTraversed = false;
    //decide if we want to do graph before or after
    for(SpacialGraph::entry_iterator entryIter = m_graph.entryBegin(); 
            entryIter != m_graph.entryEnd(); 
            ++entryIter) {
        if(labeler(entryIter->data().label(), m_label) == BETTER) {
            //do graph before
            graphTraversed = true;
            m_graph.optimize(result, entryPoint, labeler, bounder);
            break;
        }
    }
    //optimize the inset
    if(isValid()) {
        Loop::const_finite_cw_iterator nearestPoint = boundary().clockwiseFinite();
        Scalar nearestDistanceSquared = std::numeric_limits<Scalar>::max();
        bool nearestBoundaryTest = false;
        for(Loop::const_finite_cw_iterator iter = boundary().clockwiseFinite(); 
                iter != boundary().clockwiseEnd(); 
                ++iter) {
            Segment2Type testSegment(entryPoint, *iter);
            Scalar distanceSquared = testSegment.squaredLength();
            bool boundaryTest = bounder(testSegment);
            if(grueCfg.get_doFixedLayerStart()) {
                distanceSquared = Point2Type(*iter).dotProduct(
                        Point2Type(1.0,1.0));
                if(distanceSquared < nearestDistanceSquared) {
                    nearestDistanceSquared = distanceSquared;
                    nearestBoundaryTest = boundaryTest;
                    nearestPoint = iter;
                }
            } else {
                if(!nearestBoundaryTest) {
                    if(boundaryTest) {
                        nearestPoint = iter;
                        nearestDistanceSquared = distanceSquared;
                        nearestBoundaryTest = true;
                    } else {
                        if(distanceSquared < nearestDistanceSquared) {
                            nearestPoint = iter;
                            nearestDistanceSquared = distanceSquared;
                        }
                    }
                } else {
                    if(boundaryTest && distanceSquared < nearestDistanceSquared) {
                        nearestPoint = iter;
                        nearestDistanceSquared = distanceSquared;
                    }
                }
            }
        }
        if(!entryPoint.first() && nearestBoundaryTest) {
            LabeledOpenPath connection(PathLabel(PathLabel::TYP_CONNECTION, 
                    m_label.myOwner, m_label.myValue));
            connection.myPath.appendPoint(entryPoint);
            connection.myPath.appendPoint(*nearestPoint);
            result.push_back(connection);
        }
        entryPoint = *nearestPoint;
        LabeledOpenPath myPath(m_label);
        LoopPath myLoopPath(boundary(), Loop::const_cw_iterator(nearestPoint), 
                Loop::const_ccw_iterator(nearestPoint));
        myPath.myPath.appendPoints(myLoopPath.fromStart(), myLoopPath.end()); 
        result.push_back(myPath);
        entryPoint.setFirst(false);
    }
    //done optimizing the inset
    if(!graphTraversed) //graph wasn't done before, so do after
        m_graph.optimize(result, entryPoint, labeler, bounder);
}
template <typename LABEL_COMPARE>
void pather_hierarchical::OutlineTree::traverse(LabeledOpenPaths& result, 
        OptimizerState& entryPoint, const GrueConfig& grueCfg, 
        const LABEL_COMPARE& labeler) {
    iterator currentChild;
    typedef basic_boxlist<Segment2Type> bounding_type;
    bounding_type boundaries;
    basic_boundary_test<bounding_type> bounder(boundaries);
    constructBoundariesRecursive(boundaries);
    traverse(result, entryPoint, grueCfg, labeler, bounder);
    return;
    while((currentChild = selectBestChild(entryPoint)) != end()) {
        currentChild->traverse(result, entryPoint, grueCfg, labeler);
        erase(currentChild);
    }
    constructBoundaries(boundaries);
    m_insets.traverse(result, entryPoint, grueCfg, labeler, bounder);
    m_graph.optimize(result, entryPoint, labeler, bounder);
}
template <typename LABEL_COMPARE, typename BOUNDARY_TEST>
void pather_hierarchical::OutlineTree::traverse(LabeledOpenPaths& result, 
        OptimizerState& entryPoint, const GrueConfig& grueCfg, 
        const LABEL_COMPARE& labeler, 
        const BOUNDARY_TEST& bounder) {
    iterator currentChild;
    while((currentChild = selectBestChild(entryPoint)) != end()) {
        currentChild->traverse(result, entryPoint, grueCfg, labeler, bounder);
        erase(currentChild);
    }
    entryPoint.setFirst(true);
    m_insets.traverse(result, entryPoint, grueCfg, labeler, bounder);
    m_graph.optimize(result, entryPoint, labeler, bounder);
}
template <typename SPACIAL_CONTAINER>
void pather_hierarchical::OutlineTree::constructBoundaries(
        SPACIAL_CONTAINER& boundaries) const {
    //add all children's boundaries
    for(const_iterator childIter = begin(); 
            childIter != end(); 
            ++childIter) {
        for(Loop::const_finite_cw_iterator loopIter = 
                childIter->boundary().clockwiseFinite(); 
                loopIter != childIter->boundary().clockwiseEnd(); 
                ++loopIter) {
            boundaries.insert(
                    childIter->boundary().segmentAfterPoint(loopIter));
        }
    }
    //insert mine
    for(Loop::const_finite_cw_iterator loopIter = boundary().clockwiseFinite(); 
            loopIter != boundary().clockwiseEnd(); 
            ++loopIter) {
        boundaries.insert(boundary().segmentAfterPoint(loopIter));
    }
}
template <typename SPACIAL_CONTAINER>
void pather_hierarchical::OutlineTree::constructBoundariesRecursive(
        SPACIAL_CONTAINER& boundaries) const {
    //recurse
    for(const_iterator childIter = begin(); 
            childIter != end(); 
            ++childIter) {
        childIter->constructBoundariesRecursive(boundaries);
    }
    //do this node
    for(Loop::const_finite_cw_iterator loopIter = boundary().clockwiseFinite(); 
            loopIter != boundary().clockwiseEnd(); 
            ++loopIter) {
        boundaries.insert(boundary().segmentAfterPoint(loopIter));
    }
}

}

#endif	/* MGL_PATHER_HIERARCHICAL_IMPL_H */
