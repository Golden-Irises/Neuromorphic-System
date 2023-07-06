KOKKORO_BEGIN

kokkoro_set<uint64_t> CaffeIdx(uint64_t &iCaffeLnCnt, uint64_t &iCaffeColCnt, uint64_t &iSampOutLnCnt, uint64_t &iSampOutColCnt, uint64_t iSampInLnCnt, uint64_t iSampInColCnt, uint64_t iSampInChannCnt, uint64_t iFilterLnCnt, uint64_t iFilterColCnt, uint64_t iFilterElemCnt, uint64_t iLnStride, uint64_t iColStride, uint64_t iLnDilate, uint64_t iColDilate) {
    iSampOutLnCnt  = samp_output_dir_cnt(iSampInLnCnt, iFilterLnCnt, iLnStride, iLnDilate);
    iSampOutColCnt = samp_output_dir_cnt(iSampInColCnt, iFilterColCnt, iColStride, iColDilate);
    iCaffeLnCnt    = iSampOutLnCnt * iSampOutColCnt;
    iCaffeColCnt   = iSampInChannCnt * iFilterElemCnt;
    kokkoro_set<uint64_t> setAns(iCaffeLnCnt * iCaffeColCnt);
    for (auto c = 0ull; c < iSampInChannCnt; ++c) for (auto i = 0ull; i < iSampOutLnCnt; ++i) for (auto j = 0ull; j < iSampOutColCnt; ++j) for (auto k = 0ull; k < iFilterLnCnt; ++k) for (auto l = 0ull; l < iFilterColCnt; ++l) {
        auto iMetaLn    = samp_trace_pos(i, k, iLnStride, iLnDilate),
             iMetaCol   = samp_trace_pos(j, l, iColStride, iColDilate),
             iIm2ColLn  = iMetaLn * iSampInColCnt + iMetaCol,
             iIm2ColCol = c,
             iCaffeLn   = i * iSampOutColCnt + j,
             iCaffeCol  = c * iFilterElemCnt + k * iFilterColCnt + l;
        setAns[iCaffeLn * iCaffeColCnt + iCaffeCol] = iIm2ColLn * iSampInChannCnt + iIm2ColCol;
    }
    return setAns;
}

kokkoro_matrix Caffe(const kokkoro_matrix &vecIn, kokkoro_set<uint64_t> setCaffeIdx, uint64_t iCaffeLnCnt, uint64_t iCaffeColCnt) {
    kokkoro_matrix vecAns {iCaffeLnCnt, iCaffeColCnt};
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) vecAns.index(i) = vecIn.index(setCaffeIdx[i]);
    return vecAns;
}

kokkoro_matrix CaffeGradIn(const kokkoro_matrix &vecGradOut, const kokkoro_set<uint64_t> &setCaffeIdx, uint64_t iSampInElemCnt, uint64_t iSampInChannCnt) {
    kokkoro_matrix vecAns {iSampInElemCnt, iSampInChannCnt};
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) vecAns.index(setCaffeIdx[i]) += vecGradOut.index(i);
    return vecAns;
}

kokkoro_matrix ConvInitKernel(uint64_t iQty, uint64_t iChannCnt, uint64_t iLnCnt, uint64_t iColCnt, double dFstRng = -1, double dSndRng = 1) {
    kokkoro_matrix vecAns {iLnCnt * iColCnt * iChannCnt, iQty};
    vecAns.elem_rand(dFstRng, dSndRng);
    return vecAns;
}

kokkoro_matrix Conv(const kokkoro_matrix &vecCaffeOut, const kokkoro_matrix &vecKernel) { return FCOut(vecKernel, vecCaffeOut); }

kokkoro_matrix ConvGradCaffeOut(const kokkoro_matrix &vecGradOut, const kokkoro_matrix &vecKernelT) { return FCGradWeight(vecGradOut, vecKernelT); }

kokkoro_matrix ConvGradKernel(const kokkoro_matrix &vecGradOut, const kokkoro_matrix &vecCaffeOutT) { return FCGradIn(vecGradOut, vecCaffeOutT); }

kokkoro_matrix PoolGlbAvg(const kokkoro_matrix &vecIn) {
    kokkoro_matrix vecAns {1, vecIn.column_count};
    for (auto i = 0ull; i < vecIn.line_count; ++i) for (auto j = 0ull; j < vecIn.column_count; ++j) vecAns.index(j) += vecIn[i][j];
    vecAns.elem_wise_div(vecIn.line_count);
    return vecAns;
}

kokkoro_matrix PoolGradGlbAvgIn(const kokkoro_matrix vecGradOut, uint64_t iSampInElemCnt, uint64_t iSampInChannCnt) {
    kokkoro_matrix vecAns {iSampInElemCnt, iSampInChannCnt};
    for (auto i = 0ull; i < vecGradOut.element_count; ++i) vecAns.index(i) = vecGradOut.index(i) / iSampInElemCnt;
    for (auto i = 1ull; i < vecAns.line_count; ++i) for (auto j = 0ull; j < vecAns.column_count; ++j) vecAns[i][j] = vecAns.index(j);
    return vecAns;
}

kokkoro_matrix PoolAvg(const kokkoro_matrix vecIn, const kokkoro_set<uint64_t> setCaffeIdx, uint64_t iFilterElemCnt, uint64_t iCaffeLnCnt) {
    kokkoro_matrix vecAns {iCaffeLnCnt, vecIn.column_count};
    uint64_t iAnsElemCnt = 0,
             iStrideCnt  = 0;
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) {
        vecAns.index(iAnsElemCnt) += vecIn.index(setCaffeIdx[i]);
        if (++iStrideCnt == iFilterElemCnt) {
            vecAns.index(iAnsElemCnt++) /= iFilterElemCnt;
            iStrideCnt = 0;
        }
    }
    return vecAns;
}

kokkoro_matrix PoolGradAvgIn(const kokkoro_matrix &vecGradOut, const kokkoro_set<uint64_t> &setCaffeIdx, uint64_t iFilterElemCnt, uint64_t iSampInElemCnt) {
    kokkoro_matrix vecAns {iSampInElemCnt, vecGradOut.column_count};
    uint64_t iGradElemCnt = 0,
             iStrideCnt   = 0;
    double   dGradTmp     = 0;
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) {
        if (!iStrideCnt) dGradTmp = vecGradOut.index(iGradElemCnt++) / iFilterElemCnt;
        vecAns.index(setCaffeIdx[i]) += dGradTmp;
        if (++iStrideCnt == iFilterElemCnt) iStrideCnt = 0;
    }
    return vecAns;
}

kokkoro_matrix PoolMax(const kokkoro_matrix &vecIn, const kokkoro_set<uint64_t> &setCaffeIdx, uint64_t iFilterElemCnt, uint64_t iCaffeLnCnt, kokkoro_set<kokkoro_set<uint64_t>> &setElemIdx = kokkoro_null_ref(kokkoro_set<kokkoro_set<uint64_t>>)) {
    kokkoro_matrix vecAns {iCaffeLnCnt, vecIn.column_count};
    if (&setElemIdx) {
        if (!setElemIdx.length) setElemIdx.init(vecAns.element_count, false);
        if (!setElemIdx[0].length) setElemIdx[0].init(iFilterElemCnt + 1);
        setElemIdx[0][0] = 0;
    }
    uint64_t iAnsElemCnt = 0,
             iStrideCnt  = 0;
    double   dMaxValTemp = 0;
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) {
        auto iCurrIdx  = setCaffeIdx[i];
        auto dCurrElem = vecIn.index(iCurrIdx);
        if (!iStrideCnt) dMaxValTemp = dCurrElem;
        if (dCurrElem >= dMaxValTemp) {
            if (dCurrElem > dMaxValTemp) {
                dMaxValTemp = dCurrElem;
                if (&setElemIdx) setElemIdx[iAnsElemCnt][0] = 0;
            }
            if (&setElemIdx) setElemIdx[iAnsElemCnt][++setElemIdx[iAnsElemCnt][0]] = iCurrIdx;
        }
        if (++iStrideCnt == iFilterElemCnt) {
            vecAns.index(iAnsElemCnt++) = dMaxValTemp;
            if (&setElemIdx && iAnsElemCnt < setElemIdx.length) {
                if (!setElemIdx[iAnsElemCnt].length) setElemIdx[iAnsElemCnt].init(iFilterElemCnt + 1);
                setElemIdx[iAnsElemCnt][0] = 0;
            }
            iStrideCnt = 0;
        }
    }
    return vecAns;
}

kokkoro_matrix PoolGradMaxIn(const kokkoro_matrix &vecGradOut, uint64_t iSampInElemCnt, const kokkoro_set<kokkoro_set<uint64_t>> &setElemIdx) {
    kokkoro_matrix vecAns(iSampInElemCnt, vecGradOut.column_count);
    for(auto i = 0ull; i < setElemIdx.length; ++i) for (auto j = 0ull; j < setElemIdx[i][0]; ++j) vecAns.index(setElemIdx[i][j + 1]) += vecGradOut.index(i);
    return vecAns;
}

KOKKORO_END