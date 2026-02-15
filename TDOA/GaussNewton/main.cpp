#include <stdio.h>
#include <cmath>

#define C 1500.0

double pinger[3];
double hydrophone[8][3];

double peaktime[8];
double tdoa[7];

double residual[7];

double normdiffs[8];

double costfunction_gradient[3];

void meastdoa() {
    for (int i = 0;i <= 6;i++) {
        tdoa[i] = peaktime[0] - peaktime[i + 1];
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

void costfunction() {
    costfunction_gradient[0] = 0.0;
    costfunction_gradient[1] = 0.0;
    costfunction_gradient[2] = 0.0;

    double buffx = ((pinger[0] - hydrophone[0][0]) / normdiffs[0]);
    double buffy = ((pinger[1] - hydrophone[0][1]) / normdiffs[0]);
    double buffz = ((pinger[2] - hydrophone[0][2]) / normdiffs[0]);

    for (int i = 0;i <= 6;i++) {
        costfunction_gradient[0] += (-1.0 / C) * residual[i] *
            (((pinger[0] - hydrophone[i + 1][0]) / normdiffs[i + 1]) - buffx);    //possible divide by zero???
        costfunction_gradient[1] += (-1.0 / C) * residual[i] *
            (((pinger[1] - hydrophone[i + 1][1]) / normdiffs[i + 1]) - buffy);
        costfunction_gradient[2] += (-1.0 / C) * residual[i] *
            (((pinger[2] - hydrophone[i + 1][2]) / normdiffs[i + 1]) - buffz);
    }
}

int main() {

}