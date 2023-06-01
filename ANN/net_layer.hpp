NEUNET_BEGIN

struct Layer {
    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) = 0;

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) = 0;

    virtual void Deduce(net_matrix &vecIn) = 0;

    virtual void Update() {}

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) {}

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {}

    virtual constexpr uint64_t LayerType() const = 0;
};

struct LayerIO : virtual Layer {
    net_set<net_matrix> setIO;

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) { setIO.init(iBatSz, false); }
};

template <double dLearnRate = 0., double dGradDecay = 0.9>
struct LayerWeight : virtual Layer {
    net_counter iBatSzCnt;

    net_matrix vecWeight, vecWeightT, vecWeightN;

    ada_delta<dGradDecay> AdaD;

    ada_nesterov<dLearnRate, dGradDecay> AdaN;

    virtual void Update() {
        auto vecGrad = net_matrix::sigma(setIO);
        vecGrad.elem_wise_div(setIO.length);
        if constexpr (dLearnRate) {
            AdaN.update(vecWeight, vecWeightN, vecGrad);
            vecWeightT = vecWeightN.transpose;
        } else {
            AdaD.update(vecWeight, vecGrad);
            vecWeightT = vecWeight.transpose;
        }
    }
};

template <double dLearnRate = 0., double dRandFstRng = -1., double dRandSndRng = 1., double dGradDecay = 0.9>
struct LayerBias : LayerWeight<dLearnRate, dGradDecay> {
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        this->vecWeight = {iInLnCnt * iInColCnt, iInChannCnt};
        this->vecWeight.elem_rand(dRandFstRng, dRandSndRng);
        if constexpr (dLearnRate) this->vecWeightN = this->vecWeight;
    }

    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) {
        if (this->dLearnRate) vecIn += this->vecWeightN;
        else Deduce(vecIn);
    }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = vecGrad;
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            this->iBatSzCnt.cnt = 0;
            this->Update();
        }
    }

    virtual void Deduce(net_matrix &vecIn) { vecIn += this->vecWeight; }

    virtual constexpr uint64_t LayerType() const { return neunet_bias; }
};

template <uint64_t iActFnType = neunet_null>
struct LayerAct : LayerIO {
    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) {
        if constexpr (iActFnType == neunet_sigmoid || iActFnType == neunet_ReLU) setIO[iBatSzIdx] = vecIn;
        Deduce(vecIn);
    }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
        constexpr auto fn_sigmoid = iActFnType == neunet_sigmoid,
                       fn_ReLU    = iActFnType == neunet_ReLU;
        if constexpr (fn_sigmoid || fn_ReLU) {
            if constexpr (fn_ReLU) neunet_traverse(setIO[iBatSzIdx], ReLU_dv);
            else neunet_traverse(setIO[iBatSzIdx], sigmoid_dv);
            vecGrad.elem_wise_mul(setIO[iBatSzIdx]);
        }
        if constexpr (iActFnType == neunet_softmax) softmax_cec_grad(vecGrad, vecOrgn);
    }

    virtual void Deduce(net_matrix &vecIn) {
        if constexpr (iActFnType == neunet_sigmoid) neunet_traverse(vecIn, sigmoid);
        else if constexpr (iActFnType == neunet_ReLU) neunet_traverse(vecIn, ReLU);
        else if constexpr (iActFnType == neunet_softmax) softmax(vecIn);
    }

    virtual constexpr uint64_t LayerType() const { return neunet_act; }
};

struct LayerChann : Layer {
    uint64_t iChannCnt = 0,
             iElemCnt  = 0;

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        iElemCnt  = iInLnCnt * iInColCnt;
        iChannCnt = iInChannCnt;
    }
};

template <bool bPad1Crop0 = false, uint64_t iTop = 0, uint64_t iRight = 0, uint64_t iBottom = 0, uint64_t iLeft = 0, uint64_t iLnDist = 0, uint64_t iColDist = 0>
struct LayerPC : LayerChann{
    net_set<uint64_t> setElemIdx;

    uint64_t iOutElemCnt = 0;

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        if constexpr (bPad1Crop0) setElemIdx = im2col_pad_in_idx(iInLnCnt, iInColCnt, iInLnCnt, iInColCnt, iInChannCnt, iTop, iRight, iBottom, iLeft, iLnDist, iColDist);
        else setElemIdx = im2col_crop_out_idx(iInLnCnt, iInColCnt, iInLnCnt, iInColCnt, iInChannCnt, iTop, iRight, iBottom, iLeft, iLnDist, iColDist);
        iOutElemCnt = iInLnCnt * iInColCnt;
    }

    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) { Deduce(vecIn); }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
        net_matrix vecAns {iElemCnt, iChannCnt};
        for (auto i = 0ull; i < setElemIdx.length; ++i)
            if constexpr (bPad1Crop0) vecIn.index(i) = vecAns.index(setElemIdx[i]);
            else vecIn.index(setElemIdx[i]) = vecAns.index(i);
        vecGrad = std::move(vecAns);
    }

    virtual void Deduce(net_matrix &vecIn) {
        net_matrix vecAns {iOutElemCnt, iChannCnt};
        for (auto i = 0ull; i < setElemIdx.length; ++i)
            if constexpr (bPad1Crop0) vecAns.index(setElemIdx[i]) = vecIn.index(i);
            else vecAns.index(i) = vecIn.index(setElemIdx[i]);
        vecIn = std::move(vecAns);
    }

    virtual constexpr uint64_t LayerType() const { return neunet_pc; }
};

struct LayerFlat : LayerChann{
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        iInLnCnt    = iElemCnt * iChannCnt;
        iInChannCnt = 1;
        iInColCnt   = 1;
    }

    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) { Deduce(vecIn); }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) { vecGrad.reshape(iElemCnt, iChannCnt); }

    virtual void Deduce(net_matrix &vecIn) { vecIn.reshape(vecIn.element_count, 1); }

    virtual constexpr uint64_t LayerType() const { return neunet_flat; }
};

template <uint64_t iOutLnCnt = 1, double dLearnRate = 0., double dRandFstRng = -1., double dRandSndRng = 1., double dGradDecay = 0.9>
struct LayerFC : LayerWeight<dLearnRate, dGradDecay> {    
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        this->vecWeight = FCInitWeight(iInLnCnt, iOutLnCnt, dRandFstRng, dRandSndRng);
        if constexpr (dLearnRate) this->vecWeightN = this->vecWeight;
        this->vecWeightT = this->vecWeight.transpose;
    }
    
    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) {
        this->setIO[iBatSzIdx] = vecIn.transpose;
        if constexpr (dLearnRate) vecIn = FCOut(vecIn, this->vecWeightN);
        else Deduce(vecIn);
    }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = FCGradWeight(vecGrad, this->setIO[iBatSzIdx]);
        vecGrad                = FCGradIn(vecGrad, this->vecWeightT);
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            this->iBatSzCnt.cnt = 0;
            LayerWeight<dLearnRate, dGradDecay>::Update();
        }
    }

    virtual void Deduce(net_matrix &vecIn) { vecIn = FCOut(vecIn, this->vecWeight); }

    virtual constexpr uint64_t LayerType() const { return neunet_fc; }
};

template <uint64_t iFilterLnCnt = 0, uint64_t iFilterColCnt = 0, uint64_t iLnStride = 0, uint64_t iColStride = 0, uint64_t iLnDilate = 0, uint64_t iColDilate = 0>
struct LayerCaffe : virtual LayerChann {
    uint64_t iCaffeLnCnt  = 0,
             iCaffeColCnt = 0;

    net_set<uint64_t> setCaffeIdx;

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        setCaffeIdx = CaffeIdx(iCaffeLnCnt, iCaffeColCnt, iInLnCnt, iInColCnt, iInLnCnt, iInColCnt, iChannCnt, iFilterLnCnt, iFilterColCnt, iFilterLnCnt * iFilterColCnt, iLnStride, iColStride, iLnDilate, iColDilate);
    }
};

template <uint64_t iKernelQty = 0, uint64_t iKernelLnCnt = 0, uint64_t iKernelColCnt = 0, uint64_t iLnStride = 0, uint64_t iColStride = 0, uint64_t iLnDilate = 0, uint64_t iColDilate = 0, double dLearnRate = 0., double dRandFstRng = -1., double dRandSndRng = 1., double dGradDecay = .9>
struct LayerConv : LayerWeight<dLearnRate, dGradDecay>, LayerCaffe<iKernelLnCnt, iKernelColCnt, iLnStride, iColStride, iLnDilate, iColDilate> {
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        this->vecWeight  = ConvInitKernel(iKernelQty, iInChannCnt, iKernelLnCnt, iKernelColCnt, dRandFstRng, dRandSndRng);
        this->vecWeightT = this->vecWeight.transpose;
        if constexpr (dLearnRate) this->vecWeightN = this->vecWeight;
        LayerCaffe<iKernelLnCnt, iKernelColCnt, iLnStride, iColStride, iLnDilate, iColDilate>::Shape(iInLnCnt, iInColCnt, iInChannCnt);
        iInChannCnt = iKernelQty;
    }
    
    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) {
        vecIn                  = Caffe(vecIn, this->setCaffeIdx, this->iCaffeLnCnt, this->iCaffeColCnt);
        this->setIO[iBatSzIdx] = vecIn.transpose;
        if constexpr (dLearnRate) vecIn = Conv(vecIn, this->vecWeightN);
        else vecIn = Conv(vecIn, this->vecWeight);
    }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
        this->setIO[iBatSzIdx] = ConvGradKernel(vecGrad, this->setIO[iBatSzIdx]);
        vecGrad                = CaffeGradIn(ConvGradCaffeOut(vecGrad, this->vecWeightT), this->setCaffeIdx, this->iElemCnt, this->iChannCnt);
        if (++this->iBatSzCnt.cnt == this->setIO.length) {
            this->iBatSzCnt.cnt = 0;
            LayerWeight<dLearnRate, dGradDecay>::Update();
        }
    }

    virtual void Deduce(net_matrix &vecIn) { vecIn = Conv(Caffe(vecIn, this->setCaffeIdx, this->iCaffeLnCnt, this->iCaffeColCnt), this->vecWeight); }

    virtual constexpr uint64_t LayerType() const { return neunet_conv; }
};

template <uint64_t iPoolType = neunet_avg_pool, uint64_t iFilterLnCnt = 0, uint64_t iFilterColCnt = 0, uint64_t iLnStride = 0, uint64_t iColStride = 0, uint64_t iLnDilate = 0, uint64_t iColDilate = 0>
struct LayerPool : LayerCaffe<iFilterLnCnt, iFilterColCnt, iLnStride, iColStride, iLnDilate, iColDilate> {
    static constexpr auto iFilterElemCnt = iFilterLnCnt * iFilterColCnt;

    net_set<net_set<net_set<uint64_t>>> setElemIdx;

    virtual void Batch(uint64_t iBatSz, uint64_t iBatCnt) { if constexpr (iPoolType == neunet_max_pool) setElemIdx.init(iBatSz, false); }

    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {
        if constexpr (iPoolType == neunet_gag_pool) {
            LayerChann::Shape(iInLnCnt, iInColCnt, iInChannCnt);
            iInLnCnt  = 1;
            iInColCnt = 1;
        } else LayerCaffe<iKernelLnCnt, iKernelColCnt, iLnStride, iColStride, iLnDilate, iColDilate>::Shape(iInLnCnt, iInColCnt, iInChannCnt);
    }
    
    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) {
        if constexpr (iPoolType == neunet_max_pool) vecIn = PoolMax(vecIn, this->setCaffeIdx, iFilterElemCnt, this->iCaffeLnCnt, setElemIdx[iBatSzIdx]);
        else Deduce(vecIn);
    }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
        if constexpr (iPoolType == neunet_gag_pool) vecGrad = PoolGradGlbAvgIn(vecGrad, this->iElemCnt, this->iChannCnt);
        else if constexpr (iPoolType == neunet_avg_pool) vecIn = PoolGradAvgIn(vecGrad, this->setCaffeIdx, iFilterElemCnt, this->iElemCnt);
        else if constexpr (iPoolType == neunet_max_pool) vecIn = PoolGradMaxIn(vecGrad, this->iElemCnt, setElemIdx[iBatSzIdx]);
    }

    virtual void Deduce(net_matrix &vecIn) {
        if constexpr (iPoolType == neunet_gag_pool) vecIn = PoolGlbAvg(vecIn);
        else if constexpr (iPoolType == neunet_avg_pool) vecIn = PoolAvg(vecIn, this->setCaffeIdx, iFilterElemCnt, this->iCaffeLnCnt);
        else if constexpr (iPoolType == neunet_max_pool) vecIn = PoolMax(vecIn, this->setCaffeIdx, iFilterElemCnt, this->iCaffeLnCnt);
    }

    virtual constexpr uint64_t LayerType() const { return neunet_pool; }
};

struct LayerBN {
    virtual void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t &iInChannCnt) {}
    
    virtual void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) {
    }

    virtual void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) {
    }

    virtual void Deduce(net_matrix &vecIn) {
    }

    virtual constexpr uint64_t LayerType() const { return neunet_bn; }
};

NEUNET_END