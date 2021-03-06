/**
 * Copyright (c) 2016-2017 Los Alamos National Security, LLC
 *                         All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * LA-CC 10-123
 */

//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Contact:
// Michael A. Heroux ( maherou@sandia.gov)
// Jack Dongarra     (dongarra@eecs.utk.edu)
// Piotr Luszczek    (luszczek@eecs.utk.edu)
//
// ***************************************************
//@HEADER

/*!
    @file GenerateProblem.cpp

    HPCG routine
 */

#include "LegionStuff.hpp"
#include "LegionMatrices.hpp"

#include "GenerateGeometry.hpp"
#include "GenerateProblem.hpp"
#include "SetupHalo.hpp"

#include <cassert>

/**
 *
 */
inline void
getXYZFineAndCoarse(
    const Geometry &AfGeom,
    global_int_t &nxf,
    global_int_t &nyf,
    global_int_t &nzf,
    local_int_t  &nxc,
    local_int_t  &nyc,
    local_int_t  &nzc
) {
    nxf = AfGeom.nx;
    nyf = AfGeom.ny;
    nzf = AfGeom.nz;
    // Need fine grid dimensions to be divisible by 2.
    assert(nxf % 2 == 0);
    assert(nyf % 2 == 0);
    assert(nzf % 2 == 0);
    //Coarse nx, ny, nz.
    nxc = nxf / 2;
    nyc = nyf / 2;
    nzc = nzf / 2;
}

/**
 *
 */
inline void
GenerateCoarseProblemTopLevel(
    LogicalSparseMatrix &Af,
    int level,
    Context ctx,
    HighLevelRuntime *runtime
) {
    // Make local copies of geometry information.  Use global_int_t since the
    // RHS products in the calculations below may result in global range values.
    global_int_t nxf, nyf, nzf;
    //Coarse nx, ny, nz
    local_int_t nxc, nyc, nzc;
    //
    getXYZFineAndCoarse(*Af.geom, nxf, nyf, nzf, nxc, nyc, nzc);
    // Construct the geometry and linear system
    Geometry *geomc = new Geometry();
    GenerateGeometry(
        Af.geom->size,
        Af.geom->rank,
        Af.geom->numThreads,
        nxc,
        nyc,
        nzc,
        Af.geom->stencilSize,
        geomc
    );
    //
    LogicalSparseMatrix *Ac = new LogicalSparseMatrix();
    //
    std::string name = "A-L" + std::to_string(level);
    Ac->allocate(name, *geomc, ctx, runtime);
    //
    Ac->partition(geomc->size, ctx, runtime);
    //
    Ac->geom = geomc;
    Af.Ac = Ac;
}

/*!
    Routine to construct a prolongation/restriction operator for a given fine
    grid matrix solution (as computed by a direct solver).

    @param[inout]  Af - The known system matrix, on output its coarse operator,
                   fine-to-coarse operator and auxiliary vectors will be defined.

    Note that the matrix Af is considered const because the attributes we are
    modifying are declared as mutable.

*/
inline void
GenerateCoarseProblem(
    SparseMatrix &Af,
    int level,
    Context ctx,
    HighLevelRuntime *lrt
) {
    const Geometry *const AfGeom = Af.geom->data();
    assert(AfGeom);
    // Make local copies of geometry information.  Use global_int_t since the
    // RHS products in the calculations below may result in global range values.
    global_int_t nxf, nyf, nzf;
    //Coarse nx, ny, nz
    local_int_t nxc, nyc, nzc;
    //
    getXYZFineAndCoarse(*AfGeom, nxf, nyf, nzf, nxc, nyc, nzc);
    // Construct the geometry and linear system
    GenerateGeometry(
        AfGeom->size,
        AfGeom->rank,
        AfGeom->numThreads,
        nxc,
        nyc,
        nzc,
        AfGeom->stencilSize,
        Af.Ac->geom->data()
    );
    //
    GenerateProblem(*Af.Ac, NULL, NULL, NULL, level, ctx, lrt);
    GetNeighborInfo(*Af.Ac);
}

/**
 *
 */
inline void
f2cOperatorPopulate(
    SparseMatrix &Af,
    Context,
    HighLevelRuntime *
) {
    const Geometry *const AfGeom = Af.geom->data();
    assert(AfGeom);
    // Make local copies of geometry information.  Use global_int_t since the
    // RHS products in the calculations below may result in global range values.
    global_int_t nxf, nyf, nzf;
    //Coarse nx, ny, nz
    local_int_t nxc, nyc, nzc;
    //
    getXYZFineAndCoarse(*AfGeom, nxf, nyf, nzf, nxc, nyc, nzc);
    //
    local_int_t *f2cOperator = Af.mgData->f2cOperator->data();
    assert(f2cOperator);
    for (local_int_t izc = 0; izc < nzc; ++izc) {
        local_int_t izf = 2 * izc;
        for (local_int_t iyc = 0; iyc < nyc; ++iyc) {
            local_int_t iyf = 2 * iyc;
            for (local_int_t ixc = 0; ixc < nxc; ++ixc) {
                local_int_t ixf = 2 * ixc;
                local_int_t cCoarseRow = izc * nxc * nyc + iyc * nxc + ixc;
                local_int_t cFineRow = izf * nxf * nyf + iyf * nxf + ixf;
                f2cOperator[cCoarseRow] = cFineRow;
            } // end iy loop
        } // end even iz if statement
    } // end iz loop
}
