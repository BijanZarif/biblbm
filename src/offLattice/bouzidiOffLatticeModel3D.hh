/* This file is part of the Palabos library.
 *
 * Copyright (C) 2011-2013 FlowKit Sarl
 * Route d'Oron 2
 * 1010 Lausanne, Switzerland
 * E-mail contact: contact@flowkit.com
 *
 * The most recent release of Palabos can be downloaded at 
 * <http://www.palabos.org/>
 *
 * The library Palabos is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BOUZIDI_OFF_LATTICE_MODEL_3D_HH
#define BOUZIDI_OFF_LATTICE_MODEL_3D_HH

#include "offLattice/bouzidiOffLatticeModel3D.h"
#include "latticeBoltzmann/geometricOperationTemplates.h"
#include "latticeBoltzmann/externalFieldAccess.h"
#include <algorithm>
#include <vector>
#include <cmath>

namespace plb {


template<typename T, template<typename U> class Descriptor>
BouzidiOffLatticeModel3D<T,Descriptor>::BouzidiOffLatticeModel3D (
        BoundaryShape3D<T,Array<T,3> >* shape_, int flowType_)
    : OffLatticeModel3D<T,Array<T,3> >(shape_, flowType_),
      computeStat(true)
{
    typedef Descriptor<T> D;
    invAB.resize(D::q);
    invAB[0] = T();
    for (plint iPop=1; iPop<D::q; ++iPop) {
        invAB[iPop] = (T)1 / sqrt(util::sqr(D::c[iPop][0])+util::sqr(D::c[iPop][1])+util::sqr(D::c[iPop][2]));
    }
}

template<typename T, template<typename U> class Descriptor>
BouzidiOffLatticeModel3D<T,Descriptor>* BouzidiOffLatticeModel3D<T,Descriptor>::clone() const {
    return new BouzidiOffLatticeModel3D(*this);
}

template<typename T, template<typename U> class Descriptor>
plint BouzidiOffLatticeModel3D<T,Descriptor>::getNumNeighbors() const {
    return 1;
}

template<typename T, template<typename U> class Descriptor>
void BouzidiOffLatticeModel3D<T,Descriptor>::prepareCell (
        Dot3D const& cellLocation,
        AtomicContainerBlock3D& container )
{
    typedef Descriptor<T> D;
    Dot3D offset = container.getLocation();
    BouzidiOffLatticeInfo3D* info =
        dynamic_cast<BouzidiOffLatticeInfo3D*>(container.getData());
    PLB_ASSERT( info );
    std::vector<int> solidDirections;
    std::vector<plint> boundaryIds;
    std::vector<bool> hasFluidNeighbor;
    if (this->isFluid(cellLocation+offset)) {
        for (plint iPop=1; iPop<D::q; ++iPop) {
            Dot3D neighbor(cellLocation.x+D::c[iPop][0], cellLocation.y+D::c[iPop][1], cellLocation.z+D::c[iPop][2]);
            Dot3D prevNode(cellLocation.x-D::c[iPop][0], cellLocation.y-D::c[iPop][1], cellLocation.z-D::c[iPop][2]);
            // If the fluid node has a non-fluid neighbor ...
            if (!this->isFluid(neighbor+offset)) {
                plint iTriangle=-1;
                global::timer("intersect").start();
                Array<T,3> locatedPoint;
                T distance;
                Array<T,3> wallNormal;
                Array<T,3> surfaceData;
                OffBoundary::Type bdType;
#ifdef PLB_DEBUG
                bool ok =
#endif
                    this->pointOnSurface (
                            cellLocation+offset, Dot3D(D::c[iPop][0],D::c[iPop][1],D::c[iPop][2]), locatedPoint, distance,
                            wallNormal, surfaceData, bdType, iTriangle );
                // In the following, the importance of directions is sorted wrt. how well they
                //   are aligned with the wall normal. It is better to take the continuous normal,
                //   because it is not sensitive to the choice of the triangle when we shoot at
                //   an edge.
                global::timer("intersect").stop();
                PLB_ASSERT( ok );
                // ... then add this node to the list.
                solidDirections.push_back(iPop);
                boundaryIds.push_back(iTriangle);
                bool prevNodeIsPureFluid = this->isFluid(prevNode+offset);
                if (prevNodeIsPureFluid) {
                    hasFluidNeighbor.push_back(true);
                }
                else {
                    hasFluidNeighbor.push_back(false);
                }
            }
        }
        if (!solidDirections.empty()) {
            info->getBoundaryNodes().push_back(cellLocation);
            info->getSolidDirections().push_back(solidDirections);
            info->getBoundaryIds().push_back(boundaryIds);
            info->getHasFluidNeighbor().push_back(hasFluidNeighbor);
        }
    }
}

template<typename T, template<typename U> class Descriptor>
ContainerBlockData*
    BouzidiOffLatticeModel3D<T,Descriptor>::generateOffLatticeInfo() const
{
    return new BouzidiOffLatticeInfo3D;
}

template<typename T, template<typename U> class Descriptor>
Array<T,3> BouzidiOffLatticeModel3D<T,Descriptor>::getLocalForce (
                AtomicContainerBlock3D& container ) const
{
    BouzidiOffLatticeInfo3D* info =
        dynamic_cast<BouzidiOffLatticeInfo3D*>(container.getData());
    PLB_ASSERT( info );
    return info->getLocalForce();
}

template<typename T, template<typename U> class Descriptor>
void BouzidiOffLatticeModel3D<T,Descriptor>::boundaryCompletion (
        AtomicBlock3D& nonTypeLattice,
        AtomicContainerBlock3D& container,
        std::vector<AtomicBlock3D const*> const& args )
{
    BlockLattice3D<T,Descriptor>& lattice =
        dynamic_cast<BlockLattice3D<T,Descriptor>&> (nonTypeLattice);
    BouzidiOffLatticeInfo3D* info =
        dynamic_cast<BouzidiOffLatticeInfo3D*>(container.getData());
    PLB_ASSERT( info );
    std::vector<Dot3D> const&
        boundaryNodes = info->getBoundaryNodes();
    std::vector<std::vector<int> > const&
        solidDirections = info->getSolidDirections();
    std::vector<std::vector<plint> > const&
        boundaryIds = info->getBoundaryIds();
    std::vector<std::vector<bool> > const&
        hasFluidNeighbor = info->getHasFluidNeighbor();
    PLB_ASSERT( boundaryNodes.size() == solidDirections.size() );
    PLB_ASSERT( boundaryNodes.size() == boundaryIds.size() );
    PLB_ASSERT( boundaryNodes.size() == hasFluidNeighbor.size() );

    Dot3D absoluteOffset = lattice.getLocation();

    Array<T,3>& localForce = info->getLocalForce();
    localForce.resetToZero();
    for (pluint i=0; i<boundaryNodes.size(); ++i) {
        cellCompletion (
            lattice, boundaryNodes[i], solidDirections[i],
            boundaryIds[i], hasFluidNeighbor[i], absoluteOffset, localForce, args );
    }
}


template<typename T, template<typename U> class Descriptor>
void BouzidiOffLatticeModel3D<T,Descriptor>::cellCompletion (
        BlockLattice3D<T,Descriptor>& lattice,
        Dot3D const& boundaryNode,
        std::vector<int> const& solidDirections, std::vector<plint> const& boundaryIds,
        std::vector<bool> const& hasFluidNeighbor, Dot3D const& absoluteOffset,
        Array<T,3>& localForce, std::vector<AtomicBlock3D const*> const& args )
{
    typedef Descriptor<T> D;
    Array<T,D::d> deltaJ;
    deltaJ.resetToZero();

    plint numNeumannNodes=0;
    T neumannDensity = T();
    Cell<T,Descriptor>& cell = lattice.get(boundaryNode.x,boundaryNode.y,boundaryNode.z);
    for(pluint i=0; i<solidDirections.size(); ++i) {
        int iPop = solidDirections[i];
        int oppPop = indexTemplates::opposite<D>(i);
        Array<T,3> wallNode, wall_vel;
        T AC;
        OffBoundary::Type bdType;
        Array<T,3> wallNormal;
        plint id = boundaryIds[i];
#ifdef PLB_DEBUG
        bool ok =
#endif
        this->pointOnSurface (
                boundaryNode+absoluteOffset, Dot3D(D::c[iPop][0],D::c[iPop][1],D::c[iPop][2]),
                wallNode, AC, wallNormal, wall_vel, bdType, id );
        PLB_ASSERT( ok );
        T q = AC * invAB[iPop];
        Cell<T,Descriptor>& iCell = lattice.get(boundaryNode.x+D::c[iPop][0],boundaryNode.y+D::c[iPop][1],boundaryNode.z+D::c[iPop][2]);
        Cell<T,Descriptor>& jCell = lattice.get(boundaryNode.x-D::c[iPop][0],boundaryNode.y-D::c[iPop][1],boundaryNode.z-D::c[iPop][2]);
        if (bdType==OffBoundary::dirichlet) {
            T u_ci = D::c[iPop][0]*wall_vel[0]+D::c[iPop][1]*wall_vel[1]+D::c[iPop][2]*wall_vel[2];
            if (hasFluidNeighbor[i]) {
                if (q<(T)0.5) {
                    cell[oppPop] = 2.*q*iCell[iPop] + (1.-2.*q)*cell[iPop];
                    cell[oppPop] += 2.* u_ci*D::t[iPop]*D::invCs2;
                }
                else {
                    cell[oppPop] = 1./(2.*q)*iCell[iPop]+(2.*q-1)/(2.*q)*jCell[oppPop];
                    cell[oppPop] += 1./q* u_ci*D::t[iPop]*D::invCs2;
                }
            }
            else {
                cell[oppPop] = iCell[iPop]+2.* u_ci*D::t[iPop]*D::invCs2;
            }
        }
        else if (bdType==OffBoundary::densityNeumann) {
            ++numNeumannNodes;
            neumannDensity += wall_vel[0];
            if (hasFluidNeighbor[i]) {
                cell[oppPop] = jCell[oppPop];
            }
            else {
                cell[oppPop] = cell[iPop];
            }
        }
        else {
            // Not implemented yet.
            PLB_ASSERT( false );
        }
        if (computeStat) {
            deltaJ[0] = D::c[iPop][0]*iCell[iPop] - D::c[oppPop][0]*cell[oppPop];
            deltaJ[1] = D::c[iPop][1]*iCell[iPop] - D::c[oppPop][1]*cell[oppPop];
            deltaJ[2] = D::c[iPop][2]*iCell[iPop] - D::c[oppPop][2]*cell[oppPop];
        }
    }
    localForce += deltaJ;
    if (numNeumannNodes>0) {
        neumannDensity /= numNeumannNodes;
        T oldRhoBar;
        Array<T,3> j;
        momentTemplates<T,Descriptor>::get_rhoBar_j(cell, oldRhoBar, j);
        T newRhoBar = D::rhoBar(neumannDensity);
        T jSqr = normSqr(j);
        for (plint iPop=0; iPop<D::q; ++iPop) {
            T oldEq = cell.getDynamics().computeEquilibrium(iPop, oldRhoBar, j, jSqr);
            T newEq = cell.getDynamics().computeEquilibrium(iPop, newRhoBar, j, jSqr);
            cell[iPop] += newEq - oldEq;
        }
    }
}

}  // namespace plb

#endif  // BOUZIDI_OFF_LATTICE_MODEL_3D_HH

