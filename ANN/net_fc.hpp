NEUNET_BEGIN

net_matrix FCInitWeight(uint64_t iInLnCnt, uint64_t iOutLnCnt, double dFstRng = -1, double dSndRng = 1) {
    net_matrix vecAns {iOutLnCnt, iInLnCnt};
    vecAns.rand_elem(dFstRng, dSndRng);
    return vecAns;
}

NEUNET_END