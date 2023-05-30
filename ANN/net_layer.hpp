NEUNET_BEGIN

struct Layer { virtual constexpr uint64_t LayerType() const { return neunet_layer_null; } };

template <double dLearnRate = 0., double dRandFstRng = -1., double dRandSndRng = 1.>
struct LayerWeight : virtual Layer {
    net_counter iBatSzCnt;

    double dLearnRate  = 0,
           dRandFstRng = 0,
           dRandSndRng = 0;

    net_matrix vecWeight, vecWeightT, vecWeightN;

    net_set<net_matrix> setWeightGrad;

    ada_delta AdaD;

    ada_nesterov AdaN;

    // call this function after weight initializing
    void Shape(uint64_t iBatSz, bool bWeightT) {
        setWeightGrad.init(iBatSz, false);
        if (dLearnRate) vecWeightN = vecWeight;
        if (bWeightT) vecWeightT = vecWeight.transpose;
    }

    void Update(bool bWeightT) {
        auto vecGrad = net_matrix::sigma(setWeightGrad);
        vecGrad.elem_wise_div(setWeightGrad.length);
        ada_update(vecGrad, vecWeight, vecWeightN, dLearnRate, AdaD, AdaN);
    }
};

template <double dLearnRate = 0., double dRandFstRng = -1., double dRandSndRng = 1.>
struct LayerBias final : LayerWeight<dLearnRate, dRandFstRng, dRandSndRng> {
    void Shape(uint64_t iInLnCnt, uint64_t iInColCnt, uint64_t iChannCnt, uint64_t iBatSz) {
        this->vecWeight = {iInLnCnt * iInColCnt, iChannCnt};
        vecWeight.rand_elem(dRandFstRng, dRandSndRng);
        LayerWeight::Shape(iBatSz, false);
    }

    void ForProp(net_matrix &vecIn) {
        if (dLearnRate) vecIn += this->vecWeightN;
        else Deduce(vecIn);
    }

    void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx) {
        this->setWeightGrad[iBatSzIdx] = vecGrad;
        if (++this->iBatSzCnt.cnt == this->setWeightGrad.length) {
            this->iBatSzCnt.cnt = 0;
            Update(false);
        }
    }

    void Deduce(net_matrix &vecIn) { vecIn += this->vecWeight; }

    virtual constexpr uint64_t LayerType() const { return neunet_layer_bias; }
};

struct LayerIn : virtual Layer {
    net_set<net_matrix> setIn;

    void Shape(uint64_t iBatSz) { setIn.init(iBatSz, false); }
};

template <uint64_t iActFnType = 0>
struct LayerAct : LayerIn {

    void Shape(uint64_t iBatSz) { if constexpr (iActFnType > neunet_softmax) LayerIn::Shape(iBatSz);  }

    void ForProp(net_matrix &vecIn, uint64_t iBatSzIdx) const {
        if constexpr (iActFnType > neunet_softmax) setIn[iBatSzIdx] = vecIn;
        Deduce(vecIn);
    }

    void BackProp(net_matrix &vecGrad, uint64_t iBatSzIdx, net_matrix &vecOrgn) const { switch(iFnType) {
        case neunet_sigmoid:
            neunet_traverse(setIn[iBatSzIdx], sigmoid_dv);
            vecGrad.elem_wise_mul(setIn[iBatSzIdx]);
            break;
        case neunet_ReLU:
            neunet_traverse(setIn[iBatSzIdx], ReLU_dv);
            vecGrad.elem_wise_mul(setIn[iBatSzIdx]);
        case neunet_softmax: softmax_cec_grad(vecGrad, vecOrgn); break;
        default: break;
    } }

    void Deduce(net_matrix &vecIn) const {
        switch(iFnType) {
        case neunet_sigmoid: neunet_traverse(vecIn, sigmoid); break;
        case neunet_ReLU: neunet_traverse(vecIn, ReLU); break;
        case neunet_softmax: softmax(vecIn); break;
        default: break;
        }
    }

    virtual constexpr uint64_t LayerType() const { return neunet_layer_act; }
};

struct LayerOut : Layer { uint64_t iOutLnCnt = 0; };

template <bool bPadMode = true, uint64_t iTop = 0, uint64_t iRight = 0, uint64_t iBottom = 0, uint64_t iLeft = 0, uint64_t iLnDist = 0, uint64_t iColDist = 0>
struct LayerPC final : LayerOut {
    uint64_t iInLnCnt   = 0,
             iInColCnt  = 0,
             iOutColCnt = 0;
    
    net_set<uint64_t> setPCIdx;
    
    void Shape(uint64_t &iInLnCnt, uint64_t &iInColCnt, uint64_t iChannCnt) {
        this->iInLnCnt  = iInLnCnt;
        this->iInColCnt = iInColCnt;
        if constexpr (bPadMode) setPCIdx = im2col_pad_idx(iOutLnCnt, iOutColCnt, iInLnCnt, iInColCnt, iChannCnt, iTop, iRight, iBottom, iLeft, iLnDist, iColDist);
        else setPCIdx = im2col_crop_idx(iOutLnCnt, iOutColCnt, iInLnCnt, iInColCnt, iChannCnt, iTop, iRight, iBottom, iLeft, iLnDist, iColDist);
        iInLnCnt  = iOutLnCnt;
        iInColCnt = iOutColCnt;
    }

    void ForProp(net_matrix &vecIn) const { Deduce(vecIn); }

    void BackProp(net_matrix &vecGrad) const {
        net_matrix vecAns {iInLnCnt, iInColCnt};
        for (auto i = 0ull; i < setPCIdx.length; ++i)
            if constexpr (bPadMode) { if (setPCIdx[i]) vecAns.index(setPCIdx[i] - 1) = vecGrad.index(i) }
            else vecAns.index(setPCIdx[i]) = vecGrad.index(i);
        vecGrad = std::move(vecAns);
    }

    void Deduce(net_matrix &vecIn) const {
        net_matrix vecAns {iOutLnCnt, iOutColCnt};
        for (auto i = 0ull; i < setPCIdx.length; ++i)
            if constexpr (bPadMode) { if (setPCIdx[i]) vecAns.index(i) = vecIn.index(setPCIdx[i] - 1); }
            else vecAns.index(i) = vecIn.index(setPCIdx[i]);
        vecIn = std::move(vecAns);
    }

    virtual constexpr uint64_t LayerType() const { return neunet_layer_pc; }
};

NEUNET_END