NEUNET_BEGIN

double sigmoid(double src) { return 1 / (1 + 1 / std::exp(src)); }

double sigmoid_dv(double src) { { return sigmoid(src) * (1 - sigmoid(src)); } }

double ReLU(double src) { return src < 0 ? 0 : src; }

double ReLU_dv(double src) { return src < 0 ? 0 : 1; }

double AReLU(double src) { return src < 1 ? 0 : src; }

double AReLU_dv(double src) { return src < 1 ? 0 : 1; }

NEUNET_END