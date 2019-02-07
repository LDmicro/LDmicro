#include "ldmicro.h"
#include "accel.h"
double Proportional(double k, double n)
{
    return k * n;
}

double eFv(double m, double v)
{
    return m * v * v / 2.0;
}

//k=const ===================================================================
double stProp(double k, double t)
{
    return k * t * t / 2.0;
}

double tsProp(double k, double s)
{
    return sqrt(2.0 * s / k);
}

double vProp(double k, double t)
{
    return k * t;
}

double aProp(double k, double t)
{
    return k;
}

double kProp(int nSize)
{
    return 1.0 / (2.0 * nSize);
}
//v=k*sqrt(t) ===============================================================
double tsSqrt2(double k, double s)
{
    return pow(3.0 * s / (2.0 * k), 2.0 / 3.0);
}

double stSqrt2(double k, double t)
{
    return k * pow(t, 3.0 / 2.0) / (3.0 / 2.0);
}

double vSqrt2(double k, double t)
{
    return k * sqrt(t);
}

double aSqrt2(double k, double t)
{
    return k / (2.0 * sqrt(t));
}

double kSqrt2(int nSize)
{
    return pow(1.0 / pow(3.0 * nSize / 2.0, 2.0 / 3.0), 3.0 / 4.0);
}
//v=k*Sqrt3(t) ==============================================================
double tsSqrt3(double k, double s)
{
    return pow(4.0 * s / (3.0 * k), 3.0 / 4.0);
}

double stSqrt3(double k, double t)
{
    return k * pow(t, 4.0 / 3.0) / (4.0 / 3.0);
}

double vSqrt3(double k, double t)
{
    return k * pow(t, 1.0 / 3.0);
}

double aSqrt3(double k, double t)
{
    return k / (3.0 * pow(t, 2.0 / 3.0));
}

double kSqrt3(int nSize)
{
    return pow(1.0 / pow(4.0 * nSize / 3.0, 3.0 / 4.0), 4.0 / 9.0);
}
//v=k*t^2 ===================================================================
double ts2(double k, double s)
{
    return pow(3.0 * s / k, 1.0 / 3.0);
}

double st2(double k, double t)
{
    return k * t * t * t / 3.0;
}

double v2(double k, double t)
{
    return k * t * t;
}

double a2(double k, double t)
{
    return k * 2.0 * t;
}

double k2(int nSize)
{
    return pow(1.0 / pow(3.0 * nSize, 1.0 / 3.0), 6.0);
}
//v=2*k*t^2 v=-2*k*(t-tz)^2==================================================
double tsS(double k, double s)
{
    return pow(3.0 * s / k, 1.0 / 3.0);
}

double stS(double k, double t)
{
    return k * t * t * t / 3.0;
}

double vS(double k, double t)
{
    return k * t * t;
}

double aS(double k, double t)
{
    return k * 2.0 * t;
}
double ktS(int nSize)
{
    return 0.5 / pow(1.0 * nSize / 2.0, 2.0);
}
double ksS(int nSize)
{
    return 0.5 / pow(1.0 * (nSize * 6) / 2.0, 2.0);
    ;
}
//===========================================================================

void makeAccelTable(FILE *f, int max, int P, int nSize, ElemAccel **TT, char *name,
                    int  nN,  // (1-для несимметричных кривых разгонов, 2-для cимметричных S-образных кривых разгона)
                    int  sFt, // (1-sFt, 0-tFs)
                    int *n, int *Psum,
                    int *shrt, // mult = 2 ^ shrt
                    int *sovElement, fxFunc *fs, double ks, fxFunc *ft, double kt, fxFunc *fv, double kv, fxFunc *fa,
                    double ka, fxFunc *eFv, double m)
{
    ElemAccel *T = nullptr;
    int        v1 = 10;
    if(max - nSize * 2 > 0)
        v1 = max - nSize * 2;

    T = (ElemAccel *)CheckMalloc((1 + nSize + v1 + nSize + 1) * sizeof(ElemAccel)); //+1+1 Ok
    //need !!! CheckFree(r.T);
    if(!T) {
        Error(
            "Can not alocate memory for acceleration/deceleration table.\n"
            "Reduce size.");
        return;
    }
    *TT = T;

    int i, j;
    //LOG("acceleration")
    for(i = 1; i <= nSize; i++) {
        if(sFt) {
            T[i].t = ft(kt, i);
            T[i].s = fs(ks, T[i].t);
        } else {
            T[i].s = fs(ks, i);
            T[i].t = ft(kt, T[i].s);
        }

        T[i].ds = T[i].s - T[i - 1].s;
        T[i].dt = T[i].t - T[i - 1].t;

        T[i].v = fv(kv, T[i].t);
        T[i].a = fa(ka, T[i].t);
        T[i].e = eFv(m, T[i].v);

        T[i].si = int(round(T[i].s));
        T[i].dsi = int(round(T[i].ds));

        T[i].ti = int(round(T[i].t));
        T[i].dti = int(round(T[i].dt));

        T[i].tdiscrete = T[i].dti + T[i - 1].tdiscrete;
        if(T[i].dti) {
            T[i].vdiscrete = 1.0 * T[i].dsi / T[i].dti; // variant 1
                                                        //      T[i].vdiscrete = fv(kv, T[i].tdiscrete); // variant 2
            //      T[i].vdiscrete = 1.0 * (T[i].dsi+T[i+1].dsi) / (T[i].dti+T[i+1].dti); // variant 3
        }
    }
    if(nN == 2) {
        j = nSize / 2 /* + nSize%2*/;

        for(i = 1 + nSize / 2 /* + nSize%2*/; i <= nSize; i++) {
            T[i].ds = T[j].ds;
            T[i].s = T[i].ds + T[i - 1].s; //aaa

            T[i].dt = T[j].dt;
            T[i].t = T[i].dt + T[i - 1].t; //aaa

            T[i].v = 1.0 - T[j - 1].v;
            T[i].a = (T[i].v - T[i - 1].v) / T[i].dt; // aaa
            T[i].e = eFv(m, T[i].v);

            T[i].si = int(round(T[i].s));
            T[i].dsi = int(round(T[i].ds));

            T[i].ti = int(round(T[i].t));
            T[i].dti = int(round(T[i].dt));

            T[i].tdiscrete = T[i].dti + T[i - 1].tdiscrete;
            T[i].vdiscrete = 1.0 - T[j - 1].vdiscrete;
            j--;
        }
    }
    //LOG("v = 1")
    for(i = (1 + nSize); i <= (nSize + v1); i++) {
        T[i].ds = 1;
        T[i].s = T[i].ds + T[i - 1].s;

        T[i].dt = 1;
        T[i].t = T[i].dt + T[i - 1].t;

        T[i].v = T[i - 1].v;
        T[i].a = 0; //T[i-1].a;
        T[i].e = T[i - 1].e;

        T[i].si = int(round(T[i].s));
        T[i].dsi = int(round(T[i].ds));

        T[i].ti = int(round(T[i].t));
        T[i].dti = int(round(T[i].dt));

        T[i].tdiscrete = T[i].dti + T[i - 1].tdiscrete;
        T[i].vdiscrete = T[i - 1].vdiscrete;
    }
    //LOG("deceleration")
    j = nSize;
    for(i = (1 + nSize + v1); i <= (1 + nSize + v1 + nSize); i++) {
        T[i].ds = T[j].ds;
        ;
        T[i].s = T[i].ds + T[i - 1].s;

        T[i].dt = T[j].dt;
        T[i].t = T[i].dt + T[i - 1].t;

        T[i].si = int(round(T[i].s));
        T[i].dsi = int(round(T[i].ds));

        T[i].ti = int(round(T[i].t));
        T[i].dti = int(round(T[i].dt));

        T[i].tdiscrete = T[i].dti + T[i - 1].tdiscrete;
        if((j - 1) >= 0) {
            T[i].vdiscrete = T[j - 1].vdiscrete;

            //dbp("i=%d j=%d %d", i, j, j-1);
            T[i].v = T[j - 1].v;
            T[i].a = -T[j - 1].a;
            T[i].e = T[j - 1].e;
        }
        j--;
    }
    //LOG("calculation")
    for(i = 1; i <= (1 + nSize + v1); i++) {
        T[i].dv = T[i].v - T[i - 1].v;
        T[i].da = T[i].a - T[i - 1].a;
        T[i].de = T[i].e - T[i - 1].e;
    }
    for(i = 1 + nSize + v1; i <= (1 + nSize + v1 + nSize); i++) {
        T[i - 1].dv = T[i].v - T[i - 1].v;
        T[i - 1].da = T[i].a - T[i - 1].a;
        T[i - 1].de = T[i].e - T[i - 1].e;
    }

    double dtMax = -1.0;
    for(i = nSize + 1; i >= 1; i--) {
        if(dtMax < T[i].dt)
            dtMax = T[i].dt;
    }

    int mult = 1; // множитель dti до 128
    *shrt = 0;    // mult = 2 ^ shrt
    if(P < 1) {
        P = 1;
        if(dtMax > 128 - 1) {
            //      oops();
        } else if(dtMax > 64 - 1) {
            mult = 1;
            *shrt = 0;
        } else if(dtMax > 32 - 1) {
            mult = 2;
            *shrt = 1;
        } else if(dtMax > 16 - 1) {
            mult = 4;
            *shrt = 2;
        } else if(dtMax > 8 - 1) {
            mult = 8;
            *shrt = 3;
        } else if(dtMax > 4 - 1) {
            mult = 16;
            *shrt = 4;
        } else if(dtMax > 2 - 1) {
            mult = 32;
            *shrt = 5;
        }
    };

    int dtMulMax = 0;
    for(i = 1; i <= nSize; i++) {
        T[i].dtMul = (SDWORD)round(T[i].dt * mult * P);
        T[i].dtShr = T[i].dtMul >> (*shrt);
        if(dtMulMax < T[i].dtMul) {
            dtMulMax = T[i].dtMul;
        }
    }
    *sovElement = byteNeeded(dtMulMax);

    int T_nSize_dtMul = T[nSize].dtMul;
    for(i = nSize; i >= 1; i--) {
        if(T[i].dtMul == T_nSize_dtMul)
            *n = i + 1;
        else
            break;
    }

    *Psum = 0;
    for(i = 1; i < std::min(*n, max / 2); i++) {
        *Psum = *Psum + T[i].dtMul;
    }

    //LOG("2. v = 1")
    for(i = (1 + nSize); i <= (nSize + v1); i++) {
        T[i].dtMul = T[i].dti * mult * P;
    }

    //LOG("2. deceleration")
    j = nSize;
    for(i = (1 + nSize + v1); i <= (1 + nSize + v1 + nSize); i++) {
        T[i].dtMul = T[j].dtMul;
        j--;
    }
    if(f) {
        fprintf(f, " %s\n", name);
        fprintf(f,
                " max=%d nSize=%d *n=%d P=%d *Psum=%d *shrt=%d *sovElement=%d nN=%d kv=%f\n",
                max,
                nSize,
                *n,
                P,
                *Psum,
                *shrt,
                *sovElement,
                nN,
                kv);
        fprintf(f,
                "     %d       %d    %d   %d       %d       %d             %d    %d    %f\n",
                max,
                nSize,
                *n,
                P,
                *Psum,
                *shrt,
                *sovElement,
                nN,
                kv);
        fprintf(f, " \n");
        fprintf(f, " \n");
        fprintf(f, " \n");
        fprintf(f, " \n");
        fprintf(f, " \n");
        fprintf(f,
                " %5s %10s %5s %10s %5s %10s %5s %10s %5s %5s %10s %10s %10s %10s %10s %10s %5s %10s %5s \n",
                "i",
                "s",
                "si",
                "ds",
                "dsi",
                "t",
                "ti",
                "dt",
                "dti",
                "dtMul",
                "v",
                "dv",
                "a",
                "da",
                "e",
                "de",
                "tdisc",
                "vdiscrete",
                "dtShr");

        for(i = 0; i <= (nSize + v1 + nSize + 1); i++) {
            fprintf(f,
                    " %5d %10f %5d %10f %5d %10f %5d %10f %5d %5d %10f %10f %10f %10f %10f %10f %5d %10f %5d \n",
                    i,
                    T[i].s,
                    T[i].si,
                    T[i].ds,
                    T[i].dsi,
                    T[i].t,
                    T[i].ti,
                    T[i].dt,
                    T[i].dti,
                    T[i].dtMul,
                    T[i].v,
                    T[i].dv,
                    T[i].a,
                    T[i].da,
                    T[i].e,
                    T[i].de,
                    T[i].tdiscrete,
                    T[i].vdiscrete,
                    T[i].dtShr);
        }
    }
}

void CalcSteps(ElemStepper *s, ResSteps *r)
{
    memset(&(*r), 0, sizeof(ResSteps));

    FILE *f;

    double massa = 1;
    int    nSize = s->nSize;
    int    graph = s->graph;

    int P = 0;
    if(IsNumber(s->P)) {
        P = hobatoi(s->P);
    };

    int max = 0;
    if(IsNumber(s->max)) {
        max = hobatoi(s->max);
    };

    char fname[MAX_PATH];
    sprintf(fname, "%s\\%s", CurrentLdPath, "acceleration_deceleration.txt");
    f = fopen(fname, "w");

    double k;
    if(graph == 1) {
        k = kProp(nSize);
        makeAccelTable(f,
                       max,
                       P,
                       nSize,
                       &r->T,
                       "v=k*t  a=const  s=k*t^2/2  e=m*v^2/2",
                       1,
                       0,
                       &r->n,
                       &r->Psum,
                       &r->shrt,
                       &r->sovElement,
                       &Proportional,
                       1,
                       &tsProp,
                       k,
                       &vProp,
                       k,
                       &aProp,
                       k,
                       &eFv,
                       massa);
    } else if(graph == 2) {
        k = kSqrt2(nSize);
        makeAccelTable(f,
                       max,
                       P,
                       nSize,
                       &r->T,
                       "v=k*sqrt(t)  a=k/(2*t^(1/2))  s=k*t^(3/2)/(3/2)  e=m*v^2/2",
                       1,
                       0,
                       &r->n,
                       &r->Psum,
                       &r->shrt,
                       &r->sovElement,
                       &Proportional,
                       1,
                       &tsSqrt2,
                       k,
                       &vSqrt2,
                       k,
                       &aSqrt2,
                       k,
                       &eFv,
                       massa);
    } else if(graph == 3) {
        k = kSqrt3(nSize);
        makeAccelTable(f,
                       max,
                       P,
                       nSize,
                       &r->T,
                       "v=k*t^(1/3)  a=k/(3*t^(2/3))  s=k*t^(4/3)/(4/3)  e=m*v^2/2",
                       1,
                       0,
                       &r->n,
                       &r->Psum,
                       &r->shrt,
                       &r->sovElement,
                       &Proportional,
                       1,
                       &tsSqrt3,
                       k,
                       &vSqrt3,
                       k,
                       &aSqrt3,
                       k,
                       &eFv,
                       massa);
    } else if(graph == 4) {
        k = k2(nSize);
        makeAccelTable(f,
                       max,
                       P,
                       nSize,
                       &r->T,
                       "v=k*t^2  a=k*2*t  s=k*t^3/3  e=m*v^2/2",
                       1,
                       0,
                       &r->n,
                       &r->Psum,
                       &r->shrt,
                       &r->sovElement,
                       &Proportional,
                       1,
                       &ts2,
                       k,
                       &v2,
                       k,
                       &a2,
                       k,
                       &eFv,
                       massa);
    } else if(graph == 5) {
        k = ksS(nSize);
        makeAccelTable(f,
                       max,
                       P,
                       nSize,
                       &r->T,
                       "v=k*t^2  a=k*2*t  s=k*t^3/3  s=i  e=m*v^2/2",
                       2,
                       0,
                       &r->n,
                       &r->Psum,
                       &r->shrt,
                       &r->sovElement,
                       &Proportional,
                       1.0,
                       &tsS,
                       k,
                       &vS,
                       k,
                       &aS,
                       k,
                       &eFv,
                       massa);
    } else if(graph == 6) {
        k = ktS(nSize);
        makeAccelTable(f,
                       max,
                       P,
                       nSize,
                       &r->T,
                       "v=k*t^2  a=k*2*t  s=k*t^3/3  t=i  e=m*v^2/2",
                       2,
                       1,
                       &r->n,
                       &r->Psum,
                       &r->shrt,
                       &r->sovElement,
                       &stS,
                       k,
                       &Proportional,
                       1,
                       &vS,
                       k,
                       &aS,
                       k,
                       &eFv,
                       massa);
    } else {
        fprintf(f, "Generates %s steps without acceleration/deceleration.", s->max);
    }

    fclose(f);

    s->n = r->n;
}
