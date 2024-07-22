KOKKORO_BEGIN

template <double dPlaceholder = 0.>
kokkoro_matrix BNBetaGammaInit(uint64_t iChannCnt) {
    kokkoro_matrix vecAns {1, iChannCnt};
    if constexpr (dPlaceholder != 0) for (auto i = 0ull; i < iChannCnt; ++i) vecAns.index(i) = dPlaceholder;
    return vecAns;
}

struct BNData final {
    kokkoro_matrix vecMuBeta, vecSigmaSqr, vecSigmaEps,
                   vecExpMuBeta, vecExpSigmaSqr, vecExpSigmaEps;

    uint64_t iBatCnt = 0,
             iBatIdx = 0;

    double dCoeBatSz = 0;

    kokkoro_set<kokkoro_matrix> setBarX, setDist;
};

void BNDataInit(BNData &BdData, uint64_t iBatchSize, uint64_t iBatchCnt) {
    BdData.iBatCnt   = iBatchCnt;
    BdData.dCoeBatSz = 1. / iBatchSize;
    BdData.setBarX.init(iBatchSize, false);
    BdData.setDist.init(iBatchSize, false);
}

// Train
void BNOut(kokkoro_set<kokkoro_matrix> &setIn, BNData &BdData, const kokkoro_matrix &vecBeta, const kokkoro_matrix &vecGamma) {
    BdData.vecMuBeta  = kokkoro_matrix::sigma(setIn);
    BdData.vecMuBeta *= BdData.dCoeBatSz;
    for (auto i = 0ull; i < setIn.length; ++i) {
        setIn[i]         -= BdData.vecMuBeta;
        BdData.setDist[i] = setIn[i];
        BdData.setDist[i].elem_wise_mul(BdData.setDist[i]);
    }
    BdData.vecSigmaSqr  = kokkoro_matrix::sigma(BdData.setDist);
    BdData.vecSigmaSqr *= BdData.dCoeBatSz;
    BdData.vecSigmaEps  = BdData.vecSigmaSqr;
    BdData.vecSigmaEps.broadcast_add(kokkoro_eps);
    BdData.vecSigmaEps.elem_wise_pow(0.5);
    BdData.setDist = setIn;
    for (auto i = 0ull; i < setIn.length; ++i) {
        setIn[i].elem_wise_div(BdData.vecSigmaEps);
        BdData.setBarX[i] = setIn[i];
        for (auto j = 0ull; j < setIn[i].line_count; ++j) for (auto k = 0ull; k < setIn[i].column_count; ++k) {
            setIn[i][j][k] *= vecGamma.index(k);
            setIn[i][j][k] += vecBeta.index(k);
        }
    }
}

// call after BNOut
template <double dMovAvgDecay = .9>
void BNMovAvg(BNData &BdData) {
    constexpr auto dDcy = 1 - dMovAvgDecay;
    BdData.vecMuBeta   *= dDcy;
    BdData.vecSigmaSqr *= dDcy;
    if (BdData.vecExpMuBeta.verify && BdData.vecExpSigmaSqr.verify) {
        BdData.vecExpMuBeta   *= dMovAvgDecay;
        BdData.vecExpSigmaSqr *= dMovAvgDecay;
        BdData.vecExpMuBeta   += BdData.vecMuBeta;
        BdData.vecExpSigmaSqr += BdData.vecSigmaSqr;
    } else {
        BdData.vecExpMuBeta   = std::move(BdData.vecMuBeta);
        BdData.vecExpSigmaSqr = std::move(BdData.vecSigmaSqr);
    }
    if (++BdData.iBatIdx == BdData.iBatCnt) {
        BdData.iBatIdx        = 0;
        BdData.vecExpSigmaEps = BdData.vecExpSigmaSqr;
        BdData.vecExpSigmaEps.elem_wise_pow(0.5);
    }
}

void BNGradIn(kokkoro_set<kokkoro_matrix> &setGradOut, BNData &BdData, kokkoro_matrix &vecGradBeta, kokkoro_matrix &vecGradGamma, const kokkoro_matrix &vecGamma) {
    // shift gradient
    BdData.vecMuBeta = kokkoro_matrix::sigma(setGradOut);
    if (vecGradBeta.verify) vecGradBeta.clear();
    else vecGradBeta = {1, vecGamma.element_count};
    for (auto i = 0ull; i < BdData.vecMuBeta.line_count; ++i) for (auto j = 0ull; j < vecGradBeta.element_count; ++j) vecGradBeta.index(j) += BdData.vecMuBeta[i][j];
    // scale gradient tensor
    for (auto i = 0ull; i < setGradOut.length; ++i) BdData.setBarX[i].elem_wise_mul(setGradOut[i]);
    BdData.vecSigmaSqr = kokkoro_matrix::sigma(BdData.setBarX);
    // bar x gradient
    for (auto i = 0ull; i < setGradOut.length; ++i) for (auto j = 0ull; j < setGradOut[i].line_count; ++j) for (auto k = 0ull; k < setGradOut[i].column_count; ++k) setGradOut[i][j][k] *= vecGamma.index(k);
    // scale gradient
    if (vecGradGamma.verify) vecGradGamma.clear();
    else vecGradGamma = {1, vecGamma.element_count};
    for (auto i = 0ull; i < BdData.vecSigmaSqr.line_count; ++i) for (auto j = 0ull; j < vecGradGamma.element_count; ++j) vecGradGamma.index(j) += BdData.vecSigmaSqr[i][j];
    // variant gradient
    BdData.setBarX = setGradOut;
    for (auto i = 0ull; i < setGradOut.length; ++i) BdData.setBarX[i].elem_wise_mul(BdData.setDist[i]);
    BdData.vecSigmaSqr = kokkoro_matrix::sigma(BdData.setBarX);
    for (auto i = 0; i < 3; ++i) BdData.vecSigmaSqr.elem_wise_div(BdData.vecSigmaEps);
    BdData.vecSigmaSqr *= BdData.dCoeBatSz;
    // expectation gradient
    for (auto i = 0ull; i < setGradOut.length; ++i) setGradOut[i].elem_wise_div(BdData.vecSigmaEps);
    BdData.vecMuBeta = kokkoro_matrix::sigma(BdData.setDist);
    BdData.vecMuBeta.elem_wise_mul(BdData.vecSigmaSqr);
    BdData.vecMuBeta -= kokkoro_matrix::sigma(setGradOut);
    BdData.vecMuBeta *= BdData.dCoeBatSz;
    // input gradient
    for (auto i = 0ull; i < setGradOut.length; ++i) {
        BdData.setDist[i].elem_wise_mul(BdData.vecSigmaSqr);
        setGradOut[i] -= BdData.setDist[i];
        setGradOut[i] += BdData.vecMuBeta;
    }
}

// Deduce
void BNOut(kokkoro_matrix &vecIn, BNData &BdData, const kokkoro_matrix &vecBeta, const kokkoro_matrix &vecGamma) {
    vecIn -= BdData.vecExpMuBeta;
    vecIn.elem_wise_div(BdData.vecExpSigmaEps);
    for (auto i = 0ull; i < vecIn.line_count; ++i) for (auto j = 0ull; j < vecIn.column_count; ++j) {
        vecIn[i][j] *= vecGamma.index(j);
        vecIn[i][j] += vecBeta.index(j);
    }
}

KOKKORO_END