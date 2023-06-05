NEUNET_BEGIN

struct NeunetCore {
    uint64_t iTrainBatSz   = 32,
             iTrainBatCnt  = 0,
             iTestBatSz    = 32,
             iTestBatCnt   = 0,
             iLayersCnt    = 0,
             iTrainDataCnt = 0,
             iTestDataCnt  = 0;

    std::atomic_uint64_t iBatCnt = 0,
                         iAccCnt = 0,
                         iRcCnt  = 0,
                         iStatus = neunet_ok;

    double dTrainPrec = .1;

    nuenet_layer_ptr arrLayers[nuenet_len];

    net_queue<uint64_t> queTrainAcc, queTrainRc, queTestAcc, queTestRc;

    async_controller asyCtrl;

    async_pool asyPool;

    NeunetCore(uint64_t iTrainBatchSize = 32, uint64_t iTestBatchSize = 32, double dTrainPrecision = .1) :
        iTrainBatSz(iTrainBatchSize),
        iTestBatSz(iTestBatchSize),
        dTrainPrec(dTrainPrecision),
        asyPool(std::max(iTrainBatchSize, iTestBatchSize)) {}
};

/* Construct the layer
 * Built-in Layer instance
 * LayerBias
 * LayerAct
 * LayerPC
 * LayerFC
 * LayerConv
 * LayerPool
 * LayerBN
 * LayerFlat
 */
template <typename LayerType>
void NeunetAddLayer(NeunetCore &netSrc) { netSrc.arrLayers[netSrc.iLayersCnt++] = std::make_shared<LayerType>(); }

void NeunetInit(NeunetCore &netSrc, uint64_t iTrainDataCnt, uint64_t iTestDataCnt, uint64_t iInLnCnt, uint64_t iInColCnt, uint64_t iChannCnt) {
    netSrc.iTrainBatCnt  = iTrainDataCnt / netSrc.iTrainBatSz;
    netSrc.iTestBatCnt   = iTestDataCnt / netSrc.iTestBatSz;
    netSrc.iTrainDataCnt = iTrainDataCnt;
    netSrc.iTestDataCnt  = iTestDataCnt;
    for (auto i = 0ull; i < netSrc.iLayersCnt; ++i) {
        netSrc.arrLayers[i]->Batch(netSrc.iTrainBatSz, netSrc.iTrainBatCnt);
        netSrc.arrLayers[i]->Shape(iInLnCnt, iInColCnt, iChannCnt);
    }
}

bool NeunetAbort(NeunetCore &netSrc) {
    if (netSrc.iStatus != neunet_err) return false;
    netSrc.queTestAcc.reset();
    netSrc.queTestRc.reset();
    netSrc.queTrainAcc.reset();
    netSrc.queTrainRc.reset();
    netSrc.asyCtrl.thread_wake_all();
    return true;
}

bool NeunetStopVerify(NeunetCore &netSrc) { return netSrc.iStatus == neunet_fin || netSrc.iStatus == neunet_err; }

void NeunetRun(NeunetCore &netSrc, const net_set<net_matrix> &setTrianData, const net_set<uint64_t> &setTrainLbl, net_set<uint64_t> &setTrainDataIdx, const net_set<net_matrix> &setTestData, const net_set<uint64_t> &setTestLbl, uint64_t iLblTypeCnt) { for (auto i = 0ull; i < netSrc.asyPool.size(); ++i) netSrc.asyPool.add_task([&netSrc, &setTrianData, &setTrainLbl, &setTestData, &setTestLbl, &setTrainDataIdx, iLblTypeCnt, i]{ while (netSrc.iStatus == neunet_ok) {
    uint64_t iDataIdx = i < netSrc.iTrainBatSz ? i : 0;
    // train
    if (i < netSrc.iTrainBatSz) while (iDataIdx < setTrainLbl.length) {
        auto iLbl    = setTrainLbl[setTrainDataIdx[iDataIdx]];
        auto vecIn   = setTrianData[setTrainDataIdx[iDataIdx]],
             vecOrgn = net_lbl_orgn(iLbl, iLblTypeCnt);
        for (auto j = 0ull; j < netSrc.iLayersCnt; ++j) netSrc.arrLayers[j]->ForProp(vecIn, i);
        if (!vecIn.verify) netSrc.iStatus = neunet_err;
        if (NeunetAbort(netSrc)) break;
        net_out_acc_rc(vecIn, netSrc.dTrainPrec, iLbl, netSrc.iAccCnt, netSrc.iRcCnt);
        for (auto j = netSrc.iLayersCnt; j; --j) netSrc.arrLayers[j - 1]->BackProp(vecIn, i, vecOrgn);
        if (!vecIn.verify) netSrc.iStatus = neunet_err;
        if (NeunetAbort(netSrc)) break;
        iDataIdx += netSrc.iTrainBatSz;
        if (++netSrc.iBatCnt == netSrc.iTrainBatSz) {
            netSrc.queTrainAcc.en_queue(netSrc.iAccCnt);
            netSrc.queTrainRc.en_queue(netSrc.iRcCnt);
            netSrc.iAccCnt = 0;
            netSrc.iRcCnt  = 0;
            netSrc.iBatCnt = 0;
            netSrc.asyCtrl.thread_wake_all();
        } else netSrc.asyCtrl.thread_sleep();
    } else while (iDataIdx < setTrainLbl.length) {
        iDataIdx += netSrc.iTrainBatSz;
        netSrc.asyCtrl.thread_sleep();
        if (NeunetStopVerify(netSrc)) break;
    }
    if (NeunetStopVerify(netSrc)) break;
    // test
    iDataIdx = i;
    if (i < netSrc.iTestBatSz) while (iDataIdx < setTestLbl.length) {
        auto iLbl  = setTestLbl[iDataIdx];
        auto vecIn = setTestData[iDataIdx];
        for (auto j = 0ull; j < netSrc.iLayersCnt; ++j) netSrc.arrLayers[j]->Deduce(vecIn);
        if (!vecIn.verify) netSrc.iStatus = neunet_err;
        if (NeunetAbort(netSrc)) break;
        net_out_acc_rc(vecIn, netSrc.dTrainPrec, iLbl, netSrc.iAccCnt, netSrc.iRcCnt);
        iDataIdx += netSrc.iTestBatSz;
    }
    if (++netSrc.iBatCnt == netSrc.asyPool.size()) {
        netSrc.queTestAcc.en_queue(netSrc.iAccCnt);
        netSrc.queTestRc.en_queue(netSrc.iRcCnt);
        netSrc.iAccCnt = 0;
        netSrc.iRcCnt  = 0;
        netSrc.iBatCnt = 0;
        setTrainDataIdx.shuffle();
        netSrc.asyCtrl.thread_wake_all();
    } else netSrc.asyCtrl.thread_sleep();
    if (NeunetStopVerify(netSrc)) break;
} }); }

void NeunetResult(NeunetCore &netSrc) {
    double   dRcRt  = 0;
    uint64_t iEpCnt = 0;
    while (dRcRt < 1) {
        auto cEpTmPt = neunet_chrono_time_point;
        // train
        for (auto i = 0ull; i < netSrc.iTrainBatCnt; ++i) {
            auto cTrnTmPt = neunet_chrono_time_point;
            auto dAcc     = netSrc.queTrainAcc.de_queue() / (netSrc.iTrainBatSz * 1.);
                 dRcRt    = netSrc.queTrainRc.de_queue() / (netSrc.iTrainBatSz * 1.);
            net_train_progress((i + 1), netSrc.iTrainBatCnt, dAcc, dRcRt, (neunet_chrono_time_point - cTrnTmPt));
        }
        // test
        std::printf("\r[Deducing]...");
        auto dAcc  = netSrc.queTestAcc.de_queue() / (netSrc.iTestDataCnt * 1.);
             dRcRt = netSrc.queTestRc.de_queue() / (netSrc.iTestDataCnt * 1.);
        net_epoch_status(++iEpCnt, dAcc, dRcRt, (neunet_chrono_time_point - cEpTmPt));
    }
}

NEUNET_END