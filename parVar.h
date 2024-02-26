// #include <stdio.h>

// https://www.johndcook.com/blog/standard_deviation/ 
struct RunningStat {
    int m_n; 
    double m_oldM, m_newM, m_oldS, m_newS;
};

void RunningStat_clear(struct RunningStat *rs) 
{
    rs->m_n = 0;
}

double RunningStat_variance(struct RunningStat *rs); // 
void RunningStat_push(struct RunningStat *rs, short x) 
{
    rs->m_n++; 

    if(rs->m_n == 1) {
        rs->m_oldM = x;
        rs->m_newM = x; 
        rs->m_oldS = 0.0; 
    } else {
        rs->m_newM = rs->m_oldM + (x - rs->m_oldM) / rs->m_n;
        rs->m_newS = rs->m_oldS + (x - rs->m_oldM) * (x - rs->m_newM);

        rs->m_oldM = rs->m_newM; 
        rs->m_oldS = rs->m_newS;
    }

    // TEMP DEBUG STUFF
    // printf("m_n: %d, var: %f\n", rs->m_n, RunningStat_variance(rs));
}

double RunningStat_variance(struct RunningStat * rs) 
{
    // return ( (rs->m_n > 1) ? rs->m_newS/(rs->m_n - 1) : 0.0 ); // this is sample variance, we want population variance since we have all the data of the laplacian
    return ( (rs->m_n > 1) ? rs->m_newS/(rs->m_n) : 0.0);
}

void RunningStat_merge(struct RunningStat *self, struct RunningStat *other) 
{
    // https://github.com/a-mitani/welford/blob/main/welford/welford.py 

    int count = self->m_n + other->m_n; 
    double delta = self->m_newM - other->m_newM;
    double delta2 = delta * delta;

    double m = (self->m_n * self->m_newM + other->m_n * other->m_newM) / count;
    double s = self->m_newS + other->m_newS + delta2 * (self->m_n * other->m_n) / count;

    self->m_n = count; 
    self->m_newM = m; 
    self->m_newS = s;
}