NEUNET_BEGIN

net_matrix FCInitWeight(uint64_t iInLnCnt, uint64_t iOutLnCnt, double dFstRng = -1, double dSndRng = 1) {
    net_matrix vecAns {iOutLnCnt, iInLnCnt};
    vecAns.elem_rand(dFstRng, dSndRng);
    return vecAns;
}

net_matrix FCOut(const net_matrix &vecIn, const net_matrix &vecWeight) { return vecWeight * vecIn; }

net_matrix FCGradIn(const net_matrix &vecGradOut, const net_matrix &vecWeightT) { return vecWeightT * vecGradOut; }

net_matrix FCGradWeight(const net_matrix &vecGradOut, const net_matrix &vecInT) { return vecGradOut * vecInT; }

NEUNET_END