#include "path_indices.h"

#include <iostream>
#include <stack>
#include <map>
#include <vector>

#include "path_expressions.h"
#include "graph.h"
#include "util/collections.h"

using namespace std;

namespace simit {
namespace pe {

// class PathIndex
std::ostream &operator<<(std::ostream &os, const PathIndex &pi) {
  if (pi.ptr != nullptr) {
    os << *pi.ptr;
  }
  else {
    os << "empty PathIndex";
  }
  return os;
}

// class SetEndpointPathIndex
SetEndpointPathIndex::SetEndpointPathIndex(const simit::Set &edgeSet)
    : edgeSet{edgeSet} {
  // TODO: Generalize to support gaps in the future
  iassert(edgeSet.isHomogeneous())
      << "Must be homogeneous because otherwise there are gaps";
}

unsigned SetEndpointPathIndex::numElements() const {
    return edgeSet.getSize();
}

unsigned SetEndpointPathIndex::numNeighbors(unsigned elemID) const {
  return edgeSet.getCardinality();
}

unsigned SetEndpointPathIndex::numNeighbors() const {
  return numElements() * edgeSet.getCardinality();
}

SetEndpointPathIndex::Neighbors
SetEndpointPathIndex::neighbors(unsigned elemID) const {
  class SetEndpointNeighbors : public PathIndexImpl::Neighbors::Base {
    class Iterator : public PathIndexImpl::Neighbors::Iterator::Base {
    public:
      Iterator(const simit::Set::Endpoints::Iterator &epit) : epit(epit) {}

      void operator++() {++epit;}
      unsigned operator*() const {return epit->getIdent();}
      Base* clone() const {return new Iterator(*this);}

    protected:
      bool eq(const Base& o) const {
        const Iterator *other = static_cast<const Iterator*>(&o);
        return epit == other->epit;
      }

    private:
      simit::Set::Endpoints::Iterator epit;
    };

  public:
    SetEndpointNeighbors(const simit::Set::Endpoints &endpoints)
        : endpoints(endpoints) {}

    Neighbors::Iterator begin() const {return new Iterator(endpoints.begin());}
    Neighbors::Iterator end() const {return new Iterator(endpoints.end());}

  private:
    simit::Set::Endpoints endpoints;
  };

  return new SetEndpointNeighbors(edgeSet.getEndpoints(ElementRef(elemID)));
}

void SetEndpointPathIndex::print(std::ostream &os) const {
  os << "SetEndpointPathIndex:";
  for (auto &e : *this) {
    os << "\n" << "  " << e << ": ";
    for (auto ep : neighbors(e)) {
      os << ep << " ";
    }
  }
}

// class SegmentedPathIndex
SegmentedPathIndex::Neighbors
SegmentedPathIndex::neighbors(unsigned elemID) const {
  class SegmentNeighbors : public PathIndexImpl::Neighbors::Base {
    class Iterator : public PathIndexImpl::Neighbors::Iterator::Base {
    public:
      /// nbrs points to the neighbor segment of the current element.
      Iterator(unsigned currNbr, const unsigned *nbrs)
          : currNbr(currNbr), nbrs(nbrs) {}

      void operator++() {++currNbr;}
      unsigned operator*() const {return nbrs[currNbr];}
      Base* clone() const {return new Iterator(*this);}

    protected:
      bool eq(const Base& o) const {
        const Iterator *other = static_cast<const Iterator*>(&o);
        return currNbr == other->currNbr;
      }

    private:
      unsigned currNbr;
      const unsigned *nbrs;
    };

  public:
    SegmentNeighbors(unsigned numNbrs, const unsigned *nbrs)
        : numNbrs(numNbrs), nbrs(nbrs) {}

    Neighbors::Iterator begin() const {return new Iterator(0, nbrs);}
    Neighbors::Iterator end() const {return new Iterator(numNbrs, nbrs);}

  private:
    unsigned numNbrs;
    const unsigned *nbrs;
  };

  return new SegmentNeighbors(numNeighbors(elemID), &nbrs[nbrsStart[elemID]]);
}

void SegmentedPathIndex::print(std::ostream &os) const {
  os << "SegmentedPathIndex:";
  os << "\n  ";
  for (size_t i=0; i < numElements()+1; ++i) {
    os << nbrsStart[i] << " ";
  }
  os << "\n  ";
  for (size_t i=0; i < numNeighbors(); ++i) {
    os << nbrs[i] << " ";
  }
}


// class PathIndexBuilder
PathIndex PathIndexBuilder::buildSegmented(const PathExpression &pe,
                                           unsigned sourceEndpoint){
  iassert(pe.isBound())
      << "attempting to build an index from a path expression (" << pe
      << ") that is not bound to sets";

  /// Interpret the path expression, starting at sourceEndpoint, over the graph.
  /// That is given an element, the find its neighbors through the paths
  /// described by the path expression.
  class PathNeighborVisitor : public PathExpressionVisitor {
  public:
    struct Location {
      PathExpression pathExpr;
      unsigned endpoint;
    };
    typedef map<Var, vector<Location>> VarToLocationsMap;

    PathNeighborVisitor(PathIndexBuilder *builder) : builder(builder) {}

    PathIndex build(const PathExpression &pe) {
      pe.accept(this);
      PathIndex pit = pi;
      pi = nullptr;
      return pit;
    }

  private:
    /// Pack neighbor vectors into a segmented vector (contiguous array).
    PathIndex pack(const map<unsigned, set<unsigned>> &pathNeighbors) {
      unsigned numNeighbors = 0;
      for (auto &p : pathNeighbors) {
        numNeighbors += p.second.size();
      }

      unsigned numElements = pathNeighbors.size();
      unsigned *nbrsStart = new unsigned[numElements + 1];
      unsigned *nbrs = new unsigned[numNeighbors];

      int currNbrsStart = 0;
      for (auto &p : pathNeighbors) {
        unsigned elem = p.first;
        nbrsStart[elem] = currNbrsStart;

        unsigned pNeighborSize = p.second.size();
        if (pNeighborSize > 0) {
          vector<unsigned> pNeighbors(p.second.begin(), p.second.end());
          sort(pNeighbors.begin(), pNeighbors.end());

          memcpy(&nbrs[currNbrsStart], pNeighbors.data(),
                 pNeighbors.size() * sizeof(unsigned));

          currNbrsStart += pNeighborSize;
        }
      }
      nbrsStart[numElements] = currNbrsStart;
      return new SegmentedPathIndex(numElements, nbrsStart, nbrs);;
    }

    void visit(const Link *link) {
      const simit::Set &edgeSet = *link->getEdgeBinding();
      iassert(edgeSet.getCardinality() > 0)
          << "not an edge set" << edgeSet.getName();

      switch (link->getType()) {
        case Link::ev: {
          pi = new SetEndpointPathIndex(edgeSet);
          break;
        }
        case Link::ve: {
          // add each edge to the neighbor vectors of its endpoints
          map<unsigned, set<unsigned>> pathNeighbors;

          // create neighbor lists
          const simit::Set &vertexSet = *link->getVertexBinding();
          for (auto &v : vertexSet) {
            pathNeighbors.insert({v.getIdent(), set<unsigned>()});
          }

          // populate neighbor lists
          for (auto &e : edgeSet) {
            iassert(e.getIdent() >= 0);
            for (auto &ep : edgeSet.getEndpoints(e)) {
              iassert(ep.getIdent() >= 0);
              pathNeighbors.at(ep.getIdent()).insert(e.getIdent());
            }
          }
          pi = pack(pathNeighbors);
          break;
        }
      }
    }

    static VarToLocationsMap
    getVarToLocationsMap(const vector<PathExpression>& pexprs) {
      VarToLocationsMap varToLocationsMap;
      for (const PathExpression& pexpr : pexprs) {
        for (unsigned ep=0; ep < pexpr.getNumPathEndpoints(); ++ep) {
          Location loc;
          loc.pathExpr = pexpr;
          loc.endpoint = ep;
          varToLocationsMap[pexpr.getPathEndpoint(ep)].push_back(loc);
        }
      }
      return varToLocationsMap;
    }

    PathIndex buildIndex(const PathExpression &pathExpr,
                         const Var &source, const Var &sink) {
      VarToLocationsMap locs = getVarToLocationsMap({pathExpr});
      iassert(util::contains(locs, source))
          << "source variable is not in the path expression";
      iassert(util::contains(locs, sink))
          << "sink variable is not in the path expression";
      return builder->buildSegmented(locs.at(source)[0].pathExpr,
                                     locs.at(source)[0].endpoint);
    }

    tuple<PathIndex,PathIndex> buildIndices(const PathExpression &lhs,
                                            const PathExpression &rhs,
                                            const Var &source,
                                            const Var &quantified,
                                            const Var &sink) {
      VarToLocationsMap varToLocations = getVarToLocationsMap({lhs,rhs});
      iassert(varToLocations.find(quantified) != varToLocations.end())
          << "could not find quantified variable locations";
      iassert(varToLocations[quantified].size() == 2)
          << "quantified binary expr only uses quantified variable once";

      Location sourceLoc = varToLocations[source][0];
      PathIndex sourceToQuantified =
          builder->buildSegmented(sourceLoc.pathExpr, sourceLoc.endpoint);
      PathIndex sourceToQuantified2, quantifiedToSink2;

      Location sinkLoc = varToLocations[sink][0];
      unsigned quantifiedLoc = ((sinkLoc.endpoint) == 0) ? 1 : 0;
      PathIndex quantifiedToSink =
          builder->buildSegmented(sinkLoc.pathExpr, quantifiedLoc);

      return {sourceToQuantified, quantifiedToSink};
    }

    void visit(const And *f) {
      auto &freeVars = f->getFreeVars();
      iassert(freeVars.size() == 2)
          << "For now, we only support matrix path expressions";

      PathExpression lhs = f->getLhs();
      PathExpression rhs = f->getRhs();

      map<unsigned, set<unsigned>> pathNeighbors;
      if (!f->isQuantified()) {
        // Build indices from first to second free variable through lhs and rhs
        PathIndex lhsIndex = buildIndex(lhs, freeVars[0], freeVars[1]);
        PathIndex rhsIndex = buildIndex(rhs, freeVars[0], freeVars[1]);

        // Build a path index that is the intersection of lhsIndex and rhsIndex.
        // OPT: If path indices supported efficient lookups we could instead:
        //      for each (elem,nbr) pair in lhs, if it exist in rhs then emit.
        map<unsigned, set<unsigned>> lhsPathNeighbors;
        for (unsigned elem : lhsIndex) {
          lhsPathNeighbors.insert({elem, set<unsigned>()});
          for (unsigned nbr : lhsIndex.neighbors(elem)) {
            lhsPathNeighbors.at(elem).insert(nbr);
          }
        }
        for (unsigned elem : rhsIndex) {
          pathNeighbors.insert({elem, set<unsigned>()});
          for (unsigned nbr : rhsIndex.neighbors(elem)) {
            auto &elemNbrs = lhsPathNeighbors.at(elem);
            if (elemNbrs.find(nbr) != elemNbrs.end()) {
              pathNeighbors.at(elem).insert(nbr);
            }
          }
        }
      }
      else {
        iassert(f->getQuantifiedVars().size() == 1)
            << "For now, we only support one quantified variable";

        QuantifiedVar qvar = f->getQuantifiedVars()[0];

        // The expression combines two binary path expressions with one
        // quantified variable. Thus, each operand must link one of the two free
        // variables to the quantified variable.

        // Build indices from the first free variable to the quantified var,
        // and from the quantified var to the second free variable
        PathIndex sourceToQuantified;
        PathIndex quantifiedToSink;

        tie(sourceToQuantified, quantifiedToSink) =
            buildIndices(lhs, rhs, freeVars[0], qvar.getVar(), freeVars[1]);

        // Build a path index from the first free variable to the second free
        // variable, through the quantified variable.
        for (unsigned source : sourceToQuantified) {
          pathNeighbors.insert({source, set<unsigned>()});
          for (unsigned q : sourceToQuantified.neighbors(source)) {
            for (unsigned sink : quantifiedToSink.neighbors(q)) {
              pathNeighbors.at(source).insert(sink);
            }
          }
        }
      }
      pi = pack(pathNeighbors);
    }

    void visit(const Or *f) {
      auto &freeVars = f->getFreeVars();
      iassert(freeVars.size() == 2)
          << "For now, we only support matrix path expressions";

      PathExpression lhs = f->getLhs();
      PathExpression rhs = f->getRhs();

      map<unsigned, set<unsigned>> pathNeighbors;
      if (!f->isQuantified()) {
        // Build indices from first to second free variable through lhs and rhs
        PathIndex lhsIndex = buildIndex(lhs, freeVars[0], freeVars[1]);
        PathIndex rhsIndex = buildIndex(rhs, freeVars[0], freeVars[1]);

        // Build a path index that is the union of lhsIndex and rhsIndex
        for (unsigned elem : lhsIndex) {
          pathNeighbors.insert({elem, set<unsigned>()});
          for (unsigned nbr : lhsIndex.neighbors(elem)) {
            pathNeighbors.at(elem).insert(nbr);
          }
        }
        for (unsigned elem : rhsIndex) {
          iassert(pathNeighbors.find(elem) != pathNeighbors.end());
          for (unsigned nbr : rhsIndex.neighbors(elem)) {
            pathNeighbors.at(elem).insert(nbr);
          }
        }
      }
      else {
        iassert(f->getQuantifiedVars().size() == 1)
            << "For now, we only support one quantified variable";

        QuantifiedVar qvar = f->getQuantifiedVars()[0];

        // The expression combines two binary path expressions with one
        // quantified variable. Thus, each operand must link one of the two free
        // variables to the quantified variable.

        // Build indices from the first free variable to the quantified var,
        // and from the quantified var to the second free variable
        PathIndex sourceToQuantified;
        PathIndex quantifiedToSink;

        // OPT: The index building algorithm is agnostic to the direction these
        //      indices are built in. We should take advantage by:
        //      - checking whether one direction is already available/memoized
        //      - checking whether one direction is an ev link (which is fast)
        tie(sourceToQuantified, quantifiedToSink) =
            buildIndices(lhs, rhs, freeVars[0], qvar.getVar(), freeVars[1]);

        // Build a path index that from the first free variable to the
        // quantified variable. Every free variable that can reach any
        // quantified variable gets links to every element of the second
        // variable. Vice versa for the second variable, but jump from the
        // quantified var.
        const simit::Set *sinkSet = f->getBinding(freeVars[1]);

        for (unsigned source : sourceToQuantified) {
          pathNeighbors.insert({source, set<unsigned>()});
          if (sourceToQuantified.numNeighbors(source) > 0) {
            for (auto &sinkElem : *sinkSet) {
              unsigned sink = sinkElem.getIdent();
              pathNeighbors.at(source).insert(sink);
            }
          }
        }

        for (unsigned quantified : quantifiedToSink) {
          for (unsigned sink: quantifiedToSink.neighbors(quantified)) {
            for (unsigned source : sourceToQuantified) {
              pathNeighbors.at(source).insert(sink);
            }
          }
        }
      }
      pi = pack(pathNeighbors);
    }

    PathIndex pi;  // Path index returned from cases
    PathIndexBuilder *builder;
  };

  // TODO: Possible optimization is to detect symmetric path expressions, and
  //       return the same path index when they are evaluated in both directions

  // Check if we have memoized the path index for this path expression, starting
  // at this sourceEndpoint, bound to these sets.
  if (util::contains(pathIndices, {pe,sourceEndpoint})) {
    return pathIndices.at({pe,sourceEndpoint});
  }

  PathIndex pi = PathNeighborVisitor(this).build(pe);
  pathIndices.insert({{pe,sourceEndpoint}, pi});
  return pi;
}

}}
