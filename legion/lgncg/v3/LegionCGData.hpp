/**
 * Copyright (c)      2017 Los Alamos National Security, LLC
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

#pragma once

#include "LegionArrays.hpp"
#include "LegionMatrices.hpp"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct LogicalCGData : public LogicalMultiBase {
    //
    LogicalArray<floatType> r;  //!< residual vector
    //
    LogicalArray<floatType> z;  //!< preconditioned residual vector
    //
    LogicalArray<floatType> p;  //!< direction vector
    //
    LogicalArray<floatType> Ap; //!< Krylov vector

protected:

    /**
     * Order matters here. If you update this, also update unpack.
     */
    void
    mPopulateRegionList(void) {
        mLogicalItems = {&r, &z, &p, &Ap};
    }

public:
    /**
     *
     */
    LogicalCGData(void) {
        mPopulateRegionList();
    }

    /**
     *
     */
    void
    allocate(
        const Geometry &geom,
        LegionRuntime::HighLevel::Context &ctx,
        LegionRuntime::HighLevel::HighLevelRuntime *lrt
    ) {
        const auto globalXYZ = getGlobalXYZ(geom);

        r.allocate(globalXYZ, ctx, lrt);
#warning "Misallocation of structure..."
        z.allocate(globalXYZ, ctx, lrt);
#warning "Misallocation of structure..."
        p.allocate(globalXYZ, ctx, lrt);
        Ap.allocate(globalXYZ, ctx, lrt);
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
        r.partition(nParts, ctx, lrt);
        z.partition(nParts, ctx, lrt);
        p.partition(nParts, ctx, lrt);
        Ap.partition(nParts, ctx, lrt);
    }

    /**
     * Cleans up and returns all allocated resources.
     */
    void
    deallocate(
        LegionRuntime::HighLevel::Context &ctx,
        LegionRuntime::HighLevel::HighLevelRuntime *lrt
    ) {
        r.deallocate(ctx, lrt);
        z.deallocate(ctx, lrt);
        p.deallocate(ctx, lrt);
        Ap.deallocate(ctx, lrt);
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct CGData : public PhysicalMultiBase {
    //
    Array<floatType> *r = nullptr;
    //
    Array<floatType> *z = nullptr;
    //
    Array<floatType> *p = nullptr;
    //
    Array<floatType> *Ap = nullptr;

    /**
     *
     */
    CGData(void) = default;

    /**
     *
     */
    virtual
    ~CGData(void) {
        delete r;
        delete z;
        delete p;
        delete Ap;
    }

    /**
     *
     */
    CGData(
        const std::vector<PhysicalRegion> &regions,
        size_t baseRID,
        Context ctx,
        HighLevelRuntime *runtime
    ) {
        mUnpack(regions, baseRID, ctx, runtime);
        mVerifyUnpack();
    }

protected:
    /**
     * MUST MATCH PACK ORDER IN mPopulateRegionList!
     */
    void
    mUnpack(
        const std::vector<PhysicalRegion> &regions,
        size_t baseRID,
        Context ctx,
        HighLevelRuntime *rt
    ) {
        size_t cid = baseRID;
        // Populate members from physical regions.
        r = new Array<floatType>(regions[cid++], ctx, rt);
        //
        z = new Array<floatType>(regions[cid++], ctx, rt);
        //
        p = new Array<floatType>(regions[cid++], ctx, rt);
        //
        Ap = new Array<floatType>(regions[cid++], ctx, rt);
        // Calculate number of region entries for this structure.
        mNRegionEntries = cid - baseRID;
    }

    /**
     *
     */
    void
    mVerifyUnpack(void) {
        assert(r->data());
        assert(z->data());
        assert(p->data());
        assert(Ap->data());
    }
};