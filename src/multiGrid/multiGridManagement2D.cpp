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

/* Main author: Daniel Lagrava
 **/

/** \file
 * Management of 2D multi grid domains -- Implementation
 */

#include "multiGrid/multiGridManagement2D.h"
#include "io/parallelIO.h"
#include <memory>

namespace plb {

MultiGridManagement2D::MultiGridManagement2D(plint nx, plint ny, 
                                             plint numLevels, plint overlapWidth_, plint referenceLevel_)
                                  : overlapWidth(overlapWidth_),
                                    referenceLevel(referenceLevel_),
                                    boundingBoxes(numLevels),
                                    coarseGridInterfaces(numLevels),
                                    fineGridInterfaces(numLevels),
                                    bulks(numLevels),
                                    coarseInterfaceOrientations(numLevels),
                                    fineInterfaceOrientations(numLevels)
{
    PLB_ASSERT( numLevels > 0 );
    PLB_ASSERT( referenceLevel < numLevels );
    initialize(Box2D(0, nx-1, 0, ny-1));
}

MultiGridManagement2D::MultiGridManagement2D(Box2D coarseBoundingBox, 
                                             plint numLevels, plint overlapWidth_, plint referenceLevel_)
                                  : overlapWidth(overlapWidth_),
                                    referenceLevel(referenceLevel_),
                                    boundingBoxes(numLevels),
                                    coarseGridInterfaces(numLevels),
                                    fineGridInterfaces(numLevels),
                                    bulks(numLevels),
                                    coarseInterfaceOrientations(numLevels),
                                    fineInterfaceOrientations(numLevels)
{
    PLB_ASSERT( numLevels > 0 );
    PLB_ASSERT( referenceLevel < numLevels );
    initialize(coarseBoundingBox);
}

MultiGridManagement2D::~MultiGridManagement2D() {
    delete scaleManager;
}

MultiGridManagement2D::MultiGridManagement2D(MultiGridManagement2D const& rhs)
       :overlapWidth(rhs.overlapWidth),
        referenceLevel(rhs.referenceLevel),
        boundingBoxes(rhs.boundingBoxes),
        coarseGridInterfaces(rhs.coarseGridInterfaces),
        fineGridInterfaces(rhs.fineGridInterfaces),
        bulks(rhs.bulks),
        coarseInterfaceOrientations(rhs.coarseInterfaceOrientations),
        fineInterfaceOrientations(rhs.fineInterfaceOrientations),
        mpiProcess(rhs.mpiProcess),
        scaleManager(rhs.scaleManager->clone())
{}

void MultiGridManagement2D::initialize(Box2D const& level0_box)
{
    scaleManager = global::getDefaultMultiScaleManager().clone();

    // The parameters nx and ny refer to the coarsest lattice (lattice 0).
    boundingBoxes[0] = level0_box;
    // Multiply the values maxX and maxY by two to get, iteratively,
    //   the boundingBoxes of the next finer lattice respectively.
    for (pluint iDim=1; iDim<boundingBoxes.size(); ++iDim) {
        boundingBoxes[iDim] = scaleManager->scaleBox(boundingBoxes[iDim-1], 1);
    }
    // Add a block for the full domain of the first instantiated multi-block.
    bulks[referenceLevel].push_back(boundingBoxes[referenceLevel]);
}


void MultiGridManagement2D::swap(MultiGridManagement2D& rhs)
{
    std::swap(overlapWidth, rhs.overlapWidth);
    std::swap(referenceLevel, rhs.referenceLevel);
    boundingBoxes.swap(rhs.boundingBoxes);
    mpiProcess.swap(rhs.mpiProcess);
    coarseGridInterfaces.swap(rhs.coarseGridInterfaces);
    fineGridInterfaces.swap(rhs.fineGridInterfaces);
    bulks.swap(rhs.bulks);
    coarseInterfaceOrientations.swap(rhs.coarseInterfaceOrientations);
    fineInterfaceOrientations.swap(rhs.fineInterfaceOrientations);
    std::swap(scaleManager, rhs.scaleManager);
}

Box2D MultiGridManagement2D::getBoundingBox(plint level) const {
    PLB_PRECONDITION(level>=0 && level<(plint)bulks.size());
    return boundingBoxes[level];
}

std::vector<Box2D> const& MultiGridManagement2D::getBulks(plint iLevel) const{
    PLB_PRECONDITION( iLevel>=0 && iLevel<(plint)bulks.size() );
    return bulks[iLevel];
}

std::vector<std::vector<Box2D> > const& MultiGridManagement2D::getBulks() const{
  return bulks;
}


/**
 * Use one of the Parallelizer2D objects to recompute the parallelization of the
 *   domain.
 */
void MultiGridManagement2D::parallelize(Parallelizer2D* parallelizer){
    parallelizer->parallelize();
    bulks = parallelizer->getRecomputedBlocks();
    mpiProcess = parallelizer->getMpiDistribution();
    delete parallelizer;
}

/**
 * Insert a refinement at coarseLevel over coarseDomain. It is necessary that the coordinates
 * of coarseDomain are given with respect to coarseLevel. 
 */
void MultiGridManagement2D::refine(plint coarseLevel, Box2D coarseDomain) {
    
    // The finest multi-block, at level bulks.size()-1, cannot be further refined.
    PLB_PRECONDITION( coarseLevel>=0 && coarseLevel< (plint)bulks.size()-1 );
    plint fineLevel = coarseLevel+1;
    
    // First, trim the domain coarseDomain in case it exceeds the extent of the
    //   multi-block, and determine whether coarseDomain touches one of the boundaries
    //   of the multi-block. This information is needed, because the coarse domain
    //   fully replaces the fine domain on boundaries of the multi-block, and there
    //   is therefore no need to create a coarse-fine coupling.
    bool touchLeft=false, touchRight=false, touchBottom=false, touchTop=false;
    trimDomain(coarseLevel, coarseDomain, touchLeft, touchRight, touchBottom, touchTop);

    // The reduced coarse domain is the one which is going to be excluded from
    //   the original coarse lattice.
    Box2D reducedCoarseDomain(coarseDomain.enlarge(-1));
    // The extended coarse domain it the one which is going to be added
    //   to the original fine lattice.
    Box2D extendedCoarseDomain(coarseDomain.enlarge(overlapWidth));

    // Same as coarseDomain, but in units of the fine lattice.
    Box2D fineDomain(coarseDomain.multiply(2));
    
    // If the domain in question touches a boundary of the multi-block,
    //   both the reduced and the extended coarse domain are
    //   identified with the boundary location.
    if (touchLeft) {
        reducedCoarseDomain.x0    -= 1;
        extendedCoarseDomain.x0   += overlapWidth;
    }
    if (touchRight) {
        reducedCoarseDomain.x1    += 1;
        extendedCoarseDomain.x1   -= overlapWidth;
    }
    if (touchBottom) {
        reducedCoarseDomain.y0    -= 1;
        extendedCoarseDomain.y0   += overlapWidth;
    }
    if (touchTop) {
        reducedCoarseDomain.y1    += 1;
        extendedCoarseDomain.y1   -= overlapWidth;
    }

    
    // Extract reduced coarse domain from the original coarse multi-block, for the bulks.
    std::vector<Box2D> exceptedBulks;
    for (pluint iBlock=0; iBlock<bulks[coarseLevel].size(); ++iBlock) {
        except(bulks[coarseLevel][iBlock], reducedCoarseDomain, exceptedBulks);
    }
    exceptedBulks.swap(bulks[coarseLevel]);
    
    // Convert the extended coarse domain to fine units,
    //   and add to the original fine multi-block.
    Box2D extendedFineDomain(extendedCoarseDomain.multiply(2));
    bulks[fineLevel].push_back(extendedFineDomain);

    // Define coupling interfaces for all four sides of the coarsened domain, unless they
    //   touch a boundary of the multi-block.
    if (!touchLeft) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( coarseDomain.x0, coarseDomain.x0,
                                                            coarseDomain.y0, coarseDomain.y1 ) );
        // it is a right border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(0,1));
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x0, 
                                                        extendedCoarseDomain.y0, extendedCoarseDomain.y1 ) );
        // it is a left border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(0,-1));
    }
    if (!touchRight) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( coarseDomain.x1, coarseDomain.x1,
                                                            coarseDomain.y0, coarseDomain.y1 ) );
        // it is a left border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(0,-1));
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x1, extendedCoarseDomain.x1, 
                                                        extendedCoarseDomain.y0, extendedCoarseDomain.y1 ) );
        // it is a right border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(0,1));
    }
    if (!touchBottom) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( coarseDomain.x0, coarseDomain.x1,
                                                            coarseDomain.y0, coarseDomain.y0 ) );
        
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x1, 
                                                        extendedCoarseDomain.y0, extendedCoarseDomain.y0 ) );
        // it is an upper border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(1,1));
        // it is a lower border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(1,-1));
    }
    if (!touchTop) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( coarseDomain.x0, coarseDomain.x1,
                                                            coarseDomain.y1, coarseDomain.y1 ) );
        // it is a lower border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(1,-1));
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x1, 
                                                        extendedCoarseDomain.y1, extendedCoarseDomain.y1 ) );
        // it is an upper border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(1,1));
    }
}

/**
 * Insert a coarse patch at fineLevel over coarseDomain. The coordinates of coarseDomain
 *   must be in fine coordinates.
 */

void MultiGridManagement2D::coarsen(plint fineLevel, Box2D coarseDomain){
    // The coarsest multi-block, at level 0, cannot be further coarsened.
    PLB_PRECONDITION( fineLevel>=1 && fineLevel<(plint)bulks.size() );
    plint coarseLevel = fineLevel-1;

    // First, trim the domain coarseDomain in case it exceeds the extent of the
    //   multi-block, and determine whether coarseDomain touches one of the boundaries
    //   of the multi-block. This information is needed, because the coarse domain
    //   fully replaces the fine domain on boundaries of the multi-block, and there
    //   is therefore no need to create a coarse-fine coupling.
    bool touchLeft=false, touchRight=false, touchBottom=false, touchTop=false;
    trimDomain(coarseLevel, coarseDomain, touchLeft, touchRight, touchBottom, touchTop);

    // Convert the coarse domain to fine units.
    Box2D fineDomain(coarseDomain.multiply(2));
    // The reduced fine domain is the one which is going to be excluded from
    //   the original fine lattice.
    Box2D intermediateFineDomain(fineDomain.enlarge(-1));
    // The extended coarse domain it the one which is going to be added
    //   to the original coarse lattice.
    Box2D extendedCoarseDomain(coarseDomain.enlarge(1));
    
    // If the domain in question touches a boundary of the multi-block,
    //   both the reduced fine domain and the extended coarse domain are
    //   identified with the boundary location.
    if (touchLeft) {
        intermediateFineDomain.x0 -= 1;
        extendedCoarseDomain.x0   += 1;
    }
    if (touchRight) {
        intermediateFineDomain.x1 += 1;
        extendedCoarseDomain.x1   -= 1;
    }
    if (touchBottom) {
        intermediateFineDomain.y0 -= 1;
        extendedCoarseDomain.y0   += 1;
    }
    if (touchTop) {
        intermediateFineDomain.y1 += 1;
        extendedCoarseDomain.y1   -= 1;
    }

    // Extract reduced fine domain from the original fine multi-block.
    std::vector<Box2D> exceptedBlocks;
    for (pluint iBlock=0; iBlock<bulks[fineLevel].size(); ++iBlock) {
        except(bulks[fineLevel][iBlock], intermediateFineDomain, exceptedBlocks);
    }
    exceptedBlocks.swap(bulks[fineLevel]);

    // Add extended coarse domain to the original coarse multi-block.
    bulks[coarseLevel].push_back(extendedCoarseDomain);
    
    // Define coupling interfaces for all four sides of the refined domain, unless they
    //   touch a boundary of the multi-block.
    if (!touchLeft) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x0,
                                                            extendedCoarseDomain.y0, extendedCoarseDomain.y1 ) );
        fineGridInterfaces[fineLevel].push_back( Box2D( coarseDomain.x0, coarseDomain.x0, 
                                                        coarseDomain.y0, coarseDomain.y1 ) );
                                                        
        // it is a left border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(0,-1));
        // it is an right border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(0,1));
    }
    if (!touchRight) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( extendedCoarseDomain.x1, extendedCoarseDomain.x1,
                                                            extendedCoarseDomain.y0, extendedCoarseDomain.y1 ) );
        fineGridInterfaces[fineLevel].push_back( Box2D( coarseDomain.x1, coarseDomain.x1, 
                                                        coarseDomain.y0, coarseDomain.y1 ) );
                                                        
        // it is a right border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(0,1));
        // it is a left border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(0,-1));
    }
    if (!touchBottom) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x1,
                                                            extendedCoarseDomain.y0, extendedCoarseDomain.y0 ) );
        fineGridInterfaces[fineLevel].push_back( Box2D( coarseDomain.x0, coarseDomain.x1, 
                                                        coarseDomain.y0, coarseDomain.y0 ) );
                                                        
        // it is a bottom border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(1,-1)); //TODO check this!!!
        // it is a top border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(1,1));        
    }
    if (!touchTop) {
        coarseGridInterfaces[coarseLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x1,
                                                            extendedCoarseDomain.y1, extendedCoarseDomain.y1 ) );
        fineGridInterfaces[fineLevel].push_back( Box2D( coarseDomain.x0, coarseDomain.x1, 
                                                        coarseDomain.y1, coarseDomain.y1 ) );
                                                        
        // it is a top border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(1,1));
        // it is an bottom border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(1,-1));
    }
}


void MultiGridManagement2D::refineMultiGrid(plint coarseLevel, Box2D coarseDomain){
        
    // The finest multi-block, at level bulks.size()-1, cannot be further refined.
    PLB_PRECONDITION( coarseLevel>=0 && coarseLevel< (plint)bulks.size()-1 );
    plint fineLevel = coarseLevel+1;
    
    // First, trim the domain coarseDomain in case it exceeds the extent of the
    //   multi-block, and determine whether coarseDomain touches one of the boundaries
    //   of the multi-block. This information is needed, because the coarse domain
    //   fully replaces the fine domain on boundaries of the multi-block, and there
    //   is therefore no need to create a coarse-fine coupling.
    bool touchLeft=false, touchRight=false, touchBottom=false, touchTop=false;
    trimDomain(coarseLevel, coarseDomain, touchLeft, touchRight, touchBottom, touchTop);

    // The reduced coarse domain is the one which is going to be excluded from
    //   the original coarse lattice.
    Box2D reducedCoarseDomain(coarseDomain.enlarge(-1));
    // The extended coarse domain it the one which is going to be added
    //   to the original fine lattice.
    Box2D extendedCoarseDomain(coarseDomain.enlarge(overlapWidth));

    // Same as coarseDomain, but in units of the fine lattice.
    Box2D fineDomain(coarseDomain.multiply(2));
    
    // If the domain in question touches a boundary of the multi-block,
    //   both the reduced and the extended coarse domain are
    //   identified with the boundary location.
    if (touchLeft) {
        reducedCoarseDomain.x0    -= 1;
        extendedCoarseDomain.x0   += overlapWidth;
    }
    if (touchRight) {
        reducedCoarseDomain.x1    += 1;
        extendedCoarseDomain.x1   -= overlapWidth;
    }
    if (touchBottom) {
        reducedCoarseDomain.y0    -= 1;
        extendedCoarseDomain.y0   += overlapWidth;
    }
    if (touchTop) {
        reducedCoarseDomain.y1    += 1;
        extendedCoarseDomain.y1   -= overlapWidth;
    }

    
    // Do not do anything to the coarse domain
    
    
    // Convert the extended coarse domain to fine units,
    //   and add to the original fine multi-block.
    Box2D extendedFineDomain(extendedCoarseDomain.multiply(2));
    bulks[fineLevel].push_back(extendedFineDomain);

    // Define coupling interfaces for all four sides of the coarsened domain, unless they
    //   touch a boundary of the multi-block.
    if (!touchLeft) {

        // it is a right border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(0,1));
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x0, 
                                                        extendedCoarseDomain.y0, extendedCoarseDomain.y1 ) );
        // it is a left border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(0,-1));
    }
    if (!touchRight) {

        // it is a left border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(0,-1));
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x1, extendedCoarseDomain.x1, 
                                                        extendedCoarseDomain.y0, extendedCoarseDomain.y1 ) );
        // it is a right border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(0,1));
    }
    if (!touchBottom) {
        
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x1, 
                                                        extendedCoarseDomain.y0, extendedCoarseDomain.y0 ) );
        // it is an upper border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(1,1));
        // it is a lower border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(1,-1));
    }
    if (!touchTop) {
        
        // it is a lower border in the coarse case
        coarseInterfaceOrientations[coarseLevel].push_back(Array<plint,2>(1,-1));
        fineGridInterfaces[fineLevel].push_back( Box2D( extendedCoarseDomain.x0, extendedCoarseDomain.x1, 
                                                        extendedCoarseDomain.y1, extendedCoarseDomain.y1 ) );
        // it is an upper border in the fine case
        fineInterfaceOrientations[fineLevel].push_back(Array<plint,2>(1,1));
    }
    
    coarseGridInterfaces[coarseLevel].push_back(coarseDomain);
}


std::vector<std::vector<Box2D> > const& MultiGridManagement2D::getCoarseInterface() const
{
    return coarseGridInterfaces;
}

std::vector<std::vector<Box2D> > const& MultiGridManagement2D::getFineInterface() const
{
    return fineGridInterfaces;
}

std::vector<std::vector<Array<plint,2> > > const& MultiGridManagement2D::getCoarseOrientation() const
{
    return coarseInterfaceOrientations;
}

std::vector<std::vector<Array<plint,2> > > const& MultiGridManagement2D::getFineOrientation() const
{
    return fineInterfaceOrientations;
}



void MultiGridManagement2D::trimDomain (plint whichLevel, Box2D& domain,
                                        bool& touchLeft, bool& touchRight, 
                                        bool& touchBottom, bool& touchTop ) const
{
    Box2D bbox = boundingBoxes[whichLevel];
    if (domain.x0 <= bbox.x0) {
        domain.x0 = bbox.x0; // Trim in case multi-block extent is exceeded.
        touchLeft = true;
    }
    if (domain.y0 <= bbox.y0) {
        domain.y0 = bbox.y0; // Trim in case multi-block extent is exceeded.
        touchBottom = true;
    }
    if (domain.x1 >= bbox.x1) {
        // Trim in case multi-block extent is exceeded.
        domain.x1 = bbox.x1; 
        touchRight = true;
    }
    if (domain.y1 >= bbox.y1) {
        // Trim in case multi-block extent is exceeded.
        domain.y1 = bbox.y1; 
        touchTop = true;
    }
}

plint MultiGridManagement2D::getNumLevels() const {
    return (plint) bulks.size();
}

plint MultiGridManagement2D::getReferenceLevel() const {
    return referenceLevel;
}

plint MultiGridManagement2D::getOverlapWidth() const {
    return overlapWidth;
}

/** The main idea is to check if there exists a coarse part to communicate with the 
 *    fine interfaces that we have defined. Clearly, when we put two fine blocks one
 *    after the other it is not the case
*/
void MultiGridManagement2D::eliminateUnnecessaryFineInterfaces(){
    for (pluint iLevel=1; iLevel<bulks.size(); ++iLevel){
        std::vector<Box2D> newInterfaces;
        for (pluint iBlock=0; iBlock<bulks[iLevel-1].size(); ++iBlock){
            for (pluint iInterface=0; iInterface<fineGridInterfaces[iLevel].size(); ++iInterface){
                Box2D intersection;
                bool ok = intersect(bulks[iLevel-1][iBlock],fineGridInterfaces[iLevel][iInterface],intersection);
                if ((! (intersection == fineGridInterfaces[iLevel][iInterface] ) && ok )){
                    pcout << "there is no complete intersection between :\n";
                    Box2D box = fineGridInterfaces[iLevel][iInterface];
                    pcout << "interface : " << box.x0 << " " << box.x1 << " " << box.y0 << " " << box.y1 << std::endl;
                    box = bulks[iLevel-1][iBlock];
                    pcout << "bulk : " << box.x0 << " " << box.x1 << " " << box.y0 << " " << box.y1 << std::endl;
                }
            }
        }
    }

}

std::vector<std::vector<plint> > const& MultiGridManagement2D::getMpiProcesses() const{
    return mpiProcess;
}

std::vector<Box2D> const& MultiGridManagement2D::getBoundingBoxes() const
{
    return boundingBoxes;
}

/// Extract a domain (in coarse coordinates) from a MultiGridManagement2D
/** This is achieved by taking all the geometric information of the MultiGridManagement2D
 *  and intersecting it with the coarsestDomain. All the intersecting parts
 *  are kept on the new MultiGridManagement2D.
 */
MultiGridManagement2D extractManagement(MultiGridManagement2D management, Box2D coarsestDomain, bool crop){
    std::auto_ptr<MultiGridManagement2D> result;
    
    if (crop){
        result = std::auto_ptr<MultiGridManagement2D>(new MultiGridManagement2D(coarsestDomain,
                                                                            management.getNumLevels(),
                                                                            management.getOverlapWidth(),
                                                                            management.getReferenceLevel()));
    }
    else {
        result = std::auto_ptr<MultiGridManagement2D>(new MultiGridManagement2D(management.getBoundingBox(0), 
                                                                            management.getNumLevels(),
                                                                            management.getOverlapWidth(),
                                                                            management.getReferenceLevel()));
    }
    
    // erase the bulk added by default by the constructor
    result->bulks[0].erase(result->bulks[0].begin());
    
    MultiScaleManager* scaleManager = result->scaleManager->clone();
    
    // every level must contain the informations intersected with coarsestDomain
    for (plint iLevel=0; iLevel<management.getNumLevels(); ++iLevel){
        Box2D rescaledBox = scaleManager->scaleBox(coarsestDomain,iLevel);

        // boundingBoxes
        result->boundingBoxes[iLevel] = rescaledBox;
        
        if (iLevel< management.getNumLevels()-1){
            // coarseGridInterfaces
            for (pluint iInterface=0; iInterface<management.coarseGridInterfaces.size(); ++iInterface){
                Box2D intersection;
                if (intersect(rescaledBox,management.coarseGridInterfaces[iLevel][iInterface],intersection)){
                    result->coarseGridInterfaces[iLevel].push_back(intersection);
                    // copy also the orientation and direction for coarse grids
                    result->coarseInterfaceOrientations[iLevel].push_back(
                        Array<plint,2>(management.coarseInterfaceOrientations[iLevel][iInterface]));
                }
            }
        }
        
        if (iLevel > 0) {
            // fineGridInterfaces
            for (pluint iInterface=0; iInterface<management.coarseGridInterfaces.size(); ++iInterface){
                Box2D intersection;
                if (intersect(rescaledBox,management.fineGridInterfaces[iLevel][iInterface],intersection)){
                    result->fineGridInterfaces[iLevel].push_back(intersection);
                    // copy also the orientation and direction for fine grids
                    result->fineInterfaceOrientations[iLevel].push_back(
                        Array<plint,2>(management.fineInterfaceOrientations[iLevel][iInterface]));
                }
            }
        }
        
        // bulk
        for (pluint iBox=0; iBox<management.bulks[iLevel].size(); ++iBox){
            Box2D intersection;
            // verify if the bulk is contained
            if (intersect(rescaledBox,management.bulks[iLevel][iBox],intersection)){
                result->bulks[iLevel].push_back(intersection);
                // copy also the mpi process associated
                result->mpiProcess[iLevel].push_back(management.mpiProcess[iLevel][iBox]);
            }
        }
    }
    delete scaleManager;
    return *result;

}


} // namespace plb

