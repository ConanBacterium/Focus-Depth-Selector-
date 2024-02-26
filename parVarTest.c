#include "parVar.h"
#include <stdio.h>

double var_normal(short *X, int length) {
    //welfords online algorithm
    double mean = 0.0;
    double M2 = 0.0;
    double delta;

    for (int i = 0; i < length; i++) {
        double x = (double)X[i];
        delta = x - mean;
        mean += delta / (i + 1);
        M2 += delta * (x - mean);
    }

    return M2 / length;
}


int main(int argc, void **argv) 
{
    short vals[] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 12, 12, 15, 43, 52, 86, 94, 94, 105, 200, 254};
    double expectedVar = 4847.94;

    struct RunningStat rs1; 
    RunningStat_clear(&rs1);

    for(int i = 0; i < 20; i++) {
        // printf("%d, ", vals[i]);
        RunningStat_push(&rs1, vals[i]);
    }
    printf("rs1->m_n: %d\n", rs1.m_n);
    
    double var = RunningStat_variance(&rs1);

    double tmp = var_normal(vals, 20);
    printf("got %f, expected %f, welfords online %f\n", var, expectedVar, tmp);

    struct RunningStat rs2; 
    RunningStat_clear(&rs2); 
    RunningStat_clear(&rs1);

    for(int i = 0; i < 20; i++) {
        if (i % 2 == 0) 
            RunningStat_push(&rs1, vals[i]);
        else 
            RunningStat_push(&rs2, vals[i]);
    }

    RunningStat_merge(&rs1, &rs2);
    printf("parallel, got %f, expected %f\n", var, expectedVar);
}
