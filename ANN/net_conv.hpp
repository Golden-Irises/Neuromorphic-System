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
    net_matrix vecAns(iSampInElemCnt, iSampInChannCnt);
    for (auto i = 0ull; i < setCaffeIdx.length; ++i) vecAns.index(setCaffeIdx[i]) += vecGradOut.index(i);
    return vecAns;
}



NEUNET_END