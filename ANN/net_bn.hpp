NEUNET_BEGIN

struct BNBetaGamma final {
    net_matrix vecBeta, vecBetaN,
               vecGamma, vecGammaN;
    
    ada_delta adaBeta, adaGamma;

    ada_nesterov adnBeta, adnGamma;

    double dBetaLearnRate  = 0,
           dGammaLearnRate = 0;
};

net_matrix BNBetaGammaInit(uint64_t iChannCnt, double dPlaceholder = 0) {
    net_matrix vecAns {1, iChannCnt};
    if (dPlaceholder) for (auto i = 0ull; i < iChannCnt; ++i) vecAns.index(i) = dPlaceholder;
    return vecAns;
}

void BNBetaGammaInit(BNBetaGamma &BbgData, uint64_t iChannCnt, double dBeta = 0, double dGamma = 1) {
    BbgData.vecBeta   = BNBetaGammaInit(iChannCnt, dBeta);
    BbgData.vecBetaN  = BbgData.vecBeta;
    BbgData.vecGamma  = BNBetaGammaInit(iChannCnt, dGamma);
    BbgData.vecGammaN = BbgData.vecGamma;
}

// call after BNGradIn
void BNUpdate(BNBetaGamma &BbgData) {
    ada_update(BbgData.vecBetaN, BbgData.vecBeta, BbgData.vecBetaN, BbgData.dBetaLearnRate, BbgData.adaBeta, BbgData.adnBeta);
    ada_update(BbgData.vecGammaN, BbgData.vecGamma, BbgData.vecGammaN, BbgData.dGammaLearnRate, BbgData.adaGamma, BbgData.adnGamma);
}

const net_matrix &BNBeta(const BNBetaGamma &BbgData) { return BbgData.dBetaLearnRate ? BbgData.vecBetaN : BbgData.vecBeta; }

const net_matrix &BNGamma(const BNBetaGamma &BbgData) { return BbgData.dGammaLearnRate ? BbgData.vecGammaN : BbgData.vecGamma; }

struct BNData final {
    net_matrix vecMuBeta, vecSigmaSqr,
               vecSigmaEps, vecSigmaSqrEps,
               vecExpMuBeta, vecExpSigmaSqr,
               vecExpSigmaEps;

    uint64_t iTrnBatCnt = 0,
             iTrnBatIdx = 0;

    double dCoeBatSz = 0;

    net_set<net_matrix> setBarX, setDist;
};

void BNDataInit(BNData &BdData, uint64_t iTrainBatchSize, uint64_t iTrainBatchBatchCnt) {
    BdData.iTrnBatCnt = iTrainBatchBatchCnt;
    BdData.dCoeBatSz  = 1. / iTrainBatchSize;
    BdData.setBarX.init(iTrainBatchSize, false);
    BdData.setDist.init(iTrainBatchSize, false);
}

void BNOut(net_set<net_matrix> &setIn, BNBetaGamma &BbgData, BNData &BdData) {
    BdData.vecMuBeta  = net_matrix::sigma(setIn);
    BdData.vecMuBeta *= BdData.dCoeBatSz;
    for (auto i = 0ull; i < setIn.length; ++i) {
        setIn[i]         -= BdData.vecMuBeta;
        BdData.setDist[i] = setIn[i];
        BdData.setDist[i].elem_wise_mul(BdData.setDist[i]);
    }
    BdData.vecSigmaSqr  = net_matrix::sigma(BdData.setDist);
    BdData.vecSigmaSqr *= BdData.dCoeBatSz;
    BdData.vecSigmaEps  = BdData.vecSigmaSqr;
    BdData.vecSigmaEps.broadcast_add(neunet_eps);
    BdData.vecSigmaEps.elem_wise_pow(0.5);
    BdData.setDist = setIn;
    for (auto i = 0ull; i < setIn.length; ++i) {
        setIn[i].elem_wise_div(BdData.vecSigmaEps);
        BdData.setBarX[i] = setIn[i];
        for (auto j = 0ull; j < setIn[i].line_count; ++j) for (auto k = 0ull; k < setIn[i].column_count; ++k) {
            setIn[i][j][k] *= BNGamma(BbgData).index(k);
            setIn[i][j][k] += BNBeta(BbgData).index(k);
        }
    }
}

// call after BNOut
void BNMovAvg(BNData &BdData, double dDecay = .9) {
    auto dHalfDecay     = 1 - dDecay;
    BdData.vecMuBeta   *= dHalfDecay;
    BdData.vecSigmaSqr *= dHalfDecay;
    if (BdData.vecExpMuBeta.verify && BdData.vecExpSigmaSqr.verify) {
        BdData.vecExpMuBeta   *= dDecay;
        BdData.vecExpSigmaSqr *= dDecay;
        BdData.vecExpMuBeta   += BdData.vecMuBeta;
        BdData.vecExpSigmaSqr += BdData.vecSigmaSqr;
    } else {
        BdData.vecExpMuBeta   = std::move(BdData.vecMuBeta);
        BdData.vecExpSigmaSqr = std::move(BdData.vecSigmaSqr);
    }
    if (++BdData.iTrnBatIdx == BdData.iTrnBatCnt) {
        BdData.iTrnBatIdx     = 0;
        BdData.vecExpSigmaEps = BdData.vecExpSigmaSqr;
        BdData.vecExpSigmaEps.elem_wise_pow(0.5);
    }
}

void BNGradIn(net_set<net_matrix> &setGradOut, BNBetaGamma &BbgData, BNData &BdData) {
    BdData.vecExpSigmaEps = net_matrix::sigma(setGradOut);
    for (auto i = 0ull; i < BdData.vecExpSigmaEps.line_count; ++i) for (auto j = 0ull; j < BdData.vecExpSigmaEps.column_count; ++j) BbgData.vecBetaN.index(j) += BdData.vecExpSigmaEps[i][j];
    for (auto i = 0ull; i < setGradOut.length; ++i) BdData.setBarX[i].elem_wise_mul(setGradOut[i]);
    BdData.vecExpSigmaEps = net_matrix::sigma(BdData.setBarX);
    for (auto i = 0ull; i < setGradOut.length; ++i) for (auto j = 0ull; j < setGradOut[i].line_count; ++j) for (auto k = 0ull; k < setGradOut[i].column_count; ++k) setGradOut[i][j][k] *= BNGamma(BbgData).index(k);
    for (auto i = 0ull; i < BdData.vecExpSigmaEps.line_count; ++i) for (auto j = 0ull; j < BdData.vecExpSigmaEps.column_count; ++j) BbgData.vecGammaN.index(j) += BdData.vecExpSigmaEps[i][j];
    BdData.setBarX = setGradOut;
    for (auto i = 0ull; i < setGradOut.length; ++i) BdData.setBarX[i].elem_wise_mul(BdData.setDist[i]);
    BdData.vecSigmaSqr = net_matrix::sigma(BdData.setBarX);
    for (auto i = 0; i < 3; ++i) BdData.vecSigmaSqr.elem_wise_div(BdData.vecSigmaEps);
    BdData.vecSigmaSqr *= BdData.dCoeBatSz;
    for (auto i = 0ull; i < setGradOut.length; ++i) setGradOut[i].elem_wise_div(BdData.vecSigmaEps);
    BdData.vecMuBeta = net_matrix::sigma(BdData.setDist);
    BdData.vecMuBeta.elem_wise_mul(BdData.vecSigmaSqr);
    BdData.vecMuBeta -= net_matrix::sigma(setGradOut);
    BdData.vecMuBeta *= BdData.dCoeBatSz;
    for (auto i = 0ull; i < setGradOut.length; ++i) {
        BdData.setDist[i].elem_wise_mul(BdData.vecSigmaSqr);
        setGradOut[i] -= BdData.setDist[i];
        setGradOut[i] += BdData.vecMuBeta;
    }
}

NEUNET_END