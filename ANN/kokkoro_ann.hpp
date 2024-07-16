KOKKORO_BEGIN

struct Kokkoro {
    uint64_t iLayersCnt = 0;

    kokkoro_layer_ptr arrLayers[kokkoro_len];
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
template <typename LayerType,
          typename...LayerArgs, typename = std::enable_if_t<std::is_base_of_v<Layer, LayerType>>>
void KokkoroAddLayer(Kokkoro &netSrc, LayerArgs &&...argsLayerInit) { netSrc.arrLayers[netSrc.iLayersCnt++] = std::make_shared<LayerType>(std::forward<LayerArgs>(argsLayerInit)...); }

struct KokkoroANN : Kokkoro {
    uint64_t iTrainBatSz   = 32,
             iTrainBatCnt  = 0,
             iTestBatSz    = 32,
             iTestBatCnt   = 0,
             iTrainDataCnt = 0,
             iTestDataCnt  = 0;

    std::atomic_uint64_t iBatCnt = 0,
                         iAccCnt = 0,
                         iRcCnt  = 0,
                         iStatus = kokkoro_ok;

    double dTrainPrec = .1;

    kokkoro_queue<uint64_t> queTrainAcc, queTrainRc, queTestAcc, queTestRc;

    async_controller asyTrainCtrl, asyTestCtrl;

    async_pool asyPool;

    KokkoroANN(uint64_t iTrainBatchSize = 32, uint64_t iTestBatchSize = 32, double dTrainPrecision = .1) :
        iTrainBatSz(iTrainBatchSize),
        iTestBatSz(iTestBatchSize),
        dTrainPrec(dTrainPrecision),
        asyPool((std::max)(iTrainBatchSize, iTestBatchSize)) {}
};

void KokkoroTrainInit(KokkoroANN &netSrc, uint64_t iTrainDataCnt, uint64_t iTestDataCnt, uint64_t iInLnCnt, uint64_t iInColCnt, uint64_t iChannCnt) {
    netSrc.iTrainBatCnt  = iTrainDataCnt / netSrc.iTrainBatSz;
    netSrc.iTestBatCnt   = iTestDataCnt / netSrc.iTestBatSz;
    netSrc.iTrainDataCnt = iTrainDataCnt;
    netSrc.iTestDataCnt  = iTestDataCnt;
    for (auto i = 0ull; i < netSrc.iLayersCnt; ++i) {
        netSrc.arrLayers[i]->Batch(netSrc.iTrainBatSz, netSrc.iTrainBatCnt);
        netSrc.arrLayers[i]->Shape(iInLnCnt, iInColCnt, iChannCnt);
    }
}

bool KokkoroTrainAbort(KokkoroANN &netSrc) {
    if (netSrc.iStatus != kokkoro_err) return false;
    netSrc.queTestAcc.reset();
    netSrc.queTestRc.reset();
    netSrc.queTrainAcc.reset();
    netSrc.queTrainRc.reset();
    netSrc.asyTrainCtrl.thread_wake_all();
    netSrc.asyTestCtrl.thread_wake_all();
    return true;
}

bool KokkoroTrainStopVerify(KokkoroANN &netSrc) { return netSrc.iStatus == kokkoro_fin || netSrc.iStatus == kokkoro_err; }

void KokkoroTrain(KokkoroANN &netSrc, const kokkoro_set<kokkoro_matrix> &setTrianData, const kokkoro_set<uint64_t> &setTrainLbl, kokkoro_set<uint64_t> &setTrainDataIdx, const kokkoro_set<kokkoro_matrix> &setTestData, const kokkoro_set<uint64_t> &setTestLbl, uint64_t iLblTypeCnt) { for (auto i = 0ull; i < netSrc.asyPool.size(); ++i) netSrc.asyPool.add_task([&netSrc, &setTrianData, &setTrainLbl, &setTestData, &setTestLbl, &setTrainDataIdx, iLblTypeCnt, i]{ while (netSrc.iStatus == kokkoro_ok) {
    auto iDataIdx = i;
    // train
    if (i < netSrc.iTrainBatSz) while (iDataIdx < setTrainLbl.length) {
        auto iLbl    = setTrainLbl[setTrainDataIdx[iDataIdx]];
        auto vecIn   = setTrianData[setTrainDataIdx[iDataIdx]],
             vecOrgn = kokkoro_lbl_orgn(iLbl, iLblTypeCnt);
        for (auto j = 0ull; j < netSrc.iLayersCnt; ++j) netSrc.arrLayers[j]->ForProp(vecIn, i);
        if (!vecIn.verify) netSrc.iStatus = kokkoro_err;
        if (KokkoroTrainAbort(netSrc)) break;
        kokkoro_out_acc_rc(vecIn, netSrc.dTrainPrec, iLbl, netSrc.iAccCnt, netSrc.iRcCnt);
        for (auto j = netSrc.iLayersCnt; j; --j) netSrc.arrLayers[j - 1]->BackProp(vecIn, i, vecOrgn);
        if (!vecIn.verify) netSrc.iStatus = kokkoro_err;
        if (KokkoroTrainAbort(netSrc)) break;
        iDataIdx += netSrc.iTrainBatSz;
        if (++netSrc.iBatCnt == netSrc.iTrainBatSz) {
            netSrc.queTrainAcc.en_queue(netSrc.iAccCnt);
            netSrc.queTrainRc.en_queue(netSrc.iRcCnt);
            netSrc.iAccCnt = 0;
            netSrc.iRcCnt  = 0;
            netSrc.iBatCnt = 0;
            kokkoro_async_sleep(kokkoro_async_sleep_ms);
            netSrc.asyTrainCtrl.thread_wake_all();
            if (iDataIdx >= setTrainLbl.length) netSrc.asyTestCtrl.thread_wake_all();
        } else netSrc.asyTrainCtrl.thread_sleep();
    } else netSrc.asyTestCtrl.thread_sleep();
    if (KokkoroTrainStopVerify(netSrc)) break;
    // test
    iDataIdx = i;
    if (i < netSrc.iTestBatSz) while (iDataIdx < setTestLbl.length) {
        auto iLbl  = setTestLbl[iDataIdx];
        auto vecIn = setTestData[iDataIdx];
        for (auto j = 0ull; j < netSrc.iLayersCnt; ++j) netSrc.arrLayers[j]->Deduce(vecIn);
        if (!vecIn.verify) netSrc.iStatus = kokkoro_err;
        if (KokkoroTrainAbort(netSrc)) break;
        kokkoro_out_acc_rc(vecIn, netSrc.dTrainPrec, iLbl, netSrc.iAccCnt, netSrc.iRcCnt);
        iDataIdx += netSrc.iTestBatSz;
    } else netSrc.asyTrainCtrl.thread_sleep();
    if (++netSrc.iBatCnt == netSrc.iTestBatSz) {
        netSrc.queTestAcc.en_queue(netSrc.iAccCnt);
        netSrc.queTestRc.en_queue(netSrc.iRcCnt);
        netSrc.iAccCnt = 0;
        netSrc.iRcCnt  = 0;
        netSrc.iBatCnt = 0;
        setTrainDataIdx.shuffle();
        netSrc.asyTrainCtrl.thread_wake_all();
    } else netSrc.asyTrainCtrl.thread_sleep();
    if (KokkoroTrainStopVerify(netSrc)) break;
} }); }

void KokkoroTrainResult(KokkoroANN &netSrc) {
    double   dRcRt  = 0;
    uint64_t iEpCnt = 0;
    while (dRcRt < 1) {
        auto cEpTmPt = kokkoro_chrono_time_point;
        // train
        for (auto i = 0ull; i < netSrc.iTrainBatCnt; ++i) {
            auto cTrnTmPt = kokkoro_chrono_time_point;
            auto dAcc     = netSrc.queTrainAcc.de_queue() / (netSrc.iTrainBatSz * 1.);
                 dRcRt    = netSrc.queTrainRc.de_queue() / (netSrc.iTrainBatSz * 1.);
            kokkoro_train_progress((i + 1), netSrc.iTrainBatCnt, dAcc, dRcRt, (kokkoro_chrono_time_point - cTrnTmPt));
        }
        // test
        std::printf("\r[Deducing]...");
        auto dAcc  = netSrc.queTestAcc.de_queue() / (netSrc.iTestDataCnt * 1.);
             dRcRt = netSrc.queTestRc.de_queue() / (netSrc.iTestDataCnt * 1.);
        kokkoro_epoch_status(++iEpCnt, dAcc, dRcRt, (kokkoro_chrono_time_point - cEpTmPt));
    }
    for (auto i = 0ull; i < netSrc.iLayersCnt; ++i) netSrc.arrLayers[i]->SaveData();
}

KOKKORO_END