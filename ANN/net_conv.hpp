NEUNET_BEGIN

net_set<uint64_t> CaffeIdx(uint64_t &iCaffeLnCnt, uint64_t &iCaffeColCnt, uint64_t &iSampOutLnCnt, uint64_t &iSampOutColCnt, uint64_t iSampInLnCnt, uint64_t iSampInColCnt, uint64_t iSampInChannCnt, uint64_t iFilterLnCnt, uint64_t iFilterColCnt, uint64_t iFilterElemCnt, uint64_t iLnStride, uint64_t iColStride, uint64_t iLnDilate, uint64_t iColDilate) {
    iSampOutLnCnt  = samp_output_dir_cnt(iSampInLnCnt, iFilterLnCnt, iLnStride, iLnDilate);
    iSampOutColCnt = samp_output_dir_cnt(iSampInColCnt, iFilterColCnt, iColStride, iColDilate);
    iCaffeLnCnt    = iSampOutLnCnt * iSampOutColCnt;
    iCaffeColCnt   = iSampInChannCnt * iFilterElemCnt;
    net_set<uint64_t> setAns(iCaffeLnCnt * iCaffeColCnt);
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

net_matrix Caffe(const net_matrix &vecIn, net_set<uint64_t> setCaffeIdx, uint64_t iCaffeLnCnt, uint64_t iCaffeColCnt) {
    net_matrix vecAns {iCaffeLnCnt, iCaffeColCnt};
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) vecAns.index(i) = vecIn.index(setCaffeIdx[i]);
    return vecAns;
}

net_matrix CaffeGradIn(const net_matrix &vecGradOut, const net_set<uint64_t> &setCaffeIdx, uint64_t iSampInElemCnt, uint64_t iSampInChannCnt) {
    net_matrix vecAns {iSampInElemCnt, iSampInChannCnt};
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) vecAns.index(setCaffeIdx[i]) += vecGradOut.index(i);
    return vecAns;
}

net_matrix ConvInitKernel(uint64_t iQty, uint64_t iChannCnt, uint64_t iLnCnt, uint64_t iColCnt, double dFstRng = -1, double dSndRng = 1) {
    net_matrix vecAns {iLnCnt * iColCnt * iChannCnt, iQty};
    vecAns.elem_rand(dFstRng, dSndRng);
    return vecAns;
}

net_matrix Conv(const net_matrix &vecCaffeOut, const net_matrix &vecKernel) { return FCOut(vecKernel, vecCaffeOut); }

net_matrix ConvGradCaffeOut(const net_matrix &vecGradOut, const net_matrix &vecKernelT) { return FCGradWeight(vecGradOut, vecKernelT); }

net_matrix ConvGradKernel(const net_matrix &vecGradOut, const net_matrix &vecCaffeOutT) { return FCGradIn(vecGradOut, vecCaffeOutT); }

net_matrix PoolGlbAvg(const net_matrix &vecIn) {
    net_matrix vecAns {1, vecIn.column_count};
    for (auto i = 0ull; i < vecIn.line_count; ++i) for (auto j = 0ull; j < vecIn.column_count; ++j) vecAns.index(j) += vecIn[i][j];
    vecAns.elem_wise_div(vecIn.line_count);
    return vecAns;
}

net_matrix PoolGradGlbAvgIn(const net_matrix vecGradOut, uint64_t iSampInElemCnt, uint64_t iSampInChannCnt) {
    net_matrix vecAns {iSampInElemCnt, iSampInElemCnt};
    for (auto i = 0ull; i < vecGradOut.element_count; ++i) vecAns.index(i) = vecGradOut.index(i) / iSampInElemCnt;
    for (auto i = 1ull; i < vecAns.line_count; ++i) for (auto j = 0ull; j < vecAns.column_count; ++j) vecAns[i][j] = vecAns.index(j);
    return vecAns;
}

net_matrix PoolAvg(const net_matrix vecIn, const net_set<uint64_t> setCaffeIdx, uint64_t iFilterElemCnt, uint64_t iCaffeLnCnt) {
    net_matrix vecAns {iCaffeLnCnt, vecIn.column_count};
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

net_matrix PoolGradAvgIn(const net_matrix &vecGradOut, const net_set<uint64_t> &setCaffeIdx, uint64_t iFilterElemCnt, uint64_t iSampInElemCnt) {
    net_matrix vecAns {iSampInElemCnt, vecGradOut.column_count};
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

net_matrix PoolMax(const net_matrix &vecIn, const net_set<uint64_t> &setCaffeIdx, uint64_t iFilterElemCnt, uint64_t iCaffeLnCnt, net_set<net_set<uint64_t>> &setElemIdx = neunet_null_ref(net_set<net_set<uint64_t>>)) {
    net_matrix vecAns {iCaffeLnCnt, vecIn.column_count};
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

net_matrix PoolGradMaxIn(const net_matrix &vecGradOut, uint64_t iSampInElemCnt, const net_set<net_set<uint64_t>> &setElemIdx) {
    net_matrix vecAns(iSampInElemCnt, vecGradOut.column_count);
    for(auto i = 0ull; i < setElemIdx.length; ++i) for (auto j = 0ull; j < setElemIdx[i][0]; ++j) vecAns.index(setElemIdx[i][j + 1]) += vecGradOut.index(i);
    return vecAns;
}

NEUNET_END