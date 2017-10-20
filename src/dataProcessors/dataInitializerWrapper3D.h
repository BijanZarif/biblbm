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

/** \file
 * Helper functions for domain initialization -- header file.
 */
#ifndef DATA_INITIALIZER_WRAPPER_3D_H
#define DATA_INITIALIZER_WRAPPER_3D_H

#include "core/globalDefs.h"
#include "atomicBlock/dataProcessorWrapper3D.h"
#include "multiBlock/multiDataProcessorWrapper3D.h"
#include "core/dynamics.h"
#include "dataProcessors/dataInitializerFunctional3D.h"

namespace plb {

/* *************** PART I ******************************************** */
/* *************** Initialization of the block-lattice: atomic-blocks  */
/* ******************************************************************* */

template<typename T, template<class U> class Descriptor>
void apply(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, OneCellFunctional3D<T,Descriptor>* f);

template<typename T, template<class U> class Descriptor>
void applyIndexed(BlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                  OneCellIndexedFunctional3D<T,Descriptor>* f);

template<typename T, template<class U> class Descriptor>
void defineDynamics(BlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                    Dynamics<T,Descriptor>* dynamics);

template<typename T, template<class U> class Descriptor>
void defineDynamics(BlockLattice3D<T,Descriptor>& lattice, Box3D boundingBox,
                    DomainFunctional3D* domain, Dynamics<T,Descriptor>* dynamics);

template<typename T, template<class U> class Descriptor>
void defineDynamics(BlockLattice3D<T,Descriptor>& lattice, plint iX, plint iY, plint iZ,
                    Dynamics<T,Descriptor>* dynamics);

template<typename T, template<class U> class Descriptor>
void defineDynamics(BlockLattice3D<T,Descriptor>& lattice, DotList3D const& dotList,
                    Dynamics<T,Descriptor>* dynamics);

template<typename T, template<typename U> class Descriptor>
void defineDynamics( BlockLattice3D<T,Descriptor>& lattice, ScalarField3D<bool>& boolMask,
                     Box3D domain, Dynamics<T,Descriptor>* dynamics, bool whichFlag );

template<typename T, template<typename U> class Descriptor>
void defineDynamics( BlockLattice3D<T,Descriptor>& lattice, ScalarField3D<bool>& boolMask,
                     Dynamics<T,Descriptor>* dynamics, bool whichFlag );

template<typename T, template<typename U> class Descriptor>
void defineDynamics( BlockLattice3D<T,Descriptor>& lattice, ScalarField3D<int>& intMask,
                     Box3D domain, Dynamics<T,Descriptor>* dynamics, int whichFlag );

template<typename T, template<typename U> class Descriptor>
void defineDynamics( BlockLattice3D<T,Descriptor>& lattice, ScalarField3D<int>& intMask,
                     Dynamics<T,Descriptor>* dynamics, int whichFlag );
                     
template<typename T, template<typename U> class Descriptor>
void recomposeFromFlowVariables ( BlockLattice3D<T,Descriptor>& lattice,
                                  ScalarField3D<T>& density, TensorField3D<T,3>& velocity,
                                  TensorField3D<T,6>& strainRate, Box3D domain );

template<typename T, template<typename U> class Descriptor>
void recomposeFromFlowVariables ( BlockLattice3D<T,Descriptor>& lattice,
                                  ScalarField3D<T>& density, TensorField3D<T,3>& velocity,
                                  TensorField3D<T,6>& strainRate );

template<typename T, template<class U> class Descriptor>
void setOmega(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, T omega);

template<typename T, template<class U> class Descriptor>
void setOmega(BlockLattice3D<T,Descriptor>& lattice, ScalarField3D<T> &omega, Box3D domain);

template<typename T, template<class U> class Descriptor>
void setBoundaryVelocity(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, Array<T,3> velocity);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class VelocityFunction>
void setBoundaryVelocity(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, VelocityFunction f) {
    applyIndexed (
            lattice, domain,
            new SetCustomBoundaryVelocityFunctional3D<T,Descriptor,VelocityFunction> (f) );
}

template<typename T, template<class U> class Descriptor>
void setBoundaryDensity(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, T rho);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class DensityFunction>
void setBoundaryDensity(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, DensityFunction f) {
    applyIndexed (
            lattice, domain,
            new SetCustomBoundaryDensityFunctional3D<T,Descriptor,DensityFunction> (f) );
}


template<typename T, template<class U> class Descriptor>
void initializeAtEquilibrium(BlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                             T density, Array<T,3> velocity);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class RhoUFunction>
void initializeAtEquilibrium(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, RhoUFunction f) {
    applyIndexed (
            lattice, domain,
            new IniCustomEquilibriumFunctional3D<T,Descriptor,RhoUFunction> (f) );
}

template<typename T, template<class U> class Descriptor, class Function>
void initializeAtEquilibrium(BlockLattice3D<T,Descriptor>& lattice,
                             Box3D boundingBox, DomainFunctional3D* domain, Function f);

template<typename T, template<class U> class Descriptor>
void stripeOffDensityOffset(BlockLattice3D<T,Descriptor>& lattice, Box3D domain, T deltaRho);


template<typename T, template<class U> class Descriptor>
void setCompositeDynamics( BlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                           CompositeDynamics<T,Descriptor>* compositeDynamics );

template<typename T, template<class U> class Descriptor>
void setExternalScalar( BlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                        int whichScalar, T externalScalar );

template<typename T, template<class U> class Descriptor>
void setExternalVector( BlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                        int vectorStartsAt, Array<T,Descriptor<T>::d> externalVector );


/* *************** PART II ******************************************* */
/* *************** Initialization of the block-lattice: multi-block ** */
/* ******************************************************************* */

template<typename T, template<class U> class Descriptor>
void apply(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, OneCellFunctional3D<T,Descriptor>* f);

template<typename T, template<class U> class Descriptor>
void applyIndexed(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                  OneCellIndexedFunctional3D<T,Descriptor>* f);

template<typename T, template<class U> class Descriptor>
void defineDynamics(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                    Dynamics<T,Descriptor>* dynamics);

template<typename T, template<class U> class Descriptor>
void defineDynamics(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D boundingBox,
                    DomainFunctional3D* domain, Dynamics<T,Descriptor>* dynamics);

template<typename T, template<class U> class Descriptor>
void defineDynamics(MultiBlockLattice3D<T,Descriptor>& lattice, plint iX, plint iY, plint iZ,
                    Dynamics<T,Descriptor>* dynamics);

template<typename T, template<class U> class Descriptor>
void defineDynamics(MultiBlockLattice3D<T,Descriptor>& lattice, DotList3D const& dotList,
                    Dynamics<T,Descriptor>* dynamics);

template<typename T, template<typename U> class Descriptor>
void defineDynamics( MultiBlockLattice3D<T,Descriptor>& lattice, MultiScalarField3D<bool>& boolMask,
                     Box3D domain, Dynamics<T,Descriptor>* dynamics, bool whichFlag );

template<typename T, template<typename U> class Descriptor>
void defineDynamics( MultiBlockLattice3D<T,Descriptor>& lattice, MultiScalarField3D<bool>& boolMask,
                     Dynamics<T,Descriptor>* dynamics, bool whichFlag );

template<typename T, template<typename U> class Descriptor>
void defineDynamics( MultiBlockLattice3D<T,Descriptor>& lattice, MultiScalarField3D<int>& intMask,
                     Box3D domain, Dynamics<T,Descriptor>* dynamics, int whichFlag );

template<typename T, template<typename U> class Descriptor>
void defineDynamics( MultiBlockLattice3D<T,Descriptor>& lattice, MultiScalarField3D<int>& intMask,
                     Dynamics<T,Descriptor>* dynamics, int whichFlag );
                     
template<typename T, template<typename U> class Descriptor>
void recomposeFromFlowVariables ( MultiBlockLattice3D<T,Descriptor>& lattice,
                                  MultiScalarField3D<T>& density, TensorField3D<T,3>& velocity,
                                  MultiTensorField3D<T,6>& strainRate, Box3D domain );

template<typename T, template<typename U> class Descriptor>
void recomposeFromFlowVariables ( MultiBlockLattice3D<T,Descriptor>& lattice,
                                  MultiScalarField3D<T>& density, TensorField3D<T,3>& velocity,
                                  MultiTensorField3D<T,6>& strainRate );

template<typename T, template<class U> class Descriptor>
void setOmega(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, T omega);

template<typename T, template<class U> class Descriptor>
void setOmega(MultiBlockLattice3D<T,Descriptor>& lattice, MultiScalarField3D<T>& omega, Box3D domain);

template<typename T, template<class U> class Descriptor>
void setBoundaryVelocity(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, Array<T,3> velocity);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class VelocityFunction>
void setBoundaryVelocity(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, VelocityFunction f) {
    applyIndexed (
            lattice, domain,
            new SetCustomBoundaryVelocityFunctional3D<T,Descriptor,VelocityFunction> (f) );
}

template<typename T, template<class U> class Descriptor>
void setBoundaryDensity(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, T rho);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class DensityFunction>
void setBoundaryDensity(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, DensityFunction f) {
    applyIndexed (
            lattice, domain,
            new SetCustomBoundaryDensityFunctional3D<T,Descriptor,DensityFunction> (f) );
}


template<typename T, template<class U> class Descriptor>
void initializeAtEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                             T density, Array<T,3> velocity);

template<typename T, template<class U> class Descriptor, class DomainFunctional>
void maskedInitializeAtEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice,
                                   Box3D boundingBox, DomainFunctional const& domain,
                                   T density, Array<T,3> velocity);

template<typename T, template<class U> class Descriptor>
void initializeAtEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice,
                             Box3D boundingBox, DomainFunctional3D* domain,
                             T density, Array<T,3> velocity);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class RhoUFunction>
void initializeAtEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, RhoUFunction f) {
    applyIndexed (
            lattice, domain,
            new IniCustomEquilibriumFunctional3D<T,Descriptor,RhoUFunction> (f) );
}

template<typename T, template<class U> class Descriptor, class Function>
void initializeAtEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice,
                             Box3D boundingBox, DomainFunctional3D* domain, Function f);


template<typename T, template<class U> class Descriptor>
void initializeAtThermalEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                                    T density, Array<T,3> velocity, T temperature);

template<typename T, template<class U> class Descriptor>
void initializeAtThermalEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice,
                                    Box3D boundingBox, DomainFunctional3D* domain,
                                    T density, Array<T,3> velocity, T temperature);

// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, template<class U> class Descriptor, class RhoVelTempFunction>
void initializeAtThermalEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, RhoVelTempFunction f) {
    applyIndexed (
            lattice, domain,
            new IniCustomThermalEquilibriumFunctional3D<T,Descriptor,RhoVelTempFunction> (f) );
}

template<typename T, template<class U> class Descriptor, class Function>
void initializeAtThermalEquilibrium(MultiBlockLattice3D<T,Descriptor>& lattice,
                                    Box3D boundingBox, DomainFunctional3D* domain, Function f);

template<typename T, template<class U> class Descriptor>
void stripeOffDensityOffset(MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain, T deltaRho);


template<typename T, template<class U> class Descriptor>
void setCompositeDynamics( MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                           CompositeDynamics<T,Descriptor>* compositeDynamics );

template<typename T, template<class U> class Descriptor>
void setExternalScalar( MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                        int whichScalar, T externalScalar );

template<typename T, template<class U> class Descriptor, class Functional>
void setGenericExternalScalar( MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                               int whichScalar, Functional const& functional );

template<typename T, template<class U> class Descriptor>
void setExternalVector( MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                        int vectorStartsAt, Array<T,Descriptor<T>::d> externalVector );

template<typename T, template<class U> class Descriptor, int nDim>
void setExternalVector( MultiBlockLattice3D<T,Descriptor>& lattice, Box3D domain,
                        int vectorStartsAt, MultiTensorField3D<T,nDim> &tensor );

template<typename T, template<class U> class Descriptor>
void interpolatePopulations (
        MultiBlockLattice3D<T,Descriptor>& modifiedLattice,
        MultiBlockLattice3D<T,Descriptor>& constLattice,
        plint minIter, plint maxIter, Box3D const& domain );

template<typename T, template<class U> class Descriptor>
void interpolatePopulations (
        MultiBlockLattice3D<T,Descriptor>& modifiedLattice,
        MultiBlockLattice3D<T,Descriptor>& constLattice,
        plint minIter, plint maxIter );



/* *************** PART III ******************************************* */
/* *************** Initialization of scalar- and tensor-fields: ****** */
/* *************** Atomic-Block  ************************************* */
/* ******************************************************************* */

/// Initialize scalar-field with the same constant value on each cell.
template<typename T>
void setToConstant(ScalarField3D<T>& field, Box3D domain, T value);

/// Initialize scalar-field with the same constant value on each cell on which
///   the mask is equal to the flag.
template<typename T>
void setToConstant(ScalarField3D<T>& field, ScalarField3D<int>& mask, int flag,
                   Box3D domain, T value);

/// Initialize tensor-field with the same constant tensor/vector on each cell.
template<typename T, int nDim>
void setToConstant( TensorField3D<T,nDim>& field, Box3D domain, 
                    Array<T,nDim> const& value );

/// Initialize tensor-field with the same constant tensor/vector on each cell on which
///   the mask is equal to the flag.
template<typename T, int nDim>
void setToConstant( TensorField3D<T,nDim>& field, ScalarField3D<int>& mask, int flag,
                    Box3D domain, Array<T,nDim> const& value );

/// Initialize scalar-field with the a value from a function.
// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, class Function>
void setToFunction(ScalarField3D<T>& field, Box3D domain, Function f) {
    applyProcessingFunctional (
            new SetToScalarFunctionFunctional3D<T,Function>(f), domain, field );
}

/// Initialize tensor-field with a vector/tensor value from a function.
// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, int nDim, class Function>
void setToFunction(TensorField3D<T,nDim>& field, Box3D domain, Function f) {
    applyProcessingFunctional (
            new SetToTensorFunctionFunctional3D<T,nDim,Function>(f), domain, field );
}

/// Assign the component "index" of its space coordinate to each cell.
template<typename T>
void setToCoordinate(ScalarField3D<T>& field, Box3D domain, plint index);

/// Assign its space coordinate to each cell.
template<typename T>
void setToCoordinates(TensorField3D<T,3>& field,  Box3D domain);


/// Assign scalar-field to one component of a tensor-field.
template<typename T, int nDim>
void assignComponent(TensorField3D<T,nDim>& tensorField, int whichComponent,
                     ScalarField3D<T>& scalarField, Box3D domain);

/* *************** PART IV ******************************************* */
/* *************** Initialization of scalar- and tensor-fields: ****** */
/* *************** Multi-Block *************************************** */
/* ******************************************************************* */

/// Initialize scalar-field with the same constant value on each cell.
template<typename T>
void setToConstant(MultiScalarField3D<T>& field, Box3D domain, T value);

/// Initialize scalar-field with the same constant value on each cell on which
///   the mask is equal to the flag.
template<typename T>
void setToConstant(MultiScalarField3D<T>& field, MultiScalarField3D<int>& mask, int flag,
                   Box3D domain, T value);

/// Initialize tensor-field with the same constant tensor/vector on each cell.
template<typename T, int nDim>
void setToConstant( MultiTensorField3D<T,nDim>& field, Box3D domain, 
                    Array<T,nDim> const& value );

/// Initialize tensor-field with the same constant tensor/vector on each cell on which
///   the mask is equal to the flag.
template<typename T, int nDim>
void setToConstant( MultiTensorField3D<T,nDim>& field, MultiScalarField3D<int>& mask, int flag,
                    Box3D domain, Array<T,nDim> const& value );

/// Initialize scalar-field with the a value from a function.
// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, class Function>
void setToFunction(MultiScalarField3D<T>& field, Box3D domain, Function f) {
    applyProcessingFunctional (
            new SetToScalarFunctionFunctional3D<T,Function>(f), domain, field );
}

/// Initialize tensor-field with a vector/tensor value from a function.
// This function is implemented in-place, because it cannot be precompiled due to its generic nature.
template<typename T, int nDim, class Function>
void setToFunction(MultiTensorField3D<T,nDim>& field, Box3D domain, Function f) {
    applyProcessingFunctional (
            new SetToTensorFunctionFunctional3D<T,nDim,Function>(f), domain, field );
}

/// Assign the component "index" of its space coordinate to each cell.
template<typename T>
void setToCoordinate(MultiScalarField3D<T>& field, Box3D domain, plint index);

/// Assign its space coordinate to each cell.
template<typename T>
void setToCoordinates(MultiTensorField3D<T,3>& field,  Box3D domain);


/// Assign scalar-field to one component of a tensor-field.
template<typename T, int nDim>
void assignComponent(MultiTensorField3D<T,nDim>& tensorField, int whichComponent,
                     MultiScalarField3D<T>& scalarField, Box3D domain);
}  // namespace plb

#endif  // DATA_INITIALIZER_WRAPPER_3D_H
