KOKKORO_BEGIN

kokkoro_matrix FCInitWeight(uint64_t iInLnCnt, uint64_t iOutLnCnt, double dFstRng = -1, double dSndRng = 1) {
    kokkoro_matrix vecAns {iOutLnCnt, iInLnCnt};
    vecAns.elem_rand(dFstRng, dSndRng);
    return vecAns;
}

kokkoro_matrix FCOut(const kokkoro_matrix &vecIn, const kokkoro_matrix &vecWeight) { return vecWeight * vecIn; }

kokkoro_matrix FCGradIn(const kokkoro_matrix &vecGradOut, const kokkoro_matrix &vecWeightT) { return vecWeightT * vecGradOut; }

kokkoro_matrix FCGradWeight(const kokkoro_matrix &vecGradOut, const kokkoro_matrix &vecInT) { return vecGradOut * vecInT; }

KOKKORO_END