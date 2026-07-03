#include <stdio.h>
#include <cmath>

#define C 1500.0

double pinger[3];
double hydrophone[8][3];

double peaktime[8];
double tdoa[7];

double residual[7];

double normdiffs[8];

double jacobian_residual[7][3];

void meastdoa() {
    for (int i = 0;i <= 6;i++) {
        tdoa[i] = peaktime[0] - peaktime[i + 1]
    }
}

void calcnormdiffs() {
    for (int i = 0; i <= 7; i++) {
        double dx = pinger[0] - hydrophone[i][0];
        double dy = pinger[1] - hydrophone[i][1];
        double dz = pinger[2] - hydrophone[i][2];

        normdiffs[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
}

void calcresidual() {
    for (int i = 0;i <= 6;i++) {
        residual[i] = tdoa[i] - (normdiffs[i + 1] - normdiffs[0]) / C;
    }
}

void delta_pinger() {
    double costfunction_gradient[3] = { 0.0 };
    double costfunction_hessian[3][3] = { 0.0 };
    double costfunction_hessian_inv[3][3] = { 0.0 };

    double buffx = ((pinger[0] - hydrophone[0][0]) / normdiffs[0]);
    double buffy = ((pinger[1] - hydrophone[0][1]) / normdiffs[0]);
    double buffz = ((pinger[2] - hydrophone[0][2]) / normdiffs[0]);

    for (int i = 0;i <= 6;i++) {
        jacobian_residual[i][0] = (-1.0 / C) * (((pinger[0] - hydrophone[i + 1][0]) / normdiffs[i + 1]) - buffx);    //possible divide by zero???
        jacobian_residual[i][1] = (-1.0 / C) * (((pinger[1] - hydrophone[i + 1][1]) / normdiffs[i + 1]) - buffy);
        jacobian_residual[i][2] = (-1.0 / C) * (((pinger[2] - hydrophone[i + 1][2]) / normdiffs[i + 1]) - buffz);

        costfunction_gradient[0] += jacobian_residual[i][0] * residual[i];
        costfunction_gradient[1] += jacobian_residual[i][1] * residual[i];
        costfunction_gradient[2] += jacobian_residual[i][2] * residual[i];
    }

    for (int i = 0; i <= 6; i++) {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                costfunction_hessian[r][c] += jacobian_residual[i][r] * jacobian_residual[i][c];
            }
        }
    }

    double lambda = 1e-6;           //damping
    costfunction_hessian[0][0] += lambda;
    costfunction_hessian[1][1] += lambda;
    costfunction_hessian[2][2] += lambda;

}

int main() {

}