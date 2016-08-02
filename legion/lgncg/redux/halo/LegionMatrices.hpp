/*
 * Copyright (c) 2014-2016 Los Alamos National Security, LLC
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

#pragma once

#include "LegionStuff.hpp"
#include "LegionItems.hpp"
#include "LegionArrays.hpp"

#include "hpcg.hpp"
#include "Geometry.hpp"
#include "MGData.hpp"

#include <vector>
#include <map>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct SparseMatrixLocalData {
    //total number of matrix rows across all processes
    global_int_t totalNumberOfRows = 0;
    //total number of matrix nonzeros across all processes
    global_int_t totalNumberOfNonzeros = 0;
    //number of rows local to this process
    local_int_t localNumberOfRows = 0;
    //number of columns local to this process
    local_int_t localNumberOfColumns = 0;
    //number of nonzeros local to this process
    local_int_t localNumberOfNonzeros = 0;
};
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct LogicalSparseMatrix {
public:
    // launch domain
    LegionRuntime::HighLevel::Domain launchDomain;
public:
    //
    LogicalArray<Geometry> geometries;
    //
    LogicalArray<SparseMatrixLocalData> localData;
    // The number of nonzeros in a row will always be 27 or fewer
    LogicalArray<LogicalRegion> lrNonzerosInRow;
    // Logical regions that point to regions of matrix indices as global values
    LogicalArray<LogicalRegion> lrMtxIndG;
    //Logical regions that point to matrix indices as local values
    LogicalArray<LogicalRegion> lrMtxIndL;
    //Logical regions that point to values of matrix entries
    LogicalArray<LogicalRegion> lrMatrixValues;
    //Logical regions that point to values of matrix diagonal entries
    LogicalArray<LogicalRegion> lrMatrixDiagonal;
    //Logical regions that point to local-to-global mapping arrays
    LogicalArray<LogicalRegion> lrLocalToGlobalMap;
    //global-to-local mapping
    std::map<global_int_t, local_int_t> globalToLocalMap;
    /*!
      This is for storing optimized data structres created in OptimizeProblem and
      used inside optimized ComputeSPMV().
      */
    // Coarse grid matrix
    mutable struct SparseMatrix_STRUCT *Ac;
    // Pointer to the coarse level data for this fine matrix
    mutable MGData *mgData;
    // pointer that can be used to store implementation-specific data
    void *optimizationData;
    //number of entries that are external to this process
    local_int_t numberOfExternalValues;
    //number of neighboring processes that will be send local data
    int numberOfSendNeighbors;
    //total number of entries to be sent
    local_int_t totalToBeSent;
    //elements to send to neighboring processes
    local_int_t *elementsToSend;
    //neighboring processes
    int *neighbors;
    //lenghts of messages received from neighboring processes
    local_int_t *receiveLength;
    //lenghts of messages sent to neighboring processes
    local_int_t *sendLength;
    //send buffer for non-blocking sends
    floatType *sendBuffer;

    /**
     *
     */
    LogicalSparseMatrix(void) = default;

    /**
     *
     */
    void
    allocate(
        const Geometry &geom,
        LegionRuntime::HighLevel::Context &ctx,
        LegionRuntime::HighLevel::HighLevelRuntime *lrt
    ) {
        const auto size = geom.size;
        //
                geometries.allocate(size, ctx, lrt);
                 localData.allocate(size, ctx, lrt);
           lrNonzerosInRow.allocate(size, ctx, lrt);
                 lrMtxIndG.allocate(size, ctx, lrt);
                 lrMtxIndL.allocate(size, ctx, lrt);
            lrMatrixValues.allocate(size, ctx, lrt);
          lrMatrixDiagonal.allocate(size, ctx, lrt);
        lrLocalToGlobalMap.allocate(size, ctx, lrt);
    }

    /**
     *
     */
    void
    partition(
        int64_t nParts,
        LegionRuntime::HighLevel::Context &ctx,
        LegionRuntime::HighLevel::HighLevelRuntime *lrt
    ) {
                geometries.partition(nParts, ctx, lrt);
                 localData.partition(nParts, ctx, lrt);
           lrNonzerosInRow.partition(nParts, ctx, lrt);
                 lrMtxIndG.partition(nParts, ctx, lrt);
                 lrMtxIndL.partition(nParts, ctx, lrt);
            lrMatrixValues.partition(nParts, ctx, lrt);
          lrMatrixDiagonal.partition(nParts, ctx, lrt);
        lrLocalToGlobalMap.partition(nParts, ctx, lrt);
        // just pick a structure that has a representative launch domain.
        launchDomain = geometries.launchDomain;
    }

    /**
     * Cleans up and returns all allocated resources.
     */
    void
    deallocate(
        LegionRuntime::HighLevel::Context &ctx,
        LegionRuntime::HighLevel::HighLevelRuntime *lrt
    ) {
                geometries.deallocate(ctx, lrt);
                 localData.deallocate(ctx, lrt);
           lrNonzerosInRow.deallocate(ctx, lrt);
                 lrMtxIndG.deallocate(ctx, lrt);
                 lrMtxIndL.deallocate(ctx, lrt);
            lrMatrixValues.deallocate(ctx, lrt);
          lrMatrixDiagonal.deallocate(ctx, lrt);
        lrLocalToGlobalMap.deallocate(ctx, lrt);
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class SparseMatrix {
protected:
    static constexpr int cNItemsToUnpack = 8;
    // Physical Item Container
    struct PIC {
        Item<Geometry> geom;
        Item<SparseMatrixLocalData> localData;
        Item<LogicalRegion> nonzerosInRow;
        Item<LogicalRegion> mtxIndG;
        Item<LogicalRegion> mtxIndL;
        Item<LogicalRegion> matrixValues;
        Item<LogicalRegion> matrixDiagonal;
        Item<LogicalRegion> localToGlobalMap;
    };
public:
    PIC pic;
    //
    Geometry *geom = nullptr;
    //
    SparseMatrixLocalData *localData = nullptr;

    /**
     *
     */
    SparseMatrix(void) = default;

    /**
     *
     */
    SparseMatrix(
        const std::vector<LegionRuntime::HighLevel::PhysicalRegion> &regions,
        size_t baseRegionID,
        LegionRuntime::HighLevel::Context ctx,
        LegionRuntime::HighLevel::Runtime *runtime
    ) {
        size_t curRID = baseRegionID;
        //
        pic.geom = Item<Geometry>(regions[curRID++], ctx, runtime);
        geom = pic.geom.data();
        assert(geom);
        //
        pic.localData = Item<SparseMatrixLocalData>(
            regions[curRID++], ctx, runtime
        );
        localData = pic.localData.data();
        assert(localData);
        //
        pic.nonzerosInRow = Item<LogicalRegion>(
            regions[curRID++], ctx, runtime
        );
        //
        pic.mtxIndG = Item<LogicalRegion>(regions[curRID++], ctx, runtime);
        //
        pic.mtxIndL = Item<LogicalRegion>(regions[curRID++], ctx, runtime);
        //
        pic.matrixValues = Item<LogicalRegion>(regions[curRID++], ctx, runtime);
        //
        pic.matrixDiagonal = Item<LogicalRegion>(
            regions[curRID++], ctx, runtime
        );
        //
        pic.localToGlobalMap = Item<LogicalRegion>(
            regions[curRID++], ctx, runtime
        );
    }

    /**
     *
     */
    static int
    nRegionEntries(void) { return cNItemsToUnpack; }
};
