#include <stdio.h>
#include <cmath>

#define C 1500

double pinger[3];
double hydrophones[8][3];

int peaktime[8];
int tdoa[7];

double residual[7];

double normdiffs[8];

double jacobian[3];

void meastdoa() {
    for (int i = 0;i <= 6;i++) {
        tdoa[i] = peaktime[0] - peaktime[i + 1];
    }
}

void calcnormdiffs() {
    for (int i = 0; i <= 7; i++) {
        double dx = pinger[0] - hydrophones[i][0];
        double dy = pinger[1] - hydrophones[i][1];
        double dz = pinger[2] - hydrophones[i][2];

        normdiffs[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
}

void calcresidual() {
    for (int i = 0;i <= 6;i++) {
        residual[i] = tdoa[i] - (normdiffs[i + 1] - normdiffs[0]) / C;
    }
}

void costfunction() {

}

int main() {

}