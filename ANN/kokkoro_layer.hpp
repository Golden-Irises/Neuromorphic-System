KOKKORO_BEGIN

struct Layer {
    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) = 0;

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) = 0;

    virtual void Deduce(kokkoro_matrix &vecIn) = 0;

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) {}

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {}

    virtual bool SaveData() const { return true; }

    virtual constexpr uint64_t LayerType() const = 0;
};

struct LayerIO : virtual Layer {
    kokkoro_set<kokkoro_matrix> setIO;

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) { setIO.init(iBatSz, false); }
};

template <double dLearnRate = 0.,
          double dGradDecay = 0.9>
struct LayerWeight : virtual LayerIO {
    kokkoro_counter iBatSzCnt;

    kokkoro_matrix vecWeight, vecWeightN, vecWeightT;

    ada_delta<dGradDecay> AdaD;

    ada_nesterov<dLearnRate, dGradDecay> AdaN;

    std::string sSavePath {""};

    LayerWeight(const std::string &sWeightFileSavePath = "", const std::string &sWeightFileLoadPath = "") :
        sSavePath(sWeightFileSavePath) {
        if (!sWeightFileLoadPath.length()) return;
        auto tabWeight = csv_in(sWeightFileLoadPath);
        vecWeight      = {tabWeight.length, tabWeight[0].length};
        for (auto i = 0ull; i < vecWeight.line_count; ++i) for (auto j = 0ull; j < vecWeight.column_count; ++j) vecWeight[i][j] = std::stod(tabWeight[i][j]);
    }

    template <bool bTranspose = false>
    void Update() {
        auto vecGrad = kokkoro_matrix::sigma(setIO);
        vecGrad.elem_wise_div(setIO.length);
        if constexpr (dLearnRate == 0){
            AdaD.update(vecWeight, vecGrad);
            if constexpr (bTranspose) vecWeightT = vecWeight.transpose;
        } else {
            AdaN.update(vecWeight, vecWeightN, vecGrad);
            if constexpr (bTranspose) vecWeightT = vecWeightN.transpose;
        }
    }

    virtual bool SaveData() const {
        kokkoro_set<kokkoro_set<std::string>> tabWeight(vecWeight.line_count);
        for (auto i = 0ull; i < vecWeight.line_count; ++i) {
            tabWeight[i].init(vecWeight.column_count);
            for (auto j = 0ull; j < vecWeight.column_count; ++j) tabWeight[i][j] = std::to_string(vecWeight[i][j]);
        }
        return csv_out(tabWeight, sSavePath);
    }
};

template <double dLearnRate  = 0.,
          double dRandFstRng = -1.,
          double dRandSndRng = 1.,
          double dGradDecay  = 0.9>
struct LayerBias : LayerWeight<dLearnRate,
                               dGradDecay> {
    LayerBias(const std::string &sBiasFileSavePath = "", const std::string &sBiasFileLoadPath = "") : LayerWeight<dLearnRate, dGradDecay>(sBiasFileSavePath, sBiasFileLoadPath) {}
    
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        if (this->vecWeight.verify) return;
        this->vecWeight = {iInLnCnt * iInColCnt, iInChannCnt};
        this->vecWeight.elem_rand(dRandFstRng, dRandSndRng);
        if constexpr (dLearnRate != 0) this->vecWeightN = this->vecWeight;
    }

    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) {
        if constexpr (dLearnRate == 0) Deduce(vecIn);
        else vecIn += this->vecWeightN;
    }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = vecGrad;
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            this->iBatSzCnt.cnt = 0;
            this->Update();
        }
    }

    virtual void Deduce(kokkoro_matrix &vecIn) { vecIn += this->vecWeight; }

    virtual constexpr uint64_t LayerType() const { return kokkoro_bias; }
};

template <uint64_t iActFnType = kokkoro_null>
struct LayerAct : LayerIO {
    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) {
        if constexpr (iActFnType == kokkoro_sigmoid || iActFnType == kokkoro_ReLU) setIO[iBatSzIdx] = vecIn;
        Deduce(vecIn);
    }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        if constexpr (iActFnType == kokkoro_softmax) softmax_cec_grad(vecGrad, vecOrgn);
        constexpr auto bActReLU = iActFnType == kokkoro_ReLU;
        if constexpr (bActReLU || iActFnType == kokkoro_sigmoid) {
            if constexpr (bActReLU) kokkoro_traverse(setIO[iBatSzIdx], ReLU_dv);
            else kokkoro_traverse(setIO[iBatSzIdx], sigmoid_dv);
            vecGrad.elem_wise_mul(setIO[iBatSzIdx]);
        }
    }

    virtual void Deduce(kokkoro_matrix &vecIn) {
        if constexpr (iActFnType == kokkoro_sigmoid) kokkoro_traverse(vecIn, sigmoid);
        if constexpr (iActFnType == kokkoro_ReLU) kokkoro_traverse(vecIn, ReLU);
        if constexpr (iActFnType == kokkoro_softmax) softmax(vecIn);
    }

    virtual constexpr uint64_t LayerType() const { return kokkoro_act; }
};

struct LayerChann : virtual Layer {
    uint64_t iChannCnt = 0,
             iElemCnt  = 0;

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        iElemCnt  = iInLnCnt * iInColCnt;
        iChannCnt = iInChannCnt;
    }
};

template <bool bPad1Crop0   = false,
          uint64_t iTop     = 0,
          uint64_t iRight   = 0,
          uint64_t iBottom  = 0,
          uint64_t iLeft    = 0,
          uint64_t iLnDist  = 0,
          uint64_t iColDist = 0>
struct LayerPC : LayerChann{
    kokkoro_set<uint64_t> setElemIdx;

    uint64_t iOutElemCnt = 0;

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        if constexpr (bPad1Crop0) setElemIdx = im2col_pad_in_idx(iInLnCnt, iInColCnt, iInLnCnt, iInColCnt, iInChannCnt, iTop, iRight, iBottom, iLeft, iLnDist, iColDist);
        else setElemIdx = im2col_crop_out_idx(iInLnCnt, iInColCnt, iInLnCnt, iInColCnt, iInChannCnt, iTop, iRight, iBottom, iLeft, iLnDist, iColDist);
        iOutElemCnt = iInLnCnt * iInColCnt;
    }

    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) { Deduce(vecIn); }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        kokkoro_matrix vecAns {iElemCnt, iChannCnt};
        for (auto i = 0ull; i < setElemIdx.length; ++i)
            if constexpr (bPad1Crop0) vecAns.index(i) = vecGrad.index(setElemIdx[i]);
            else vecAns.index(setElemIdx[i]) = vecGrad.index(i);
        vecGrad = std::move(vecAns);
    }

    virtual void Deduce(kokkoro_matrix &vecIn) {
        kokkoro_matrix vecAns {iOutElemCnt, iChannCnt};
        for (auto i = 0ull; i < setElemIdx.length; ++i)
            if constexpr (bPad1Crop0) vecAns.index(setElemIdx[i]) = vecIn.index(i);
            else vecAns.index(i) = vecIn.index(setElemIdx[i]);
        vecIn = std::move(vecAns);
    }

    virtual constexpr uint64_t LayerType() const { return kokkoro_pc; }
};

struct LayerFlat : LayerChann{
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        iInLnCnt    = iElemCnt * iChannCnt;
        iInChannCnt = 1;
        iInColCnt   = 1;
    }

    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) { Deduce(vecIn); }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) { vecGrad.reshape(iElemCnt, iChannCnt); }

    virtual void Deduce(kokkoro_matrix &vecIn) { vecIn.reshape(vecIn.element_count, 1); }

    virtual constexpr uint64_t LayerType() const { return kokkoro_flat; }
};

template <uint64_t iOutLnCnt = 1,
          double dLearnRate  = 0.,
          double dRandFstRng = -1.,
          double dRandSndRng = 1.,
          double dGradDecay  = 0.9>
struct LayerFC : LayerWeight<dLearnRate,
                             dGradDecay> {
    LayerFC(const std::string &sWeightFileSavePath = "", const std::string &sWeightFileLoadPath = "") : LayerWeight<dLearnRate, dGradDecay>(sWeightFileSavePath, sWeightFileLoadPath) {}

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        if (this->vecWeight.verify) return;
        this->vecWeight = FCInitWeight(iInLnCnt, iOutLnCnt, dRandFstRng, dRandSndRng);
        if constexpr (dLearnRate != 0) this->vecWeightN = this->vecWeight;
        this->vecWeightT = this->vecWeight.transpose;
        iInLnCnt         = iOutLnCnt;
    }
    
    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) {
        this->setIO[iBatSzIdx] = vecIn.transpose;
        if constexpr (dLearnRate == 0) Deduce(vecIn);
        else vecIn = FCOut(vecIn, this->vecWeightN);
    }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = FCGradWeight(vecGrad, this->setIO[iBatSzIdx]);
        vecGrad                = FCGradIn(vecGrad, this->vecWeightT);
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            this->iBatSzCnt.cnt = 0;
            this->Update<true>();
        }
    }

    virtual void Deduce(kokkoro_matrix &vecIn) { vecIn = FCOut(vecIn, this->vecWeight); }

    virtual constexpr uint64_t LayerType() const { return kokkoro_fc; }
};

template <uint64_t iFilterLnCnt  = 0,
          uint64_t iFilterColCnt = 0,
          uint64_t iLnStride     = 0,
          uint64_t iColStride    = 0,
          uint64_t iLnDilate     = 0,
          uint64_t iColDilate    = 0>
struct LayerCaffe : virtual LayerChann {
    uint64_t iCaffeLnCnt  = 0,
             iCaffeColCnt = 0;

    kokkoro_set<uint64_t> setCaffeIdx;

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        setCaffeIdx = CaffeIdx(iCaffeLnCnt, iCaffeColCnt, iInLnCnt, iInColCnt, iInLnCnt, iInColCnt, iChannCnt, iFilterLnCnt, iFilterColCnt, iFilterLnCnt * iFilterColCnt, iLnStride, iColStride, iLnDilate, iColDilate);
    }
};

template <uint64_t iKernelQty    = 0,
          uint64_t iKernelLnCnt  = 0,
          uint64_t iKernelColCnt = 0,
          uint64_t iLnStride     = 0,
          uint64_t iColStride    = 0,
          uint64_t iLnDilate     = 0,
          uint64_t iColDilate    = 0,
          double   dLearnRate    = 0.,
          double   dRandFstRng   = -1.,
          double   dRandSndRng   = 1.,
          double   dGradDecay    = .9>
struct LayerConv : LayerWeight<dLearnRate,
                               dGradDecay>,
                   LayerCaffe<iKernelLnCnt,
                              iKernelColCnt,
                              iLnStride,
                              iColStride,
                              iLnDilate,
                              iColDilate> {
    LayerConv(const std::string &sKernelFileSavePath = "", const std::string &sKernelFileLoadPath = "") : LayerWeight<dLearnRate, dGradDecay>(sKernelFileSavePath, sKernelFileLoadPath) {}
    
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        if (this->vecWeight.verify) return;
        this->vecWeight  = ConvInitKernel(iKernelQty, iInChannCnt, iKernelLnCnt, iKernelColCnt, dRandFstRng, dRandSndRng);
        this->vecWeightT = this->vecWeight.transpose;
        if constexpr (dLearnRate != 0) this->vecWeightN = this->vecWeight;
        LayerCaffe<iKernelLnCnt, iKernelColCnt, iLnStride, iColStride, iLnDilate, iColDilate>::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        iInChannCnt = iKernelQty;
    }
    
    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) {
        vecIn                  = Caffe(vecIn, this->setCaffeIdx, this->iCaffeLnCnt, this->iCaffeColCnt);
        this->setIO[iBatSzIdx] = vecIn.transpose;
        if constexpr (dLearnRate == 0) vecIn = Conv(vecIn, this->vecWeight);
        else vecIn = Conv(vecIn, this->vecWeightN);
    }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = ConvGradKernel(vecGrad, this->setIO[iBatSzIdx]);
        vecGrad                = CaffeGradIn(ConvGradCaffeOut(vecGrad, this->vecWeightT), this->setCaffeIdx, this->iElemCnt, this->iChannCnt);
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            this->iBatSzCnt.cnt = 0;
            this->Update<true>();
        }
    }

    virtual void Deduce(kokkoro_matrix &vecIn) { vecIn = Conv(Caffe(vecIn, this->setCaffeIdx, this->iCaffeLnCnt, this->iCaffeColCnt), this->vecWeight); }

    virtual constexpr uint64_t LayerType() const { return kokkoro_conv; }
};

template <uint64_t iPoolType     = kokkoro_avg_pool,
          uint64_t iFilterLnCnt  = 0,
          uint64_t iFilterColCnt = 0,
          uint64_t iLnStride     = 0,
          uint64_t iColStride    = 0,
          uint64_t iLnDilate     = 0,
          uint64_t iColDilate    = 0>
struct LayerPool : LayerCaffe<iFilterLnCnt,
                              iFilterColCnt,
                              iLnStride,
                              iColStride,
                              iLnDilate,
                              iColDilate> {
    static constexpr auto iFilterElemCnt = iFilterLnCnt * iFilterColCnt;

    kokkoro_set<kokkoro_set<kokkoro_set<uint64_t>>> setElemIdx;

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) { if constexpr (iPoolType == kokkoro_max_pool) setElemIdx.init(iBatSz, false); }

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        if constexpr (iPoolType == kokkoro_gag_pool) {
            LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
            iInLnCnt  = 1;
            iInColCnt = 1;
        } else LayerCaffe<iFilterLnCnt, iFilterColCnt, iLnStride, iColStride, iLnDilate, iColDilate>::Shape(iInLnCnt, iInColCnt, iInChannCnt);
    }
    
    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) {
        if constexpr (iPoolType == kokkoro_max_pool) vecIn = PoolMax(vecIn, this->setCaffeIdx, iFilterElemCnt, this->iCaffeLnCnt, setElemIdx[iBatSzIdx]);
        else Deduce(vecIn);
    }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        if constexpr (iPoolType == kokkoro_gag_pool) vecGrad = PoolGradGlbAvgIn(vecGrad, this->iElemCnt, this->iChannCnt);
        if constexpr (iPoolType == kokkoro_avg_pool) vecGrad = PoolGradAvgIn(vecGrad, this->setCaffeIdx, iFilterElemCnt, this->iElemCnt);
        if constexpr (iPoolType == kokkoro_max_pool) vecGrad = PoolGradMaxIn(vecGrad, this->iElemCnt, setElemIdx[iBatSzIdx]);
    }

    virtual void Deduce(kokkoro_matrix &vecIn) {
        if constexpr (iPoolType == kokkoro_gag_pool) vecIn = PoolGlbAvg(vecIn);
        if constexpr (iPoolType == kokkoro_avg_pool) vecIn = PoolAvg(vecIn, this->setCaffeIdx, iFilterElemCnt, this->iCaffeLnCnt);
        if constexpr (iPoolType == kokkoro_max_pool) vecIn = PoolMax(vecIn, this->setCaffeIdx, iFilterElemCnt, this->iCaffeLnCnt);
    }

    virtual constexpr uint64_t LayerType() const { return kokkoro_pool; }
};

template <double dShift          = 0.,
          double dScale          = 1.,
          double dShiftLearnRate = 0.,
          double dScaleLearnRate = 0.,
          double dShiftGradDecay = .9,
          double dScaleGradDecay = .9,
          double dMovAvgDecay    = .9>
struct LayerBN : LayerWeight<dShiftLearnRate,
                             dShiftGradDecay> {
    kokkoro_counter iBackBatSzCnt;

    kokkoro_matrix vecScale, vecScaleN;

    ada_delta<dScaleGradDecay> AdaDScale;

    ada_nesterov<dScaleLearnRate, dScaleGradDecay> AdaNScale;

    async_controller asyForCtrl, asyBackCtrl;

    BNData BdData;

    std::string sScaleSavePath {""};

    LayerBN(const std::string &sShiftFileSavePath = "", const std::string &sScaleFileSavePath = "", const std::string &sShiftFileLoadPath = "", const std::string &sScaleFileLoadPath = "") : LayerWeight<dShiftLearnRate, dShiftGradDecay>(sShiftFileSavePath, sShiftFileLoadPath),
        sScaleSavePath(sScaleFileSavePath) {
        if (!sScaleFileLoadPath.length()) return;
        auto tabScale = csv_in(sScaleFileLoadPath);
        vecScale      = {1, tabScale[0].length};
        for (auto i = 0ull; i < vecScale.element_count; ++i) vecScale.index(i) = std::stod(tabScale[0][i]);
    }

    const kokkoro_matrix &BNScaleRef() {
        if constexpr (dScaleLearnRate != 0) return vecScale;
        else return vecScaleN;
    }

    const kokkoro_matrix &BNShiftRef() {
        if constexpr (dShiftLearnRate != 0) return this->vecWeight;
        else return this->vecWeightN;
    }

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) {
        LayerIO::Batch(iBatSz, iBatCnt);
        BNDataInit(BdData, iBatSz, iBatCnt);
    }

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        if (this->vecWeight.verify && vecScale.verify) return;
        this->vecWeight = BNBetaGammaInit<dShift>(iInChannCnt);
        vecScale        = BNBetaGammaInit<dScale>(iInChannCnt);
        if constexpr (dScaleLearnRate != 0) vecScaleN = vecScale.transpose;
        if constexpr (dShiftLearnRate != 0) this->vecWeightN = this->vecWeight;
    }

    void Update() {
        if constexpr (dShiftLearnRate == 0) this->AdaD.update(this->vecWeight, this->vecWeightT);
        else this->AdaN.update(this->vecWeight, this->vecWeightN, this->vecWeightT);
        if constexpr (dScaleLearnRate == 0) AdaDScale.update(vecScale, vecScaleN);
        else AdaNScale.update(vecScale, vecScaleN, vecScaleN);
    }
    
    virtual void ForProp(kokkoro_matrix &vecIn, uint64_t iBatSzIdx) {
        this->setIO[iBatSzIdx] = std::move(vecIn);
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            BNOut(this->setIO, BdData, BNShiftRef(), BNScaleRef());
            this->iBatSzCnt.cnt = 0;
            asyForCtrl.thread_wake_all();
            BNMovAvg<dMovAvgDecay>(BdData);
        } else while (this->iBatSzCnt.cnt) asyForCtrl.thread_sleep(200);
        vecIn = std::move(this->setIO[iBatSzIdx]);
    }

    virtual void BackProp(kokkoro_matrix &vecGrad, uint64_t iBatSzIdx, kokkoro_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = std::move(vecGrad);
        if (++iBackBatSzCnt.cnt == this->setIO.length) {
            BNGradIn(this->setIO, BdData, this->vecWeightT, vecScaleN, BNScaleRef());
            iBackBatSzCnt.cnt = 0;
            asyBackCtrl.thread_wake_all();
            Update();
        } else while (iBackBatSzCnt.cnt) asyBackCtrl.thread_sleep(200);
        vecGrad = std::move(this->setIO[iBatSzIdx]);
    }

    virtual void Deduce(kokkoro_matrix &vecIn) { BNOut(vecIn, BdData, this->vecWeight, vecScale); }

    virtual bool SaveData() const {
        if (!LayerWeight<dShiftLearnRate, dShiftGradDecay>::SaveData()) return false;
        kokkoro_set<kokkoro_set<std::string>> tabScale(1);
        tabScale[0].init(vecScale.element_count);
        for (auto i = 0ull; i < vecScale.element_count; ++i) tabScale[0][i] = std::to_string(vecScale.index(i));
        return csv_out(tabScale, sScaleSavePath);
    }

    virtual constexpr uint64_t LayerType() const { return kokkoro_bn; }
};

KOKKORO_END