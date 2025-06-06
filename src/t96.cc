#include "t96.h"


void t96(int iopt, const std::array<double, 10>& parmod, double ps,
         double x, double y, double z, double& bx, double& by, double& bz) {

    constexpr double pdyn0 = 2.0;
    constexpr double eps10 = 3630.7;
    constexpr double a[9] = {1.162, 22.344, 18.50, 2.602, 6.903, 5.287, 0.5790, 0.4462, 0.7850};
    constexpr double am0 = 70.0, s0 = 1.08, x00 = 5.48, dsig = 0.005;
    constexpr double delimfx = 20.0, delimfy = 10.0;

    double pdyn = parmod[0];
    double dst = parmod[1];
    double byimf = parmod[2];
    double bzimf = parmod[3];

    double sps = sin(ps);

    double depr = 0.8 * dst - 13.0 * sqrt(pdyn);

    double bt = sqrt(byimf * byimf + bzimf * bzimf);
    double theta = (byimf == 0.0 && bzimf == 0.0) ? 0.0 : atan2(byimf, bzimf);
    if (theta <= 0.0) theta += 2 * M_PI;

    double ct = cos(theta);
    double st = sin(theta);
    double eps = 718.5 * sqrt(pdyn) * bt * sin(theta / 2.0);

    double facteps = eps / eps10 - 1.0;
    double factpd = sqrt(pdyn / pdyn0) - 1.0;

    double rcampl = -a[0] * depr;
    double tampl2 = a[1] + a[2] * factpd + a[3] * facteps;
    double tampl3 = a[4] + a[5] * factpd;
    double b1ampl = a[6] + a[7] * facteps;
    double b2ampl = 20.0 * b1ampl;
    double reconn = a[8];

    double xappa = pow(pdyn / pdyn0, 0.14);
    double xappa3 = xappa * xappa * xappa;
    double ys = y * ct - z * st;
    double zs = z * ct + y * st;

    double factimf = exp(x / delimfx - (ys / delimfy) * (ys / delimfy));

    double oimfx = 0.0;
    double oimfy = reconn * byimf * factimf;
    double oimfz = reconn * bzimf * factimf;

    double rimfampl = reconn * bt;

    double xx = x * xappa;
    double yy = y * xappa;
    double zz = z * xappa;

    double x0 = x00 / xappa;
    double am = am0 / xappa;
    double rho2 = y * y + z * z;
    double xmm = am + x - x0;
    if (xmm < 0.0) xmm = 0.0;
    double sigma = sqrt((am * am + rho2 + xmm * xmm + sqrt(pow(am * am + rho2 + xmm * xmm, 2) - 4.0 * am * am * xmm * xmm)) / (2.0 * am * am));

    if (sigma < s0 + dsig) {
        double cfx, cfy, cfz;
        t96DipShld(ps, xx, yy, zz, cfx, cfy, cfz);

        double bxrc, byrc, bzrc, bxt2, byt2, bzt2, bxt3, byt3, bzt3;
        t96TailRC96(sps, xx, yy, zz, bxrc, byrc, bzrc, bxt2, byt2, bzt2, bxt3, byt3, bzt3);

        double r1x, r1y, r1z, r2x, r2y, r2z;
        t96Birk1Tot02(ps, xx, yy, zz, r1x, r1y, r1z);
        t96Birk2Tot02(ps, xx, yy, zz, r2x, r2y, r2z);

        double rimfx, rimfys, rimfzs;
        t96Intercon(xx, ys * xappa, zs * xappa, rimfx, rimfys, rimfzs);

        double rimfy = rimfys * ct + rimfzs * st;
        double rimfz = rimfzs * ct - rimfys * st;

        double fx = cfx * xappa3 + rcampl * bxrc + tampl2 * bxt2 + tampl3 * bxt3 + b1ampl * r1x + b2ampl * r2x + rimfampl * rimfx;
        double fy = cfy * xappa3 + rcampl * byrc + tampl2 * byt2 + tampl3 * byt3 + b1ampl * r1y + b2ampl * r2y + rimfampl * rimfy;
        double fz = cfz * xappa3 + rcampl * bzrc + tampl2 * bzt2 + tampl3 * bzt3 + b1ampl * r1z + b2ampl * r2z + rimfampl * rimfz;

        if (sigma < s0 - dsig) {
            bx = fx;
            by = fy;
            bz = fz;
        } else {
            double fint = 0.5 * (1.0 - (sigma - s0) / dsig);
            double fext = 0.5 * (1.0 + (sigma - s0) / dsig);

            double qx, qy, qz;
            t96Dipole(ps, x, y, z, qx, qy, qz);

            bx = (fx + qx) * fint + oimfx * fext - qx;
            by = (fy + qy) * fint + oimfy * fext - qy;
            bz = (fz + qz) * fint + oimfz * fext - qz;
        }
    } else {
        double qx, qy, qz;
        t96Dipole(ps, x, y, z, qx, qy, qz);
        bx = oimfx - qx;
        by = oimfy - qy;
        bz = oimfz - qz;
    }
}


void t96DipoleShield(double psi, double x, double y, double z,
                        double *Bx, double *By, double *Bz) {
    
    /* calculate the magnetopause field which will
    shield the Earth's dipole field using cylindrical 
    harmonics. This bit comes from https://doi.org/10.1029/94JA03193 */

    /* the parameters to use for the cylindrical harmonics come
    from table 1 of the above paper*/
    double a[] = {0.24777,-27.003,-0.46815,7.0637,-1.5918,-0.090317};
    double b[] = {57.522,13.757,2.0100,10.458,4.5798,2.1695};
    double c[] = {-0.65385,-18.061,-0.40457,-5.0995,1.2846,.078231};
    double d[] = {39.592,13.291,1.9970,10.062,4.5140,2.1558};

    /* get the cylindrical harmonics for both 
    parallel and perpendicular components */
    double Bx0, By0, Bz0, Bx1, By1, Bz1;
    CylHarmPerp(x,y,z,a,b,&Bx0,&By0,&Bz0);
    CylHarmPara(x,y,z,a,b,&Bx1,&By1,&Bz1);


    /*combine them (equation 16)*/
    double cps = cos(psi);
    double sps = sin(psi);
    *Bx = Bx0*cps + Bx1*sps;
    *By = By0*cps + By1*sps;
    *Bz = Bz0*cps + Bz1*sps;

}

void CylHarmPerp(    double x, double y, double z,
                    double *a, double *b,
                    double *Bx, double *By, double *Bz) {
    
    /* I took this bit from the original Fortran code */
    double rho = sqrt(y*y + z*z);
    double sinp, cosp;
    if (rho < 1e-8) {
        sinp = 1.0;
        cosp = 0.0;
        rho = 1e-8;
    } else {
        sinp = z/rho;
        cosp = y/rho;
    }

    /* some variables which will be used more than once */
    double sinp2 = sinp*sinp;
    double cosp2 = cosp*cosp;
    double xb1,expxb0,expxb1,rhob0,rhob1,J0rb0,J0rb1,J1rb0,J1rb1;


    /* equation 10, 11 and 12 */
    double bx = 0.0, br = 0.0, bp = 0.0;
    bx = 0.0;
    br = 0.0;
    bp = 0.0;
    int i;
    for(i=0;i<3;i++) {
        /* get the common terms */
        xb1 = x/b[i+1];
        expxb0 = exp(x/b[i]);
        expxb1 = exp(xb1);
        rhob0 = rho/b[i];
        rhob1 = rho/b[i+3];
        J0rb0 = std::cyl_bessel_j(0,rhob0);
        J0rb1 = std::cyl_bessel_j(0,rhob1);
        J1rb0 = std::cyl_bessel_j(1,rhob0);
        J1rb1 = std::cyl_bessel_j(1,rhob1);

        /* sum them */
        bx += -a[i]*expxb0*J1rb0 + (a[i+3]/b[i+3])*expxb1*(rho*J0rb1 + x*J1rb1);
        br += a[i]*expxb0*(J1rb0/rhob0 - J1rb0) + a[i+3]*expxb1*(xb1*J0rb1 - (rhob1*rhob1 + xb1 - 1)*J1rb1/rhob1);
        bp += -a[i]*expxb0*J1rb0/rhob0 + a[i+3]*expxb1*(J0rb1 + ((x - b[i+3])/b[i+3])*J1rb1/rhob1);

    }
    /* multiply by sine or cosine */
    bx *= sinp;
    br *= sinp;
    bp *= cosp;

    /* convert back to GSM*/
    *Bx = bx;
    *By = br*cosp - bp*sinp;
    *Bz = br*sinp + bp*cosp;

}


void CylHarmPara(    double x, double y, double z,
                    double *c, double *d,
                    double *Bx, double *By, double *Bz) {
    
    /* I took this bit from the original Fortran code */
    double rho = sqrt(y*y + z*z);
    double sinp, cosp;
    if (rho < 1e-8) {
        sinp = 1.0;
        cosp = 0.0;
        rho = 1e-8;
    } else {
        sinp = z/rho;
        cosp = y/rho;
    }

    /* some variables which will be used more than once */
    double sinp2 = sinp*sinp;
    double cosp2 = cosp*cosp;
    double xd1,expxd0,expxd1,rhod0,rhod1,J0rd0,J0rd1,J1rd0,J1rd1;


    /* equation 13 and 14 (15 = 0)*/
    double bx = 0.0, br = 0.0, bp = 0.0;
    bx = 0.0;
    br = 0.0;

    int i;
    for(i=0;i<3;i++) {
        /* get the common terms */
        xd1 = x/d[i+1];
        expxd0 = exp(x/d[i]);
        expxd1 = exp(xd1);
        rhod0 = rho/d[i];
        rhod1 = rho/d[i+3];
        J0rd0 = std::cyl_bessel_j(0,rhod0);
        J0rd1 = std::cyl_bessel_j(0,rhod1);
        J1rd0 = std::cyl_bessel_j(1,rhod0);
        J1rd1 = std::cyl_bessel_j(1,rhod1);

        /* sum them */
        bx += -c[i]*expxd0*J1rd0 + c[i+3]*expxd1*(rhod1*J1rd1 -((x+d[i+3])/d[i+3])*J0rd1);
        br += -c[i]*expxd0*J1rd0 + (c[i+3]/d[i+3])*expxd1*(rho*J0rd1 - x*J1rd1);

    }

    /* convert back to GSM*/
    *Bx = bx;
    *By = br*cosp;
    *Bz = br*sinp;

}


/**
 * Calculates the interconnection magnetic field components (Bx, By, Bz)
 * inside the magnetosphere using Cartesian harmonics.
 *
 * Note: This operates in a rotated coordinate system aligned with IMF Bz.
 *
 * @param x   GSM X coordinate (Re)
 * @param y   GSM Y coordinate (Re)
 * @param z   GSM Z coordinate (Re)
 * @param Bx  Pointer to store X component of magnetic field (nT)
 * @param By  Pointer to store Y component of magnetic field (nT)
 * @param Bz  Pointer to store Z component of magnetic field (nT)
 */
void t96Intercon(double x, double y, double z, double* Bx, double* By, double* Bz) {
    // Static ensures initialization happens only once (equivalent to DATA + M check)
    static const std::array<double, 15> A = {
        -8.411078731, 5932254.951, -9073284.93,
        -11.68794634, 6027598.824, -9218378.368,
        -6.508798398, -11824.42793, 18015.66212,
         7.99754043, 13.9669886, 90.24475036,
         16.75728834, 1015.645781, 1553.493216
    };

    static std::array<double, 3> RP, RR;
    static bool initialized = false;

    if (!initialized) {
        for (int i = 0; i < 3; ++i) {
            RP[i] = 1.0 / A[9 + i];   // P(1..3) = A(10..12)
            RR[i] = 1.0 / A[12 + i];  // R(1..3) = A(13..15)
        }
        initialized = true;
    }

    double bx = 0.0;
    double by = 0.0;
    double bz = 0.0;

    int L = 0;

    // Only "perpendicular" symmetry is used
    for (int i = 0; i < 3; ++i) {
        double cy_pi = std::cos(y * RP[i]);
        double sy_pi = std::sin(y * RP[i]);

        for (int k = 0; k < 3; ++k) {
            double sz_rk = std::sin(z * RR[k]);
            double cz_rk = std::cos(z * RR[k]);

            double sqpr = std::sqrt(RP[i] * RP[i] + RR[k] * RR[k]);
            double epr = std::exp(x * sqpr);

            double hx = -sqpr * epr * cy_pi * sz_rk;
            double hy = RP[i] * epr * sy_pi * sz_rk;
            double hz = -RR[k] * epr * cy_pi * cz_rk;

            bx += A[L] * hx;
            by += A[L] * hy;
            bz += A[L] * hz;

            ++L;
        }
    }

    *Bx = bx;
    *By = by;
    *Bz = bz;
}



void t96TailRC96(double sps, double x, double y, double z,
                 double& BXRC, double& BYRC, double& BZRC,
                 double& BXT2, double& BYT2, double& BZT2,
                 double& BXT3, double& BYT3, double& BZT3) {

    // ----- DATA blocks converted to std::array -----
    const std::array<double, 48> ARC = {{
        -3.087699646, 3.516259114, 18.81380577, -13.95772338,
        -5.497076303, 0.1712890838, 2.392629189, -2.728020808,
        -14.79349936, 11.08738083, 4.388174084, 0.02492163197,
        0.7030375685, -0.7966023165, -3.835041334, 2.642228681,
        -0.2405352424, -0.7297705678, -0.3680255045, 0.1333685557,
        2.795140897, -1.078379954, 0.8014028630, 0.1245825565,
        0.6149982835, -0.2207267314, -4.424578723, 1.730471572,
        -1.716313926, -0.2306302941, -0.2450342688, 0.08617173961,
        1.54697858, -0.6569391113, -0.6537525353, 0.2079417515,
        12.75434981, 11.37659788, 636.4346279, 1.752483754,
        3.604231143, 12.83078674, 7.412066636, 9.434625736,
        676.7557193, 1.701162737, 3.580307144, 14.64298662
    }};

    const std::array<double, 48> ATAIL2 = {{
        0.8747515218, -0.9116821411, 2.209365387, -2.159059518,
        -7.059828867, 5.924671028, -1.916935691, 1.996707344,
        -3.877101873, 3.947666061, 11.38715899, -8.343210833,
        1.194109867, -1.244316975, 3.73895491, -4.406522465,
        -20.66884863, 3.020952989, 0.2189908481, -0.09942543549,
        -0.927225562, 0.1555224669, 0.6994137909, -0.08111721003,
        -0.7565493881, 0.4686588792, 4.266058082, -0.3717470262,
        -3.920787807, 0.02298569870, 0.7039506341, -0.5498352719,
        -6.675140817, 0.8279283559, -2.234773608, -1.622656137,
        5.187666221, 6.802472048, 39.13543412, 2.784722096,
        6.979576616, 25.71716760, 4.495005873, 8.068408272,
        93.47887103, 4.158030104, 9.313492566, 57.18240483
    }};

    const std::array<double, 48> ATAIL3 = {{
        -19091.95061, -3011.613928, 20582.16203, 4242.918430,
        -2377.091102, -1504.820043, 19884.04650, 2725.150544,
        -21389.04845, -3990.475093, 2401.610097, 1548.171792,
        -946.5493963, 490.1528941, 986.9156625, -489.3265930,
        -67.99278499, 8.711175710, -45.15734260, -10.76106500,
        210.7927312, 11.41764141, -178.0262808, 0.7558830028,
        339.3806753, 9.904695974, 69.50583193, -118.0271581,
        22.85935896, 45.91014857, -425.6607164, 15.47250738,
        118.2988915, 65.58594397, -201.4478068, -14.57062940,
        19.69877970, 20.30095680, 86.45407420, 22.50403727,
        23.41617329, 48.48140573, 24.61031329, 123.5395974,
        223.5367692, 39.50824342, 65.83385762, 266.2948657
    }};

    // ----- Parameters -----
    constexpr double RH = 9.0;
    constexpr double DR = 4.0;
    constexpr double G = 10.0;
    constexpr double D0 = 2.0;
    constexpr double DELTADY = 10.0;

    // ----- Precompute COMMON variables -----
    double dr2 = DR * DR;
    double c11 = std::sqrt((1.0 + RH) * (1.0 + RH) + dr2);
    double c12 = std::sqrt((1.0 - RH) * (1.0 - RH) + dr2);
    double c1 = c11 - c12;
    double spsc1 = sps / c1;

    warpData.RPS = 0.5 * (c11 + c12) * sps;

    double R = std::sqrt(x * x + y * y + z * z);
    double sq1 = std::sqrt((R + RH) * (R + RH) + dr2);
    double sq2 = std::sqrt((R - RH) * (R - RH) + dr2);
    double C = sq1 - sq2;
    double CS = (R + RH) / sq1 - (R - RH) / sq2;

    warpData.SPSS = spsc1 / R * C;
    warpData.CPSS = std::sqrt(1.0 - warpData.SPSS * warpData.SPSS);
    warpData.DPSRR = sps / (R * R) * (CS * R - C) / std::sqrt((R * c1) * (R * c1) - (C * sps) * (C * sps));

    double wfac = y / (std::pow(y, 4) + 10000.0);
    double W = wfac * y * y * y;
    double WS = 40000.0 * y * wfac * wfac;

    warpData.WARP = G * sps * W;

    warpData.XS = x * warpData.CPSS - z * warpData.SPSS;
    warpData.ZSWW = z * warpData.CPSS + x * warpData.SPSS;
    warpData.ZS = warpData.ZSWW + warpData.WARP;

    warpData.DXSX = warpData.CPSS - x * warpData.ZSWW * warpData.DPSRR;
    warpData.DXSY = -y * warpData.ZSWW * warpData.DPSRR;
    warpData.DXSZ = -warpData.SPSS - z * warpData.ZSWW * warpData.DPSRR;

    warpData.DZSX = warpData.SPSS + x * warpData.XS * warpData.DPSRR;
    warpData.DZSY = warpData.XS * y * warpData.DPSRR + G * sps * WS;
    warpData.DZSZ = warpData.CPSS + warpData.XS * z * warpData.DPSRR;

    warpData.D = D0 + DELTADY * std::pow(y / 20.0, 2);
    double dddy = DELTADY * y * 0.005;

    warpData.DZETAS = std::sqrt(warpData.ZS * warpData.ZS + warpData.D * warpData.D);
    warpData.DDZETADX = warpData.ZS * warpData.DXSX / warpData.DZETAS;
    warpData.DDZETADY = (warpData.ZS * warpData.DZSY + warpData.D * dddy) / warpData.DZETAS;
    warpData.DDZETADZ = warpData.ZS * warpData.DZSZ / warpData.DZETAS;

    // ----- Compute Fields -----
    double WX, WY, WZ, HX, HY, HZ;

    t96ShlCar3x3(ARC, x, y, z, sps, WX, WY, WZ);
    t96RingCurr96(x, y, z, HX, HY, HZ);
    BXRC = WX + HX;
    BYRC = WY + HY;
    BZRC = WZ + HZ;

    t96ShlCar3x3(ATAIL2, x, y, z, sps, WX, WY, WZ);
    t96TailDisk(x, y, z, HX, HY, HZ);
    BXT2 = WX + HX;
    BYT2 = WY + HY;
    BZT2 = WZ + HZ;

    t96ShlCar3x3(ATAIL3, x, y, z, sps, WX, WY, WZ);
    double HX87, HZ87;
    t96Tail87(x, z, &HX87, &HZ87);
    BXT3 = WX + HX87;
    BYT3 = WY;
    BZT3 = WZ + HZ87;
}



/**
 * Computes the magnetic field components (Bx, By, Bz) from the ring current
 * using space-warping, following Tsyganenko's 1996 approach.
 *
 * @param x   GSM X coordinate (Re)
 * @param y   GSM Y coordinate (Re)
 * @param z   GSM Z coordinate (Re)
 * @param Bx  Reference to store X component of magnetic field (nT)
 * @param By  Reference to store Y component of magnetic field (nT)
 * @param Bz  Reference to store Z component of magnetic field (nT)
 */
void t96RingCurr96(double x, double y, double z, 
                   double& Bx, double& By, double& Bz) {
    // Constants from DATA
    constexpr double D0 = 2.0;
    constexpr double DELTADX = 0.0;  // Symmetric ring current
    constexpr double XD = 0.0;
    constexpr double XLDX = 4.0;

    const std::array<double, 2> F = {569.895366, -1603.386993};
    const std::array<double, 2> BETA = {2.722188, 3.766875};

    // Compute DZSY manually (no Y-Z warping)
    double DZSY = warpData.XS * y * warpData.DPSRR;

    double xxd = x - XD;
    double fdx = 0.5 * (1.0 + xxd / std::sqrt(xxd * xxd + XLDX * XLDX));
    double dddx = DELTADX * 0.5 * XLDX * XLDX / std::pow(xxd * xxd + XLDX * XLDX, 1.5);
    double D = D0 + DELTADX * fdx;

    double dzetas = std::sqrt(warpData.ZS * warpData.ZS + D * D);
    double rhos = std::sqrt(warpData.XS * warpData.XS + y * y);

    double ddzetadx = (warpData.ZS * warpData.DZSX + D * dddx) / dzetas;
    double ddzetady = warpData.ZS * DZSY / dzetas;
    double ddzetadz = warpData.ZS * warpData.DZSZ / dzetas;

    double drhosdx, drhosdy, drhosdz;

    if (rhos < 1e-5) {
        drhosdx = 0.0;
        drhosdy = (y >= 0.0) ? 1.0 : -1.0;
        drhosdz = 0.0;
    } else {
        drhosdx = warpData.XS * warpData.DXSX / rhos;
        drhosdy = (warpData.XS * warpData.DXSY + y) / rhos;
        drhosdz = warpData.XS * warpData.DXSZ / rhos;
    }

    Bx = 0.0;
    By = 0.0;
    Bz = 0.0;

    for (int i = 0; i < 2; ++i) {
        double beta = BETA[i];

        double s1 = std::sqrt(std::pow(dzetas + beta, 2) + std::pow(rhos + beta, 2));
        double s2 = std::sqrt(std::pow(dzetas + beta, 2) + std::pow(rhos - beta, 2));

        double ds1ddz = (dzetas + beta) / s1;
        double ds2ddz = (dzetas + beta) / s2;

        double ds1drhos = (rhos + beta) / s1;
        double ds2drhos = (rhos - beta) / s2;

        double ds1dx = ds1ddz * ddzetadx + ds1drhos * drhosdx;
        double ds1dy = ds1ddz * ddzetady + ds1drhos * drhosdy;
        double ds1dz = ds1ddz * ddzetadz + ds1drhos * drhosdz;

        double ds2dx = ds2ddz * ddzetadx + ds2drhos * drhosdx;
        double ds2dy = ds2ddz * ddzetady + ds2drhos * drhosdy;
        double ds2dz = ds2ddz * ddzetadz + ds2drhos * drhosdz;

        double s1ts2 = s1 * s2;
        double s1ps2 = s1 + s2;
        double s1ps2sq = s1ps2 * s1ps2;

        double fac1 = std::sqrt(s1ps2sq - 4.0 * beta * beta);
        double AS = fac1 / (s1ts2 * s1ps2sq);

        double term1 = 1.0 / (s1ts2 * s1ps2 * fac1);
        double fac2 = AS / s1ps2sq;

        double dasds1 = term1 - fac2 / s1 * (s2 * s2 + s1 * (3.0 * s1 + 4.0 * s2));
        double dasds2 = term1 - fac2 / s2 * (s1 * s1 + s2 * (3.0 * s2 + 4.0 * s1));

        double dasdx = dasds1 * ds1dx + dasds2 * ds2dx;
        double dasdy = dasds1 * ds1dy + dasds2 * ds2dy;
        double dasdz = dasds1 * ds1dz + dasds2 * ds2dz;

        Bx += F[i] * ((2.0 * AS + y * dasdy) * warpData.SPSS - warpData.XS * dasdz
              + AS * warpData.DPSRR * (y * y * warpData.CPSS + z * warpData.ZS));

        By -= F[i] * y * (AS * warpData.DPSRR * warpData.XS + dasdz * warpData.CPSS + dasdx * warpData.SPSS);

        Bz += F[i] * ((2.0 * AS + y * dasdy) * warpData.CPSS + warpData.XS * dasdx
              - AS * warpData.DPSRR * (x * warpData.ZS + y * y * warpData.SPSS));
    }
}


/**
 * Computes the magnetic field components (Bx, By, Bz) from the tail current disk.
 * This uses space-warping instead of classical shearing (per Tsyganenko & Stern, 1996).
 *
 * @param x   GSM X coordinate (Re)
 * @param y   GSM Y coordinate (Re)
 * @param z   GSM Z coordinate (Re)
 * @param Bx  Reference to store X component of magnetic field (nT)
 * @param By  Reference to store Y component of magnetic field (nT)
 * @param Bz  Reference to store Z component of magnetic field (nT)
 */
void t96TailDisk(double x, double y, double z,
                 double& Bx, double& By, double& Bz) {
    constexpr double XSHIFT = 4.5;

    const std::array<double, 4> F = {
        -745796.7338, 1176470.141, -444610.529, -57508.01028
    };

    const std::array<double, 4> BETA = {
        7.925, 8.085, 8.47125, 27.895
    };

    // Compute RHOS (radial distance in warped coordinates)
    double dx_shift = warpData.XS - XSHIFT;
    double rhos = std::sqrt(dx_shift * dx_shift + y * y);

    double drhosdx, drhosdy, drhosdz;

    if (rhos < 1e-5) {
        drhosdx = 0.0;
        drhosdy = (y >= 0.0) ? 1.0 : -1.0;
        drhosdz = 0.0;
    } else {
        drhosdx = dx_shift * warpData.DXSX / rhos;
        drhosdy = (dx_shift * warpData.DXSY + y) / rhos;
        drhosdz = dx_shift * warpData.DXSZ / rhos;
    }

    Bx = 0.0;
    By = 0.0;
    Bz = 0.0;

    for (int i = 0; i < 4; ++i) {
        double beta = BETA[i];

        double s1 = std::sqrt(std::pow(warpData.DZETAS + beta, 2) + std::pow(rhos + beta, 2));
        double s2 = std::sqrt(std::pow(warpData.DZETAS + beta, 2) + std::pow(rhos - beta, 2));

        double ds1ddz = (warpData.DZETAS + beta) / s1;
        double ds2ddz = (warpData.DZETAS + beta) / s2;

        double ds1drhos = (rhos + beta) / s1;
        double ds2drhos = (rhos - beta) / s2;

        double ds1dx = ds1ddz * warpData.DDZETADX + ds1drhos * drhosdx;
        double ds1dy = ds1ddz * warpData.DDZETADY + ds1drhos * drhosdy;
        double ds1dz = ds1ddz * warpData.DDZETADZ + ds1drhos * drhosdz;

        double ds2dx = ds2ddz * warpData.DDZETADX + ds2drhos * drhosdx;
        double ds2dy = ds2ddz * warpData.DDZETADY + ds2drhos * drhosdy;
        double ds2dz = ds2ddz * warpData.DDZETADZ + ds2drhos * drhosdz;

        double s1ts2 = s1 * s2;
        double s1ps2 = s1 + s2;
        double s1ps2sq = s1ps2 * s1ps2;

        double fac1 = std::sqrt(s1ps2sq - 4.0 * beta * beta);
        double AS = fac1 / (s1ts2 * s1ps2sq);

        double term1 = 1.0 / (s1ts2 * s1ps2 * fac1);
        double fac2 = AS / s1ps2sq;

        double dasds1 = term1 - fac2 / s1 * (s2 * s2 + s1 * (3.0 * s1 + 4.0 * s2));
        double dasds2 = term1 - fac2 / s2 * (s1 * s1 + s2 * (3.0 * s2 + 4.0 * s1));

        double dasdx = dasds1 * ds1dx + dasds2 * ds2dx;
        double dasdy = dasds1 * ds1dy + dasds2 * ds2dy;
        double dasdz = dasds1 * ds1dz + dasds2 * ds2dz;

        Bx += F[i] * ((2.0 * AS + y * dasdy) * warpData.SPSS
              - dx_shift * dasdz
              + AS * warpData.DPSRR * (y * y * warpData.CPSS + z * warpData.ZSWW));

        By -= F[i] * y * (AS * warpData.DPSRR * warpData.XS + dasdz * warpData.CPSS + dasdx * warpData.SPSS);

        Bz += F[i] * ((2.0 * AS + y * dasdy) * warpData.CPSS
              + dx_shift * dasdx
              - AS * warpData.DPSRR * (x * warpData.ZSWW + y * y * warpData.SPSS));
    }
}


/**
 * Computes the magnetic field components (Bx, Bz) from the 1987 Tsyganenko tail current sheet model.
 * Uses space-warping via RPS and WARP values from WarpData.
 *
 * @param x   GSM X coordinate (Re)
 * @param z   GSM Z coordinate (Re)
 * @param Bx  Reference to store X component of magnetic field (nT)
 * @param Bz  Reference to store Z component of magnetic field (nT)
 */
void t96Tail87(double x, double z, double* Bx, double* Bz) {
    // Constants (equivalent to DATA statements)
    constexpr double DD   = 3.0;
    constexpr double HPI  = 1.5707963;  // PI / 2
    constexpr double RT   = 40.0;
    constexpr double XN   = -10.0;
    constexpr double X1   = -1.261;
    constexpr double X2   = -0.663;
    constexpr double B0   = 0.391734;
    constexpr double B1   = 5.89715;
    constexpr double B2   = 24.6833;
    constexpr double XN21 = 76.37;
    constexpr double XNR  = -0.1071;
    constexpr double ADLN = 0.13238005;

    // Apply space-warping to Z coordinate
    double ZS = z - warpData.RPS + warpData.WARP;
    double ZP = z - RT;
    double ZM = z + RT;

    double XNX = XN - x;
    double XNX2 = XNX * XNX;

    double XC1 = x - X1;
    double XC2 = x - X2;
    double XC22 = XC2 * XC2;
    double XR2 = XC2 * XNR;
    double XC12 = XC1 * XC1;

    double D2 = DD * DD;

    double B20 = ZS * ZS + D2;
    double B2P = ZP * ZP + D2;
    double B2M = ZM * ZM + D2;

    double B_val  = std::sqrt(B20);
    double BP     = std::sqrt(B2P);
    double BM     = std::sqrt(B2M);

    double XA1  = XC12 + B20;
    double XAP1 = XC12 + B2P;
    double XAM1 = XC12 + B2M;

    double XA2  = 1.0 / (XC22 + B20);
    double XAP2 = 1.0 / (XC22 + B2P);
    double XAM2 = 1.0 / (XC22 + B2M);

    double XNA  = XNX2 + B20;
    double XNAP = XNX2 + B2P;
    double XNAM = XNX2 + B2M;

    double F  = B20 - XC22;
    double FP = B2P - XC22;
    double FM = B2M - XC22;

    double XLN1  = std::log(XN21 / XNA);
    double XLNP1 = std::log(XN21 / XNAP);
    double XLNM1 = std::log(XN21 / XNAM);

    double XLN2  = XLN1 + ADLN;
    double XLNP2 = XLNP1 + ADLN;
    double XLNM2 = XLNM1 + ADLN;

    double ALN = 0.25 * (XLNP1 + XLNM1 - 2.0 * XLN1);

    double S0  = (std::atan(XNX / B_val) + HPI) / B_val;
    double S0P = (std::atan(XNX / BP) + HPI) / BP;
    double S0M = (std::atan(XNX / BM) + HPI) / BM;

    double S1  = (0.5 * XLN1 + XC1 * S0) / XA1;
    double S1P = (0.5 * XLNP1 + XC1 * S0P) / XAP1;
    double S1M = (0.5 * XLNM1 + XC1 * S0M) / XAM1;

    double S2  = (XC2 * XA2 * XLN2 - XNR - F * XA2 * S0) * XA2;
    double S2P = (XC2 * XAP2 * XLNP2 - XNR - FP * XAP2 * S0P) * XAP2;
    double S2M = (XC2 * XAM2 * XLNM2 - XNR - FM * XAM2 * S0M) * XAM2;

    double G1  = (B20 * S0  - 0.5 * XC1 * XLN1)  / XA1;
    double G1P = (B2P * S0P - 0.5 * XC1 * XLNP1) / XAP1;
    double G1M = (B2M * S0M - 0.5 * XC1 * XLNM1) / XAM1;

    double G2  = ((0.5 * F  * XLN2  + 2.0 * S0  * B20 * XC2) * XA2  + XR2) * XA2;
    double G2P = ((0.5 * FP * XLNP2 + 2.0 * S0P * B2P * XC2) * XAP2 + XR2) * XAP2;
    double G2M = ((0.5 * FM * XLNM2 + 2.0 * S0M * B2M * XC2) * XAM2 + XR2) * XAM2;

    *Bx = B0 * (ZS * S0 - 0.5 * (ZP * S0P + ZM * S0M))
        + B1 * (ZS * S1 - 0.5 * (ZP * S1P + ZM * S1M))
        + B2 * (ZS * S2 - 0.5 * (ZP * S2P + ZM * S2M));

    *Bz = B0 * ALN + B1 * (G1 - 0.5 * (G1P + G1M)) + B2 * (G2 - 0.5 * (G2P + G2M));
}


/**
 * Computes shielding magnetic field components (HX, HY, HZ)
 * using Cartesian harmonics for the Tsyganenko 1996 model.
 *
 * @param A     Array of 48 parameters (36 linear coefficients + 12 nonlinear scales)
 * @param x     GSM X coordinate (Re)
 * @param y     GSM Y coordinate (Re)
 * @param z     GSM Z coordinate (Re)
 * @param sps   Sine of dipole tilt angle (sin(PS))
 * @param HX    Reference to store X component of shielding field
 * @param HY    Reference to store Y component of shielding field
 * @param HZ    Reference to store Z component of shielding field
 */
void t96ShlCar3x3(const std::array<double, 48>& A, double x, double y, double z, 
                  double sps, double& HX, double& HY, double& HZ) {
    double cps = std::sqrt(1.0 - sps * sps);
    double s3ps = 4.0 * cps * cps - 1.0;   // Equivalent to sin(3*PS)/sin(PS)

    HX = 0.0;
    HY = 0.0;
    HZ = 0.0;

    int L = 0;

    for (int m = 1; m <= 2; ++m) {  // m = symmetry type
        for (int i = 0; i < 3; ++i) {
            double P = A[36 + i];
            double Q = A[42 + i];

            double cypi = std::cos(y / P);
            double sypi = std::sin(y / P);
            double cyqi = std::cos(y / Q);
            double syqi = std::sin(y / Q);

            for (int k = 0; k < 3; ++k) {
                double R = A[39 + k];
                double S = A[45 + k];

                double szrk = std::sin(z / R);
                double czrk = std::cos(z / R);
                double czsk = std::cos(z / S);
                double szsk = std::sin(z / S);

                double sqpr = std::sqrt(1.0 / (P * P) + 1.0 / (R * R));
                double sqqs = std::sqrt(1.0 / (Q * Q) + 1.0 / (S * S));

                double epr = std::exp(x * sqpr);
                double eqs = std::exp(x * sqqs);

                for (int n = 1; n <= 2; ++n) {
                    double DX, DY, DZ;

                    if (m == 1) {  // Perpendicular symmetry
                        if (n == 1) {
                            DX = -sqpr * epr * cypi * szrk;
                            DY = epr / P * sypi * szrk;
                            DZ = -epr / R * cypi * czrk;
                        } else {
                            DX *= cps;
                            DY *= cps;
                            DZ *= cps;
                        }
                    } else {  // Parallel symmetry
                        if (n == 1) {
                            DX = -sps * sqqs * eqs * cyqi * czsk;
                            DY = sps * eqs / Q * syqi * czsk;
                            DZ = sps * eqs / S * cyqi * szsk;
                        } else {
                            DX *= s3ps;
                            DY *= s3ps;
                            DZ *= s3ps;
                        }
                    }

                    HX += A[L] * DX;
                    HY += A[L] * DY;
                    HZ += A[L] * DZ;

                    ++L;
                }
            }
        }
    }
}


// ==== Struct Definitions for COMMON Blocks ====
struct Coord11 {
    std::array<double, 12> xx1;
    std::array<double, 12> yy1;
};

struct RHDR {
    double rh = 9.0;
    double dr = 4.0;
};

struct LoopDip1 {
    double tilt = 1.00891;
    std::array<double, 2> xcentre = {2.28397, -5.60831};
    std::array<double, 2> radius = {1.86106, 7.83281};
    double dipx = 1.12541;
    double dipy = 0.945719;
};

struct Coord21 {
    std::array<double, 14> xx2;
    std::array<double, 14> yy2;
    std::array<double, 14> zz2;
};

struct DX1 {
    double dx = -0.16;
    double scaleIn = 0.08;
    double scaleOut = 0.4;
};

// ==== Constants ====
constexpr double DTET0 = 0.034906;
constexpr double XLTDAY = 78.0;
constexpr double XLTNGHT = 70.0;
constexpr double DEG2RAD = 0.01745329;
constexpr double PI = 3.141592654;

// Coefficient Arrays
const double C1[26] = {
    -0.000911582, -0.00376654, -0.00727423, -0.00270084,
    -0.00123899, -0.00154387, -0.00340040, -0.0191858,
    -0.0518979, 0.0635061, 0.440680, -0.396570, 0.00561238,
     0.00160938, -0.00451229, -0.00251810, -0.00151599,
    -0.00133665, -0.000962089, -0.0272085, -0.0524319,
     0.0717024, 0.523439, -0.405015, -89.5587, 23.2806
 };
 
 const double C2[79] = {
     6.04133, 0.305415, 0.000606066, 0.000128379, -0.0000179406,
     1.41714, -27.2586, -4.28833, -1.30675, 35.5607, 8.95792, 0.000961617,
    -0.000801477, -0.000782795, -1.65242, -16.5242, -5.33798, 0.000424878,
     0.000331787, -0.000704305, 0.000844342, 0.0000953682, 0.000886271,
     25.1120, 20.9299, 5.14569, -44.1670, -51.0672, -1.87725, 20.2998,
     48.7505, -2.97415, 3.35184, -54.2921, -0.838712, -10.5123, 70.7594,
    -4.94104, 0.000106166, 0.000465791, -0.000193719, 10.8439, -29.7968,
     8.08068, 0.000463507, -0.0000224475, 0.000177035, -0.000317581,
    -0.000264487, 0.000102075, 7.71390, 10.1915, -4.99797, -23.1114,
    -29.2043, 12.2928, 10.9542, 33.6671, -9.3851, 0.000174615, -0.000000789777,
     0.000686047, 0.000460104, -0.0345216, 0.0221871, 0.110078,
    -0.0661373, 0.0249201, 0.343978, -0.0000193145, 0.0000493963,
    -0.000535748, 0.000191833, -0.00100496, -0.00210103, -0.0232195,
     0.0315335, -0.134320, -0.263222
 };
 

// ==== Main Function ====
void t96Birk1Tot02(double ps, double x, double y, double z, double* Bx, double* By, double* Bz) {
    RHDR rhdr;
    double rh = rhdr.rh;
    double dr = rhdr.dr;
    double dr2 = dr * dr;

    // Precompute angles and distances
    double tNoonN = (90.0 - XLTDAY) * DEG2RAD;
    double tNoonS = PI - tNoonN;
    double dTetDn = (XLTDAY - XLTNGHT) * DEG2RAD;

    double sps = std::sin(ps);
    double r2 = x * x + y * y + z * z;
    double r = std::sqrt(r2);
    double r3 = r * r2;

    double rmrh = r - rh;
    double rprh = r + rh;

    double sqm = std::sqrt(rmrh * rmrh + dr2);
    double sqp = std::sqrt(rprh * rprh + dr2);

    double C = sqp - sqm;
    double Q = std::sqrt((rh + 1.0) * (rh + 1.0) + dr2) - std::sqrt((rh - 1.0) * (rh - 1.0) + dr2);

    double spsas = sps / r * C / Q;
    double cpsas = std::sqrt(1.0 - spsas * spsas);

    double xas = x * cpsas - z * spsas;
    double zas = x * spsas + z * cpsas;

    double pas = (xas != 0.0 || y != 0.0) ? std::atan2(y, xas) : 0.0;
    double tas = std::atan2(std::sqrt(xas * xas + y * y), zas);
    double stas = std::sin(tas);
    double F = stas / std::pow(stas * stas * stas * stas * stas * stas * (1.0 - r3) + r3, 1.0 / 6.0);

    double tet0 = std::asin(F);
    if (tas > PI / 2.0) tet0 = PI - tet0;

    double dtet = dTetDn * std::pow(std::sin(pas * 0.5), 2);
    double tetR1N = tNoonN + dtet;
    double tetR1S = tNoonS - dtet;

    // ==== Determine Region ====
    int loc = 0;
    if (tet0 < tetR1N - DTET0 || tet0 > tetR1S + DTET0) loc = 1;  // High-latitude
    if (tet0 > tetR1N + DTET0 && tet0 < tetR1S - DTET0) loc = 2;  // Plasma sheet
    if (tet0 >= tetR1N - DTET0 && tet0 <= tetR1N + DTET0) loc = 3; // North PSBL
    if (tet0 >= tetR1S - DTET0 && tet0 <= tetR1S + DTET0) loc = 4; // South PSBL

    double bx = 0.0, by = 0.0, bz = 0.0;
    std::array<double, 4> xi = {x, y, z, ps};

    if (loc == 1) {
        double D1[3][26];
        t96DipLoop1(xi, D1);
        for (int i = 0; i < 26; ++i) {
            bx += C1[i] * D1[0][i];
            by += C1[i] * D1[1][i];
            bz += C1[i] * D1[2][i];
        }
    }

    if (loc == 2) {
        double D2[3][79];
        t96ConDip1(xi, D2);
        for (int i = 0; i < 79; ++i) {
            bx += C2[i] * D2[0][i];
            by += C2[i] * D2[1][i];
            bz += C2[i] * D2[2][i];
        }
    }

    if (loc == 3 || loc == 4) {
        bool isNorth = (loc == 3);
        double t01 = isNorth ? tetR1N - DTET0 : tetR1S - DTET0;
        double t02 = isNorth ? tetR1N + DTET0 : tetR1S + DTET0;

        double sqr = std::sqrt(r);
        double st01as = sqr / std::pow(r3 + 1.0 / std::pow(std::sin(t01), 6) - 1.0, 1.0 / 6.0);
        double st02as = sqr / std::pow(r3 + 1.0 / std::pow(std::sin(t02), 6) - 1.0, 1.0 / 6.0);
        double ct01as = (isNorth ? 1 : -1) * std::sqrt(1.0 - st01as * st01as);
        double ct02as = (isNorth ? 1 : -1) * std::sqrt(1.0 - st02as * st02as);

        double xas1 = r * st01as * std::cos(pas);
        double y1 = r * st01as * std::sin(pas);
        double zas1 = r * ct01as;
        double x1 = xas1 * cpsas + zas1 * spsas;
        double z1 = -xas1 * spsas + zas1 * cpsas;

        double xas2 = r * st02as * std::cos(pas);
        double y2 = r * st02as * std::sin(pas);
        double zas2 = r * ct02as;
        double x2 = xas2 * cpsas + zas2 * spsas;
        double z2 = -xas2 * spsas + zas2 * cpsas;

        double bx1 = 0.0, by1 = 0.0, bz1 = 0.0;
        double bx2 = 0.0, by2 = 0.0, bz2 = 0.0;

        xi = {x1, y1, z1, ps};
        if (isNorth) {
            double D1[3][26];
            t96DipLoop1(xi, D1);
            for (int i = 0; i < 26; ++i) {
                bx1 += C1[i] * D1[0][i];
                by1 += C1[i] * D1[1][i];
                bz1 += C1[i] * D1[2][i];
            }
        } else {
            double D2[3][79];
            t96ConDip1(xi, D2);
            for (int i = 0; i < 79; ++i) {
                bx1 += C2[i] * D2[0][i];
                by1 += C2[i] * D2[1][i];
                bz1 += C2[i] * D2[2][i];
            }
        }

        xi = {x2, y2, z2, ps};
        if (isNorth) {
            double D2[3][79];
            t96ConDip1(xi, D2);
            for (int i = 0; i < 79; ++i) {
                bx2 += C2[i] * D2[0][i];
                by2 += C2[i] * D2[1][i];
                bz2 += C2[i] * D2[2][i];
            }
        } else {
            double D1[3][26];
            t96DipLoop1(xi, D1);
            for (int i = 0; i < 26; ++i) {
                bx2 += C1[i] * D1[0][i];
                by2 += C1[i] * D1[1][i];
                bz2 += C1[i] * D1[2][i];
            }
        }

        double ss = std::sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1));
        double ds = std::sqrt((x - x1)*(x - x1) + (y - y1)*(y - y1) + (z - z1)*(z - z1));
        double frac = ds / ss;

        bx = bx1 * (1.0 - frac) + bx2 * frac;
        by = by1 * (1.0 - frac) + by2 * frac;
        bz = bz1 * (1.0 - frac) + bz2 * frac;
    }

    // Add shielding field
    double bsx, bsy, bsz;
    t96Birk1Shield(ps, x, y, z, &bsx, &bsy, &bsz);

    *Bx = bx + bsx;
    *By = by + bsy;
    *Bz = bz + bsz;
}



// Assuming these structs are globally available or passed in
extern Coord11 coord11;
extern LoopDip1 loopDip1;
extern RHDR rhdr;


/**
 * Calculates derivatives of dependent variables with respect to linear
 * model parameters for the Region 1 field model.
 *
 * @param xi   Input vector {X, Y, Z, PS}
 * @param D    Output 3x26 array of computed derivatives
 */
void t96DipLoop1(const std::array<double, 4>& xi, double D[3][26]) {
    double X = xi[0];
    double Y = xi[1];
    double Z = xi[2];
    double PS = xi[3];
    double SPS = std::sin(PS);

    double RH = rhdr.rh;
    double DR = rhdr.dr;

    // Handle 12 dipoles
    for (int i = 0; i < 12; ++i) {
        double xd_orig = coord11.xx1[i] * loopDip1.dipx;
        double yd_orig = coord11.yy1[i] * loopDip1.dipy;
        double r2 = xd_orig * xd_orig + yd_orig * yd_orig;
        double r = std::sqrt(r2);

        double sqm = std::sqrt((r - RH) * (r - RH) + DR * DR);
        double sqp = std::sqrt((r + RH) * (r + RH) + DR * DR);
        double C = sqp - sqm;
        double Q = std::sqrt((RH + 1.0) * (RH + 1.0) + DR * DR) - std::sqrt((RH - 1.0) * (RH - 1.0) + DR * DR);
        double SPSAS = SPS / r * C / Q;
        double CPSAS = std::sqrt(1.0 - SPSAS * SPSAS);

        double XD = xd_orig * CPSAS;
        double YD = yd_orig;
        double ZD = -xd_orig * SPSAS;

        double BX1X, BY1X, BZ1X, BX1Y, BY1Y, BZ1Y, BX1Z, BY1Z, BZ1Z;
        double BX2X = 0, BY2X = 0, BZ2X = 0, BX2Z = 0, BY2Z = 0, BZ2Z = 0;

        t96DipXYZ(X - XD, Y - YD, Z - ZD, 
                  &BX1X, &BY1X, &BZ1X, &BX1Y, &BY1Y, &BZ1Y, &BX1Z, &BY1Z, &BZ1Z);

        if (std::abs(YD) > 1e-10) {
            t96DipXYZ(X - XD, Y + YD, Z - ZD, 
                      &BX2X, &BY2X, &BZ2X, &BX2Y, &BY2Y, &BZ2Y, &BX2Z, &BY2Z, &BZ2Z);
        }

        D[0][i]     = BX1Z + BX2Z;
        D[1][i]     = BY1Z + BY2Z;
        D[2][i]     = BZ1Z + BZ2Z;

        D[0][i+12]  = (BX1X + BX2X) * SPS;
        D[1][i+12]  = (BY1X + BY2X) * SPS;
        D[2][i+12]  = (BZ1X + BZ2X) * SPS;
    }

    // Handle Octagonal Double Loop 1
    {
        double r2 = std::pow(loopDip1.xcentre[0] + loopDip1.radius[0], 2);
        double r = std::sqrt(r2);

        double sqm = std::sqrt((r - RH) * (r - RH) + DR * DR);
        double sqp = std::sqrt((r + RH) * (r + RH) + DR * DR);
        double C = sqp - sqm;
        double Q = std::sqrt((RH + 1.0) * (RH + 1.0) + DR * DR) - std::sqrt((RH - 1.0) * (RH - 1.0) + DR * DR);
        double SPSAS = SPS / r * C / Q;
        double CPSAS = std::sqrt(1.0 - SPSAS * SPSAS);

        double XOCT1 = X * CPSAS - Z * SPSAS;
        double YOCT1 = Y;
        double ZOCT1 = X * SPSAS + Z * CPSAS;

        double BXOCT1, BYOCT1, BZOCT1;
        t96CrossLoop(XOCT1, YOCT1, ZOCT1, &BXOCT1, &BYOCT1, &BZOCT1, loopDip1.xcentre[0], loopDip1.radius[0], loopDip1.tilt);

        D[0][24] = BXOCT1 * CPSAS + BZOCT1 * SPSAS;
        D[1][24] = BYOCT1;
        D[2][24] = -BXOCT1 * SPSAS + BZOCT1 * CPSAS;
    }

    // Handle Circular Loop 2
    {
        double r2 = std::pow(loopDip1.radius[1] - loopDip1.xcentre[1], 2);
        double r = std::sqrt(r2);

        double sqm = std::sqrt((r - RH) * (r - RH) + DR * DR);
        double sqp = std::sqrt((r + RH) * (r + RH) + DR * DR);
        double C = sqp - sqm;
        double Q = std::sqrt((RH + 1.0) * (RH + 1.0) + DR * DR) - std::sqrt((RH - 1.0) * (RH - 1.0) + DR * DR);
        double SPSAS = SPS / r * C / Q;
        double CPSAS = std::sqrt(1.0 - SPSAS * SPSAS);

        double XOCT2 = X * CPSAS - Z * SPSAS - loopDip1.xcentre[1];
        double YOCT2 = Y;
        double ZOCT2 = X * SPSAS + Z * CPSAS;

        double BX, BY, BZ;
        t96Circle(XOCT2, YOCT2, ZOCT2, loopDip1.radius[1], &BX, &BY, &BZ);

        D[0][25] = BX * CPSAS + BZ * SPSAS;
        D[1][25] = BY;
        D[2][25] = -BX * SPSAS + BZ * CPSAS;
    }
}



/**
 * Computes the magnetic field components (Bx, By, Bz) at point (x, y, z)
 * due to a circular current loop of radius RL.
 *
 * Uses the second-order approximation for elliptic integrals 
 * from Abramowitz and Stegun.
 *
 * @param x    X coordinate (Re)
 * @param y    Y coordinate (Re)
 * @param z    Z coordinate (Re)
 * @param rl   Radius of the current loop (Re)
 * @param Bx   Pointer to store X component of magnetic field (nT)
 * @param By   Pointer to store Y component of magnetic field (nT)
 * @param Bz   Pointer to store Z component of magnetic field (nT)
 */
void t96Circle(double x, double y, double z, double rl, 
               double* Bx, double* By, double* Bz) {
    constexpr double PI = 3.141592654;

    double rho2 = x * x + y * y;
    double rho = std::sqrt(rho2);

    double r22 = z * z + std::pow(rho + rl, 2);
    double r2 = std::sqrt(r22);

    double r12 = r22 - 4.0 * rho * rl;
    double r32 = 0.5 * (r12 + r22);

    double xk2 = 1.0 - r12 / r22;
    double xk2s = 1.0 - xk2;

    double dl = std::log(1.0 / xk2s);

    // Complete elliptic integral of the first kind approximation (K)
    double K = 1.38629436112 
             + xk2s * (0.09666344259 + xk2s * (0.03590092383 
             + xk2s * (0.03742563713 + xk2s * 0.01451196212))) 
             + dl * (0.5 + xk2s * (0.12498593597 + xk2s * (0.06880248576 
             + xk2s * (0.03328355346 + xk2s * 0.00441787012))));

    // Complete elliptic integral of the second kind approximation (E)
    double E = 1.0 
             + xk2s * (0.44325141463 + xk2s * (0.0626060122 
             + xk2s * (0.04757383546 + xk2s * 0.01736506451))) 
             + dl * xk2s * (0.2499836831 + xk2s * (0.09200180037 
             + xk2s * (0.04069697526 + xk2s * 0.00526449639)));

    double brho;

    if (rho > 1e-6) {
        // General case for BRHO
        brho = z / (rho2 * r2) * (r32 / r12 * E - K);
    } else {
        // Special case near axis to avoid singularity
        brho = PI * rl / r2 * (rl - rho) / r12 * z / (r32 - rho2);
    }

    *Bx = brho * x;
    *By = brho * y;
    *Bz = (K - E * (r32 - 2.0 * rl * rl) / r12) / r2;
}


/**
 * Computes the magnetic field components (Bx, By, Bz) from a pair of
 * inclined circular current loops ("crossed loops").
 *
 * @param x   X coordinate of the point (Re)
 * @param y   Y coordinate of the point (Re)
 * @param z   Z coordinate of the point (Re)
 * @param xc  Shift of the loop center along X-axis (Re)
 * @param rl  Radius of the loops (Re)
 * @param al  Inclination angle to the equatorial plane (radians)
 * @param Bx  Pointer to store X component of the magnetic field (nT)
 * @param By  Pointer to store Y component of the magnetic field (nT)
 * @param Bz  Pointer to store Z component of the magnetic field (nT)
 */
void t96CrossLoop(double x, double y, double z, 
                  double* Bx, double* By, double* Bz, 
                  double xc, double rl, double al) {
    double cal = std::cos(al);
    double sal = std::sin(al);

    // Rotate coordinates for the first loop
    double y1 = y * cal - z * sal;
    double z1 = y * sal + z * cal;

    // Rotate coordinates for the second loop
    double y2 = y * cal + z * sal;
    double z2 = -y * sal + z * cal;

    // Field components from each loop
    double bx1, by1, bz1;
    double bx2, by2, bz2;

    t96Circle(x - xc, y1, z1, rl, &bx1, &by1, &bz1);
    t96Circle(x - xc, y2, z2, rl, &bx2, &by2, &bz2);

    // Combine contributions from both loops
    *Bx = bx1 + bx2;
    *By = (by1 + by2) * cal + (bz1 - bz2) * sal;
    *Bz = -(by1 - by2) * sal + (bz1 + bz2) * cal;
}


/**
 * Computes the magnetic field components produced by three orthogonal dipoles
 * aligned along the X, Y, and Z axes, each with moment M = Me.
 *
 * @param x     GSM X coordinate (Re)
 * @param y     GSM Y coordinate (Re)
 * @param z     GSM Z coordinate (Re)
 * @param Bxx   Pointer to store Bx component from X-aligned dipole
 * @param Byx   Pointer to store By component from X-aligned dipole
 * @param Bzx   Pointer to store Bz component from X-aligned dipole
 * @param Bxy   Pointer to store Bx component from Y-aligned dipole
 * @param Byy   Pointer to store By component from Y-aligned dipole
 * @param Bzy   Pointer to store Bz component from Y-aligned dipole
 * @param Bxz   Pointer to store Bx component from Z-aligned dipole
 * @param Byz   Pointer to store By component from Z-aligned dipole
 * @param Bzz   Pointer to store Bz component from Z-aligned dipole
 */
void t96DipXYZ(double x, double y, double z,
               double* Bxx, double* Byx, double* Bzx,
               double* Bxy, double* Byy, double* Bzy,
               double* Bxz, double* Byz, double* Bzz) {
    
    double x2 = x * x;
    double y2 = y * y;
    double z2 = z * z;
    double r2 = x2 + y2 + z2;

    if (r2 == 0.0) {
        // Avoid division by zero if at origin
        *Bxx = *Byx = *Bzx = 0.0;
        *Bxy = *Byy = *Bzy = 0.0;
        *Bxz = *Byz = *Bzz = 0.0;
        return;
    }

    double xmr5 = 30574.0 / (r2 * r2 * std::sqrt(r2));  // Dipole scaling factor ~1/R^5
    double xmr53 = 3.0 * xmr5;

    // Dipole aligned along X-axis
    *Bxx = xmr5 * (3.0 * x2 - r2);
    *Byx = xmr53 * x * y;
    *Bzx = xmr53 * x * z;

    // Dipole aligned along Y-axis
    *Bxy = *Byx;
    *Byy = xmr5 * (3.0 * y2 - r2);
    *Bzy = xmr53 * y * z;

    // Dipole aligned along Z-axis
    *Bxz = *Bzx;
    *Byz = *Bzy;
    *Bzz = xmr5 * (3.0 * z2 - r2);
}



// Assuming these structs are declared elsewhere
extern struct DX1 { double dx, scalein, scaleout; } dx1;
extern struct Coord21 { std::array<double,14> xx, yy, zz; } coord21;


/**
 * Calculates model derivatives with respect to linear parameters for
 * conical harmonics and dipole sources.
 *
 * @param xi   Input vector {X, Y, Z, PS}
 * @param D    Output 3x79 array of computed derivatives
 */
void t96ConDip1(const std::array<double, 4>& xi, double D[3][79]) {
    double X = xi[0];
    double Y = xi[1];
    double Z = xi[2];
    double PS = xi[3];
    double SPS = std::sin(PS);
    double CPS = std::cos(PS);

    double XSM = X * CPS - Z * SPS - dx1.dx;
    double ZSM = Z * CPS + X * SPS;
    double RO2 = XSM * XSM + Y * Y;
    double RO = std::sqrt(RO2);

    std::array<double, 5> CF, SF;
    CF[0] = XSM / RO;
    SF[0] = Y / RO;

    // Generate harmonics
    for (int m = 1; m < 5; ++m) {
        CF[m] = CF[m-1] * CF[0] - SF[m-1] * SF[0];
        SF[m] = SF[m-1] * CF[0] + CF[m-1] * SF[0];
    }

    double R2 = RO2 + ZSM * ZSM;
    double R = std::sqrt(R2);
    double C = ZSM / R;
    double S = RO / R;
    double CH = std::sqrt(0.5 * (1.0 + C));
    double SH = std::sqrt(0.5 * (1.0 - C));
    double TNH = SH / CH;
    double CNH = 1.0 / TNH;

    // Conical harmonics contribution
    for (int m = 1; m <= 5; ++m) {
        double BT = m * CF[m-1] / (R * S) * (std::pow(TNH, m) + std::pow(CNH, m));
        double BF = -0.5 * m * SF[m-1] / R * (std::pow(TNH, m-1) / (CH * CH) - std::pow(CNH, m-1) / (SH * SH));

        double BXSM = BT * C * CF[0] - BF * SF[0];
        double BY = BT * C * SF[0] + BF * CF[0];
        double BZSM = -BT * S;

        D[0][m-1] = BXSM * CPS + BZSM * SPS;
        D[1][m-1] = BY;
        D[2][m-1] = -BXSM * SPS + BZSM * CPS;
    }

    // Dipole contributions for first 9 positions
    for (int i = 0; i < 9; ++i) {
        double scale = (i == 2 || i == 4 || i == 5) ? dx1.scalein : dx1.scaleout;
        double XD = coord21.xx[i] * scale;
        double YD = coord21.yy[i] * scale;
        double ZD = coord21.zz[i];

        double BX1X, BY1X, BZ1X, BX1Y, BY1Y, BZ1Y, BX1Z, BY1Z, BZ1Z;
        double BX2X, BY2X, BZ2X, BX2Y, BY2Y, BZ2Y, BX2Z, BY2Z, BZ2Z;
        double BX3X, BY3X, BZ3X, BX3Y, BY3Y, BZ3Y, BX3Z, BY3Z, BZ3Z;
        double BX4X, BY4X, BZ4X, BX4Y, BY4Y, BZ4Y, BX4Z, BY4Z, BZ4Z;

        t96DipXYZ(XSM - XD, Y - YD, ZSM - ZD, &BX1X, &BY1X, &BZ1X, &BX1Y, &BY1Y, &BZ1Y, &BX1Z, &BY1Z, &BZ1Z);
        t96DipXYZ(XSM - XD, Y + YD, ZSM - ZD, &BX2X, &BY2X, &BZ2X, &BX2Y, &BY2Y, &BZ2Y, &BX2Z, &BY2Z, &BZ2Z);
        t96DipXYZ(XSM - XD, Y - YD, ZSM + ZD, &BX3X, &BY3X, &BZ3X, &BX3Y, &BY3Y, &BZ3Y, &BX3Z, &BY3Z, &BZ3Z);
        t96DipXYZ(XSM - XD, Y + YD, ZSM + ZD, &BX4X, &BY4X, &BZ4X, &BX4Y, &BY4Y, &BZ4Y, &BX4Z, &BY4Z, &BZ4Z);

        int IX = i * 3 + 3;
        int IY = IX + 1;
        int IZ = IY + 1;

        D[0][IX] = (BX1X + BX2X - BX3X - BX4X) * CPS + (BZ1X + BZ2X - BZ3X - BZ4X) * SPS;
        D[1][IX] = BY1X + BY2X - BY3X - BY4X;
        D[2][IX] = (BZ1X + BZ2X - BZ3X - BZ4X) * CPS - (BX1X + BX2X - BX3X - BX4X) * SPS;

        D[0][IY] = (BX1Y - BX2Y - BX3Y + BX4Y) * CPS + (BZ1Y - BZ2Y - BZ3Y + BZ4Y) * SPS;
        D[1][IY] = BY1Y - BY2Y - BY3Y + BY4Y;
        D[2][IY] = (BZ1Y - BZ2Y - BZ3Y + BZ4Y) * CPS - (BX1Y - BX2Y - BX3Y + BX4Y) * SPS;

        D[0][IZ] = (BX1Z + BX2Z + BX3Z + BX4Z) * CPS + (BZ1Z + BZ2Z + BZ3Z + BZ4Z) * SPS;
        D[1][IZ] = BY1Z + BY2Z + BY3Z + BY4Z;
        D[2][IZ] = (BZ1Z + BZ2Z + BZ3Z + BZ4Z) * CPS - (BX1Z + BX2Z + BX3Z + BX4Z) * SPS;

        // Multiply by SPS for next set
        IX += 27;
        IY += 27;
        IZ += 27;

        D[0][IX] = SPS * ((BX1X + BX2X + BX3X + BX4X) * CPS + (BZ1X + BZ2X + BZ3X + BZ4X) * SPS);
        D[1][IX] = SPS * (BY1X + BY2X + BY3X + BY4X);
        D[2][IX] = SPS * ((BZ1X + BZ2X + BZ3X + BZ4X) * CPS - (BX1X + BX2X + BX3X + BX4X) * SPS);

        D[0][IY] = SPS * ((BX1Y - BX2Y + BX3Y - BX4Y) * CPS + (BZ1Y - BZ2Y + BZ3Y - BZ4Y) * SPS);
        D[1][IY] = SPS * (BY1Y - BY2Y + BY3Y - BY4Y);
        D[2][IY] = SPS * ((BZ1Y - BZ2Y + BZ3Y - BZ4Y) * CPS - (BX1Y - BX2Y + BX3Y - BX4Y) * SPS);

        D[0][IZ] = SPS * ((BX1Z + BX2Z - BX3Z - BX4Z) * CPS + (BZ1Z + BZ2Z - BZ3Z - BZ4Z) * SPS);
        D[1][IZ] = SPS * (BY1Z + BY2Z - BY3Z - BY4Z);
        D[2][IZ] = SPS * ((BZ1Z + BZ2Z - BZ3Z - BZ4Z) * CPS - (BX1Z + BX2Z - BX3Z - BX4Z) * SPS);
    }

    // Last 5 dipoles
    for (int i = 0; i < 5; ++i) {
        double ZD = coord21.zz[i + 9];

        double BX1X, BY1X, BZ1X, BX1Y, BY1Y, BZ1Y, BX1Z, BY1Z, BZ1Z;
        double BX2X, BY2X, BZ2X, BX2Y, BY2Y, BZ2Y, BX2Z, BY2Z, BZ2Z;

        t96DipXYZ(XSM, Y, ZSM - ZD, &BX1X, &BY1X, &BZ1X, &BX1Y, &BY1Y, &BZ1Y, &BX1Z, &BY1Z, &BZ1Z);
        t96DipXYZ(XSM, Y, ZSM + ZD, &BX2X, &BY2X, &BZ2X, &BX2Y, &BY2Y, &BZ2Y, &BX2Z, &BY2Z, &BZ2Z);

        int IX = 58 + (i + 1) * 2;
        int IZ = IX + 1;

        D[0][IX] = (BX1X - BX2X) * CPS + (BZ1X - BZ2X) * SPS;
        D[1][IX] = BY1X - BY2X;
        D[2][IX] = (BZ1X - BZ2X) * CPS - (BX1X - BX2X) * SPS;

        D[0][IZ] = (BX1Z + BX2Z) * CPS + (BZ1Z + BZ2Z) * SPS;
        D[1][IZ] = BY1Z + BY2Z;
        D[2][IZ] = (BZ1Z + BZ2Z) * CPS - (BX1Z + BX2Z) * SPS;

        IX += 10;
        IZ += 10;

        D[0][IX] = SPS * ((BX1X + BX2X) * CPS + (BZ1X + BZ2X) * SPS);
        D[1][IX] = SPS * (BY1X + BY2X);
        D[2][IX] = SPS * ((BZ1X + BZ2X) * CPS - (BX1X + BX2X) * SPS);

        D[0][IZ] = SPS * ((BX1Z - BX2Z) * CPS + (BZ1Z - BZ2Z) * SPS);
        D[1][IZ] = SPS * (BY1Z - BY2Z);
        D[2][IZ] = SPS * ((BZ1Z - BZ2Z) * CPS - (BX1Z - BX2Z) * SPS);
    }
}



/**
 * Computes the shielding magnetic field components (Bx, By, Bz)
 * for Region 1 Birkeland currents using box harmonics.
 *
 * @param ps   Dipole tilt angle in radians
 * @param x    GSM X coordinate (Re)
 * @param y    GSM Y coordinate (Re)
 * @param z    GSM Z coordinate (Re)
 * @param Bx   Pointer to store X component of magnetic field (nT)
 * @param By   Pointer to store Y component of magnetic field (nT)
 * @param Bz   Pointer to store Z component of magnetic field (nT)
 */
void t96Birk1Shield(double ps, double x, double y, double z, 
                    double* Bx, double* By, double* Bz) {
    // Initialize A array (80 coefficients)
    const std::array<double, 80> A = {{
        1.174198045, -1.463820502, 4.840161537, -3.674506864,
        82.18368896, -94.94071588, -4122.331796, 4670.278676,
        -21.54975037, 26.72661293, -72.81365728, 44.09887902,
        40.08073706, -51.23563510, 1955.348537, -1940.971550,
        794.0496433, -982.2441344, 1889.837171, -558.9779727,
        -1260.543238, 1260.063802, -293.5942373, 344.7250789,
        -773.7002492, 957.0094135, -1824.143669, 520.7994379,
        1192.484774, -1192.184565, 89.15537624, -98.52042999,
        -0.08168777675, 0.04255969908, 0.3155237661, -0.3841755213,
        2.494553332, -0.06571440817, -2.765661310, 0.4331001908,
        0.1099181537, -0.06154126980, -0.3258649260, 0.6698439193,
        -5.542735524, 0.1604203535, 5.854456934, -0.8323632049,
        3.732608869, -3.130002153, 107.0972607, -32.28483411,
        -115.2389298, 54.45064360, -0.5826853320, -3.582482231,
        -4.046544561, 3.311978102, -104.0839563, 30.26401293,
        97.29109008, -50.62370872, -296.3734955, 127.7872523,
        5.303648988, 10.40368955, 69.65230348, 466.5099509,
        1.645049286, 3.825838190, 11.66675599, 558.9781177,
        1.826531343, 2.066018073, 25.40971369, 990.2795225,
        2.319489258, 4.555148484, 9.691185703, 591.8280358
    }};

    // P1, R1, Q1, S1 mapped via equivalence to A[64] to A[79]
    const double* P1 = &A[64];
    const double* R1 = &A[68];
    const double* Q1 = &A[72];
    const double* S1 = &A[76];

    double RP[4], RR[4], RQ[4], RS[4];

    double cps = std::cos(ps);
    double sps = std::sin(ps);
    double s3ps = 4.0 * cps * cps - 1.0;

    *Bx = 0.0;
    *By = 0.0;
    *Bz = 0.0;

    // Precompute reciprocals
    for (int i = 0; i < 4; ++i) {
        RP[i] = 1.0 / P1[i];
        RR[i] = 1.0 / R1[i];
        RQ[i] = 1.0 / Q1[i];
        RS[i] = 1.0 / S1[i];
    }

    int L = 0;

    for (int m = 1; m <= 2; ++m) {  // m = symmetry type
        for (int i = 0; i < 4; ++i) {
            double cypi = std::cos(y * RP[i]);
            double cyqi = std::cos(y * RQ[i]);
            double sypi = std::sin(y * RP[i]);
            double syqi = std::sin(y * RQ[i]);

            for (int k = 0; k < 4; ++k) {
                double szrk = std::sin(z * RR[k]);
                double czsk = std::cos(z * RS[k]);
                double czrk = std::cos(z * RR[k]);
                double szsk = std::sin(z * RS[k]);

                double sqpr = std::sqrt(RP[i]*RP[i] + RR[k]*RR[k]);
                double sqqs = std::sqrt(RQ[i]*RQ[i] + RS[k]*RS[k]);

                double epr = std::exp(x * sqpr);
                double eqs = std::exp(x * sqqs);

                for (int n = 1; n <= 2; ++n) {
                    double hx, hy, hz;

                    if (m == 1) {  // Perpendicular symmetry
                        if (n == 1) {
                            hx = -sqpr * epr * cypi * szrk;
                            hy = RP[i] * epr * sypi * szrk;
                            hz = -RR[k] * epr * cypi * czrk;
                        } else {
                            hx *= cps;
                            hy *= cps;
                            hz *= cps;
                        }
                    } else {  // Parallel symmetry
                        if (n == 1) {
                            hx = -sps * sqqs * eqs * cyqi * czsk;
                            hy = sps * RQ[i] * eqs * syqi * czsk;
                            hz = sps * RS[k] * eqs * cyqi * szsk;
                        } else {
                            hx *= s3ps;
                            hy *= s3ps;
                            hz *= s3ps;
                        }
                    }

                    *Bx += A[L] * hx;
                    *By += A[L] * hy;
                    *Bz += A[L] * hz;
                    ++L;
                }
            }
        }
    }
}

/**
 * Combines the magnetic field contributions from the Region 2 Birkeland 
 * current shielding field and the Region 2 field itself.
 *
 * @param ps   Dipole tilt angle in radians
 * @param x    GSM X coordinate (Re)
 * @param y    GSM Y coordinate (Re)
 * @param z    GSM Z coordinate (Re)
 * @param Bx   Pointer to store X component of the magnetic field (nT)
 * @param By   Pointer to store Y component of the magnetic field (nT)
 * @param Bz   Pointer to store Z component of the magnetic field (nT)
 */
void t96Birk2Tot_02(double ps, double x, double y, double z,
                    double* Bx, double* By, double* Bz) 
{
    double Wx, Wy, Wz;
    double Hx, Hy, Hz;

    // Compute shielding field contribution
    t96Birk2Shield(x, y, z, ps, &Wx, &Wy, &Wz);

    // Compute Region 2 Birkeland current contribution
    t96R2Birk(x, y, z, ps, &Hx, &Hy, &Hz);

    // Sum both contributions
    *Bx = Wx + Hx;
    *By = Wy + Hy;
    *Bz = Wz + Hz;

    // Debug print if needed
    // std::cout << "Wx=" << Wx << " Wy=" << Wy << " Wz=" << Wz 
    //           << " Hx=" << Hx << " Hy=" << Hy << " Hz=" << Hz << std::endl;
}



/**
 * Computes the Region 2 Birkeland current shielding magnetic field components.
 *
 * @param x    GSM X coordinate (Re)
 * @param y    GSM Y coordinate (Re)
 * @param z    GSM Z coordinate (Re)
 * @param ps   Dipole tilt angle in radians
 * @param Hx   Pointer to store X component of shielding field (nT)
 * @param Hy   Pointer to store Y component of shielding field (nT)
 * @param Hz   Pointer to store Z component of shielding field (nT)
 */
void t96Birk2Shield(double x, double y, double z, double ps,
                    double* Hx, double* Hy, double* Hz)
{
    static const std::array<double, 24> A = {
        -111.6371348, 124.5402702, 110.3735178, -122.0095905,
        111.9448247, -129.1957743, -110.7586562, 126.5649012,
        -0.7865034384, -0.2483462721, 0.8026023894, 0.2531397188,
        10.72890902, 0.8483902118, -10.96884315, -0.8583297219,
        13.85650567, 14.90554500, 10.21914434, 10.09021632,
        6.340382460, 14.40432686, 12.71023437, 12.83966657
    };

    std::array<double, 2> P = { A[16], A[17] };
    std::array<double, 2> R = { A[18], A[19] };
    std::array<double, 2> Q = { A[20], A[21] };
    std::array<double, 2> S = { A[22], A[23] };

    double CPS = std::cos(ps);
    double SPS = std::sin(ps);
    double S3PS = 4.0 * CPS * CPS - 1.0;  // Equivalent to sin(3*PS)/sin(PS)

    double hx = 0.0, hy = 0.0, hz = 0.0;
    int L = 0;

    for (int m = 0; m < 2; ++m) {  // m = 0: Perpendicular, m = 1: Parallel symmetry
        for (int i = 0; i < 2; ++i) {
            double CYPI = std::cos(y / P[i]);
            double CYQI = std::cos(y / Q[i]);
            double SYPI = std::sin(y / P[i]);
            double SYQI = std::sin(y / Q[i]);

            for (int k = 0; k < 2; ++k) {
                double SZRK = std::sin(z / R[k]);
                double CZSK = std::cos(z / S[k]);
                double CZRK = std::cos(z / R[k]);
                double SZSK = std::sin(z / S[k]);

                double SQPR = std::sqrt(1.0 / (P[i] * P[i]) + 1.0 / (R[k] * R[k]));
                double SQQS = std::sqrt(1.0 / (Q[i] * Q[i]) + 1.0 / (S[k] * S[k]));

                double EPR = std::exp(x * SQPR);
                double EQS = std::exp(x * SQQS);

                for (int n = 0; n < 2; ++n) {
                    double DX, DY, DZ;

                    if (m == 0) {  // Perpendicular symmetry
                        if (n == 0) {
                            DX = -SQPR * EPR * CYPI * SZRK;
                            DY = EPR / P[i] * SYPI * SZRK;
                            DZ = -EPR / R[k] * CYPI * CZRK;
                        } else {
                            DX *= CPS;
                            DY *= CPS;
                            DZ *= CPS;
                        }
                    } else {  // Parallel symmetry
                        if (n == 0) {
                            DX = -SPS * SQQS * EQS * CYQI * CZSK;
                            DY = SPS * EQS / Q[i] * SYQI * CZSK;
                            DZ = SPS * EQS / S[k] * CYQI * SZSK;
                        } else {
                            DX *= S3PS;
                            DY *= S3PS;
                            DZ *= S3PS;
                        }
                    }

                    hx += A[L] * DX;
                    hy += A[L] * DY;
                    hz += A[L] * DZ;
                    L++;
                }
            }
        }
    }

    *Hx = hx;
    *Hy = hy;
    *Hz = hz;
}


/**
 * Computes the Region 2 Birkeland current / partial ring current magnetic field (no shielding).
 *
 * @param x     GSM X coordinate (Re)
 * @param y     GSM Y coordinate (Re)
 * @param z     GSM Z coordinate (Re)
 * @param ps    Dipole tilt angle in radians
 * @param Bx    Pointer to store X component of magnetic field (nT)
 * @param By    Pointer to store Y component of magnetic field (nT)
 * @param Bz    Pointer to store Z component of magnetic field (nT)
 */
void t96R2Birk(double x, double y, double z, double ps,
               double* Bx, double* By, double* Bz)
{
    static double prev_ps = 10.0;
    static double cps = 0.0;
    static double sps = 0.0;

    const double DELARG  = 0.030;
    const double DELARG1 = 0.015;

    // Update cached sine and cosine if PS changes
    if (std::abs(prev_ps - ps) > 1e-10) {
        prev_ps = ps;
        cps = std::cos(ps);
        sps = std::sin(ps);
    }

    // Rotate to SM coordinates
    double xsm = x * cps - z * sps;
    double zsm = z * cps + x * sps;

    double xks = xksi(xsm, y, zsm);

    double bxsm = 0.0, by = 0.0, bzsm = 0.0;

    if (xks < -(DELARG + DELARG1)) {
        t96R2Outer(xsm, y, zsm, &bxsm, &by, &bzsm);
        bxsm *= -0.02;
        by   *= -0.02;
        bzsm *= -0.02;
    }
    else if (xks >= -(DELARG + DELARG1) && xks < -DELARG + DELARG1) {
        double bxsm1, by1, bzsm1, bxsm2, by2, bzsm2;
        t96R2Outer(xsm, y, zsm, &bxsm1, &by1, &bzsm1);
        t96R2Sheet(xsm, y, zsm, &bxsm2, &by2, &bzsm2);
        double f2 = -0.02 * tksi(xks, -DELARG, DELARG1);
        double f1 = -0.02 - f2;
        bxsm = bxsm1 * f1 + bxsm2 * f2;
        by   = by1   * f1 + by2   * f2;
        bzsm = bzsm1 * f1 + bzsm2 * f2;
    }
    else if (xks >= -DELARG + DELARG1 && xks < DELARG - DELARG1) {
        t96R2Sheet(xsm, y, zsm, &bxsm, &by, &bzsm);
        bxsm *= -0.02;
        by   *= -0.02;
        bzsm *= -0.02;
    }
    else if (xks >= DELARG - DELARG1 && xks < DELARG + DELARG1) {
        double bxsm1, by1, bzsm1, bxsm2, by2, bzsm2;
        t96R2Inner(xsm, y, zsm, &bxsm1, &by1, &bzsm1);
        t96R2Sheet(xsm, y, zsm, &bxsm2, &by2, &bzsm2);
        double f1 = -0.02 * tksi(xks, DELARG, DELARG1);
        double f2 = -0.02 - f1;
        bxsm = bxsm1 * f1 + bxsm2 * f2;
        by   = by1   * f1 + by2   * f2;
        bzsm = bzsm1 * f1 + bzsm2 * f2;
    }
    else if (xks >= DELARG + DELARG1) {
        t96R2Inner(xsm, y, zsm, &bxsm, &by, &bzsm);
        bxsm *= -0.02;
        by   *= -0.02;
        bzsm *= -0.02;
    }

    // Rotate back from SM to GSM coordinates
    *Bx = bxsm * cps + bzsm * sps;
    *Bz = bzsm * cps - bxsm * sps;
    *By = by;
}

/**
 * Computes the magnetic field components for the inner part of Region 2 field-aligned currents.
 */
void t96R2Inner(double x, double y, double z, double* Bx, double* By, double* Bz) {
    // Coefficients from Fortran DATA statements
    const double PL[8] = {154.185, -2.12446, 0.0601735, -0.00153954, 0.0000355077, 
                          29.9996, 262.886, 99.9132};

    const double PN[8] = {-8.1902, 6.5239, 5.504, 7.7815, 0.8573, 3.0986, 0.0774, -0.038};

    // Arrays for conical harmonics
    std::vector<double> cbx(5), cby(5), cbz(5);

    // Compute conical harmonics
    t96BConic(x, y, z, cbx, cby, cbz, 5);

    // Compute 4-loop current system contribution
    double dbx8, dby8, dbz8;
    t96Loops4(x, y, z, &dbx8, &dby8, &dbz8, PN[0], PN[1], PN[2], PN[3], PN[4], PN[5]);

    // Compute dipole distribution contributions
    double dbx6, dby6, dbz6;
    double dbx7, dby7, dbz7;

    t96DipDistr(x - PN[6], y, z, &dbx6, &dby6, &dbz6, 0);
    t96DipDistr(x - PN[7], y, z, &dbx7, &dby7, &dbz7, 1);

    // Combine all contributions to get total field components
    *Bx = PL[0]*cbx[0] + PL[1]*cbx[1] + PL[2]*cbx[2] + PL[3]*cbx[3] + PL[4]*cbx[4]
        + PL[5]*dbx6 + PL[6]*dbx7 + PL[7]*dbx8;

    *By = PL[0]*cby[0] + PL[1]*cby[1] + PL[2]*cby[2] + PL[3]*cby[3] + PL[4]*cby[4]
        + PL[5]*dby6 + PL[6]*dby7 + PL[7]*dby8;

    *Bz = PL[0]*cbz[0] + PL[1]*cbz[1] + PL[2]*cbz[2] + PL[3]*cbz[3] + PL[4]*cbz[4]
        + PL[5]*dbz6 + PL[6]*dbz7 + PL[7]*dbz8;
}


/**
 * Computes conical harmonics magnetic field components (CBX, CBY, CBZ)
 * up to order NMAX at position (x, y, z) in GSM coordinates.
 *
 * @param x     X coordinate (Re)
 * @param y     Y coordinate (Re)
 * @param z     Z coordinate (Re)
 * @param CBX   Vector to store X components [size NMAX]
 * @param CBY   Vector to store Y components [size NMAX]
 * @param CBZ   Vector to store Z components [size NMAX]
 * @param NMAX  Number of harmonic terms
 */
void t96BConic(double x, double y, double z, 
               std::vector<double>& CBX, 
               std::vector<double>& CBY, 
               std::vector<double>& CBZ, 
               int NMAX) {
    
    double ro2 = x * x + y * y;
    double ro = std::sqrt(ro2);

    if (ro == 0.0) {
        // Avoid division by zero at the Z-axis
        CBX.assign(NMAX, 0.0);
        CBY.assign(NMAX, 0.0);
        CBZ.assign(NMAX, 0.0);
        return;
    }

    double cf = x / ro;
    double sf = y / ro;

    double cfm1 = 1.0;
    double sfm1 = 0.0;

    double r2 = ro2 + z * z;
    double r = std::sqrt(r2);

    double c = z / r;
    double s = ro / r;

    double ch = std::sqrt(0.5 * (1.0 + c));
    double sh = std::sqrt(0.5 * (1.0 - c));

    double tnhm1 = 1.0;
    double cnhm1 = 1.0;

    double tnh = sh / ch;
    double cnh = 1.0 / tnh;

    // Resize vectors if needed
    if (CBX.size() != NMAX) CBX.resize(NMAX);
    if (CBY.size() != NMAX) CBY.resize(NMAX);
    if (CBZ.size() != NMAX) CBZ.resize(NMAX);

    for (int m = 1; m <= NMAX; ++m) {
        double cfm = cfm1 * cf - sfm1 * sf;
        double sfm = cfm1 * sf + sfm1 * cf;

        cfm1 = cfm;
        sfm1 = sfm;

        double tnhm = tnhm1 * tnh;
        double cnhm = cnhm1 * cnh;

        double bt = m * cfm / (r * s) * (tnhm + cnhm);
        double bf = -0.5 * m * sfm / r * (tnhm1 / (ch * ch) - cnhm1 / (sh * sh));

        tnhm1 = tnhm;
        cnhm1 = cnhm;

        CBX[m - 1] = bt * c * cf - bf * sf;
        CBY[m - 1] = bt * c * sf + bf * cf;
        CBZ[m - 1] = -bt * s;
    }
}


/**
 * Computes the magnetic field components (Bx, By, Bz) from a linear distribution 
 * of dipolar sources along the Z-axis, as used in the Tsyganenko 1996 (T96) model.
 *
 * @param x   X coordinate in GSM (Re)
 * @param y   Y coordinate in GSM (Re)
 * @param z   Z coordinate in GSM (Re)
 * @param mode  Determines dipole strength variation:
 *              0 = Step-function distribution
 *              1 = Linear variation
 * @param Bx  Pointer to store X component of the magnetic field (nT)
 * @param By  Pointer to store Y component of the magnetic field (nT)
 * @param Bz  Pointer to store Z component of the magnetic field (nT)
 */
void t96DipDistr(double x, double y, double z, double* Bx, double* By, double* Bz, int mode) {
    double x2 = x * x;
    double rho2 = x2 + y * y;
    double r2 = rho2 + z * z;

    if (rho2 == 0.0) {
        // Avoid division by zero if point is exactly on Z-axis
        *Bx = 0.0;
        *By = 0.0;
        *Bz = 0.0;
        return;
    }

    if (mode == 0) {
        double r3 = r2 * std::sqrt(r2);
        *Bx = z / (rho2 * rho2) * (r2 * (y * y - x2) - rho2 * x2) / r3;
        *By = -x * y * z / (rho2 * rho2) * (2.0 * r2 + rho2) / r3;
        *Bz = x / r3;
    } else {
        *Bx = z / (rho2 * rho2) * (y * y - x2);
        *By = -2.0 * x * y * z / (rho2 * rho2);
        *Bz = x / rho2;
    }
}


/**
 * Calculates the magnetic field components for the outer Region 2 Birkeland current system.
 *
 * @param x   GSM X coordinate (Re)
 * @param y   GSM Y coordinate (Re)
 * @param z   GSM Z coordinate (Re)
 * @param Bx  Pointer to store X component of magnetic field (nT)
 * @param By  Pointer to store Y component of magnetic field (nT)
 * @param Bz  Pointer to store Z component of magnetic field (nT)
 */
void t96R2Outer(double x, double y, double z, double* Bx, double* By, double* Bz) {
    // Linear coefficients
    const std::array<double, 5> PL = {-34.105, -2.00019, 628.639, 73.4847, 12.5162};

    // Non-linear parameters for loops
    const std::array<double, 17> PN = {
        0.55, 0.694, 0.0031,   // Crossed loop 1
        1.55, 2.8, 0.1375,     // Crossed loop 2
       -0.7,  0.2, 0.9625,     // Crossed loop 3
       -2.994, 2.925,          // Circle loop (X-shift, radius)
       -1.775, 4.3, -0.275,    // 4-loop system params
        2.7, 0.4312, 1.55
    };

    double dbx1, dby1, dbz1;
    double dbx2, dby2, dbz2;
    double dbx3, dby3, dbz3;
    double dbx4, dby4, dbz4;
    double dbx5, dby5, dbz5;

    // Three pairs of crossed loops
    t96CrossLoop(x, y, z, &dbx1, &dby1, &dbz1, PN[0], PN[1], PN[2]);
    t96CrossLoop(x, y, z, &dbx2, &dby2, &dbz2, PN[3], PN[4], PN[5]);
    t96CrossLoop(x, y, z, &dbx3, &dby3, &dbz3, PN[6], PN[7], PN[8]);

    // Equatorial loop on the nightside
    t96Circle(x - PN[9], y, z, PN[10], &dbx4, &dby4, &dbz4);

    // 4-loop system on the nightside
    t96Loops4(x, y, z, &dbx5, &dby5, &dbz5, PN[11], PN[12], PN[13], PN[14], PN[15], PN[16]);

    // Combine field components
    *Bx = PL[0]*dbx1 + PL[1]*dbx2 + PL[2]*dbx3 + PL[3]*dbx4 + PL[4]*dbx5;
    *By = PL[0]*dby1 + PL[1]*dby2 + PL[2]*dby3 + PL[3]*dby4 + PL[4]*dby5;
    *Bz = PL[0]*dbz1 + PL[1]*dbz2 + PL[2]*dbz3 + PL[3]*dbz4 + PL[4]*dbz5;
}



/**
 * Calculates the magnetic field components from a system of 4 symmetric current loops.
 *
 * @param x    GSM X coordinate (Re)
 * @param y    GSM Y coordinate (Re)
 * @param z    GSM Z coordinate (Re)
 * @param Bx   Pointer to store X component of magnetic field (nT)
 * @param By   Pointer to store Y component of magnetic field (nT)
 * @param Bz   Pointer to store Z component of magnetic field (nT)
 * @param xc   X coordinate of center of 1st quadrant loop
 * @param yc   Y coordinate of center of 1st quadrant loop (yc > 0)
 * @param zc   Z coordinate of center of 1st quadrant loop (zc > 0)
 * @param r    Radius of the loops (same for all four)
 * @param theta Orientation angle (radians)
 * @param phi   Orientation angle (radians)
 */
void t96Loops4(double x, double y, double z,
               double* Bx, double* By, double* Bz,
               double xc, double yc, double zc,
               double r, double theta, double phi) {
    
    double ct = cos(theta);
    double st = sin(theta);
    double cp = cos(phi);
    double sp = sin(phi);

    double bx_total = 0.0, by_total = 0.0, bz_total = 0.0;

    for (int quadrant = 1; quadrant <= 4; ++quadrant) {
        double xs, yss, zs;
        if (quadrant == 1) {
            xs  =  (x - xc) * cp + (y - yc) * sp;
            yss =  (y - yc) * cp - (x - xc) * sp;
            zs  =  z - zc;
        } else if (quadrant == 2) {
            xs  =  (x - xc) * cp - (y + yc) * sp;
            yss =  (y + yc) * cp + (x - xc) * sp;
            zs  =  z - zc;
        } else if (quadrant == 3) {
            xs  = -(x - xc) * cp + (y + yc) * sp;
            yss = -(y + yc) * cp - (x - xc) * sp;
            zs  =  z + zc;
        } else { // quadrant == 4
            xs  = -(x - xc) * cp - (y - yc) * sp;
            yss = -(y - yc) * cp + (x - xc) * sp;
            zs  =  z + zc;
        }

        double xss = xs * ct - zs * st;
        double zss = zs * ct + xs * st;

        double bxss, bys, bzss;
        t96Circle(xss, yss, zss, r, &bxss, &bys, &bzss);

        double bxs = bxss * ct + bzss * st;
        double bxq, byq, bzq;

        switch (quadrant) {
            case 1:
                bzq = bzss * ct - bxss * st;
                bxq =  bxs * cp - bys * sp;
                byq =  bxs * sp + bys * cp;
                break;
            case 2:
                bzq = bzss * ct - bxss * st;
                bxq =  bxs * cp + bys * sp;
                byq = -bxs * sp + bys * cp;
                break;
            case 3:
                bzq = bzss * ct - bxss * st;
                bxq = -bxs * cp - bys * sp;
                byq =  bxs * sp - bys * cp;
                break;
            case 4:
                bzq = bzss * ct - bxss * st;
                bxq = -bxs * cp + bys * sp;
                byq = -bxs * sp - bys * cp;
                break;
        }

        bx_total += bxq;
        by_total += byq;
        bz_total += bzq;
    }

    *Bx = bx_total;
    *By = by_total;
    *Bz = bz_total;
}


void t96R2Sheet(double x, double y, double z, double* Bx, double* By, double* Bz) {
    static const double PNONX[] = {
        19.0969, -9.28828, -0.129687, 5.58594,
        22.5055, 0.48375e-01, 0.396953e-01, 0.579023e-01
    };
    static const double PNONY[] = {
        -13.6750, -6.70625, 2.31875, 11.4062,
        20.4562, 0.478750e-01, 0.363750e-01, 0.567500e-01
    };
    static const double PNONZ[] = {
        -16.7125, -16.4625, -0.1625, 5.1, 
        23.7125, 0.355625e-01, 0.318750e-01, 0.53875e-01
    };
    
    const double A[80] = {
        8.07190, -7.39582, -7.62341, 0.684671, -13.5672, 11.6681, 13.1154, -0.890217,
        7.78726, -5.38346, -8.08738, 0.609385, -2.70410, 3.53741, 3.15549, -1.11069,
       -8.47555, 0.278122, 2.73514, 4.55625, 13.1134, 1.15848, -3.52648, -8.24698,
       -6.85710, -2.81369, 2.03795, 4.64383, 2.49309, -1.22041, -1.67432, -0.422526,
       -5.39796, 7.10326, 5.53730, -13.1918, 4.67853, -7.60329, -2.53066, 7.76338,
        5.60165, 5.34816, -4.56441, 7.05976, -2.62723, -0.529078, 1.42019, -2.93919,
       55.6338, -1.55181, 39.8311, -80.6561, -46.9655, 32.8925, -6.32296, 19.7841,
       124.731, 10.4347, -30.7581, 102.680, -47.4037, -3.31278, 9.37141, -50.0268,
      -533.319, 110.426, 1000.20, -1051.40, 1619.48, 589.855, -1462.73, 1087.10,
      -1994.73, -1654.12, 1263.33, -260.210, 1424.84, 1255.71, -956.733, 219.946
   };
   
   const double B[80] = {
       -9.08427, 10.6777, 10.3288, -0.969987, 6.45257, -8.42508, -7.97464, 1.41996,
       -1.92490, 3.93575, 2.83283, -1.48621, 0.244033, -0.757941, -0.386557, 0.344566,
        9.56674, -2.5365, -3.32916, -5.86712, -6.19625, 1.83879, 2.52772, 4.34417,
        1.87268, -2.13213, -1.69134, -0.176379, -0.261359, 0.566419, 0.3138, -0.134699,
       -3.83086, -8.4154, 4.77005, -9.31479, 37.5715, 19.3992, -17.9582, 36.4604,
      -14.9993, -3.1442, 6.17409, -15.5519, 2.28621, -0.00891549, -0.462912, 2.47314,
       41.7555, 208.614, -45.7861, -77.8687, 239.357, -67.9226, 66.8743, 238.534,
      -112.136, 16.2069, -40.4706, -134.328, 21.56, -0.201725, 2.21, 32.5855,
      -108.217, -1005.98, 585.753, 323.668, -817.056, 235.750, -560.965, -576.892,
       684.193, 85.0275, 168.394, 477.776, -289.253, -123.216, 75.6501, -178.605
   };
   
   const double C[80] = {
      1167.61, -917.782, -1253.2, -274.128, -1538.75, 1257.62, 1745.07, 113.479,
       393.326, -426.858, -641.1, 190.833, -29.9435, -1.04881, 117.125, -25.7663,
      -1168.16, 910.247, 1239.31, 289.515, 1540.56, -1248.29, -1727.61, -131.785,
      -394.577, 426.163, 637.422, -187.965, 30.0348, 0.221898, -116.68, 26.0291,
        12.6804, 4.84091, 1.18166, -2.75946, -17.9822, -6.80357, -1.47134, 3.02266,
         4.79648, 0.665255, -0.256229, -0.0857282, -0.588997, 0.0634812, 0.164303, -0.15285,
        22.2524, -22.4376, -3.85595, 6.07625, -105.959, -41.6698, 0.378615, 1.55958,
        44.3981, 18.8521, 3.19466, 5.89142, -8.63227, -2.36418, -1.027, -2.31515,
      1035.38, 2040.66, -131.881, -744.533, -3274.93, -4845.61, 482.438, 1567.43,
      1354.02, 2040.47, -151.653, -845.012, -111.723, -265.343, -26.1171, 216.632
   };


   
    double xks = xksi(x, y, z);  // Variation across the current sheet

    // Compute T1, T2, T3 terms for X, Y, Z
    double t1x = xks / std::sqrt(xks * xks + PNONX[5] * PNONX[5]);
    double t2x = std::pow(PNONX[6], 3) / std::pow(std::sqrt(xks * xks + PNONX[6] * PNONX[6]), 3);
    double t3x = xks / std::pow(std::sqrt(xks * xks + PNONX[7] * PNONX[7]), 5) * 3.493856 * std::pow(PNONX[7], 4);

    double t1y = xks / std::sqrt(xks * xks + PNONY[5] * PNONY[5]);
    double t2y = std::pow(PNONY[6], 3) / std::pow(std::sqrt(xks * xks + PNONY[6] * PNONY[6]), 3);
    double t3y = xks / std::pow(std::sqrt(xks * xks + PNONY[7] * PNONY[7]), 5) * 3.493856 * std::pow(PNONY[7], 4);

    double t1z = xks / std::sqrt(xks * xks + PNONZ[5] * PNONZ[5]);
    double t2z = std::pow(PNONZ[6], 3) / std::pow(std::sqrt(xks * xks + PNONZ[6] * PNONZ[6]), 3);
    double t3z = xks / std::pow(std::sqrt(xks * xks + PNONZ[7] * PNONZ[7]), 5) * 3.493856 * std::pow(PNONZ[7], 4);

    // Cylindrical and spherical coordinates
    double rho2 = x * x + y * y;
    double r = std::sqrt(rho2 + z * z);
    double rho = std::sqrt(rho2);

    // Azimuthal angle terms
    double c1p = x / rho;
    double s1p = y / rho;
    double s2p = 2.0 * s1p * c1p;
    double c2p = c1p * c1p - s1p * s1p;
    double s3p = s2p * c1p + c2p * s1p;
    double c3p = c2p * c1p - s2p * s1p;
    double s4p = s3p * c1p + c3p * s1p;

    double ct = z / r;
    double st = rho / r;

    // Compute S1 to S5 for BX using PNONX
    double s1 = fexp(ct, PNONX[0]);
    double s2 = fexp(ct, PNONX[1]);
    double s3 = fexp(ct, PNONX[2]);
    double s4 = fexp(ct, PNONX[3]);
    double s5 = fexp(ct, PNONX[4]);

    // Compute BX component
    *Bx = 
        s1 * ((A[0] + A[1]*t1x + A[2]*t2x + A[3]*t3x)
            + c1p * (A[4] + A[5]*t1x + A[6]*t2x + A[7]*t3x)
            + c2p * (A[8] + A[9]*t1x + A[10]*t2x + A[11]*t3x)
            + c3p * (A[12] + A[13]*t1x + A[14]*t2x + A[15]*t3x))
        + s2 * ((A[16] + A[17]*t1x + A[18]*t2x + A[19]*t3x)
            + c1p * (A[20] + A[21]*t1x + A[22]*t2x + A[23]*t3x)
            + c2p * (A[24] + A[25]*t1x + A[26]*t2x + A[27]*t3x)
            + c3p * (A[28] + A[29]*t1x + A[30]*t2x + A[31]*t3x))
        + s3 * ((A[32] + A[33]*t1x + A[34]*t2x + A[35]*t3x)
            + c1p * (A[36] + A[37]*t1x + A[38]*t2x + A[39]*t3x)
            + c2p * (A[40] + A[41]*t1x + A[42]*t2x + A[43]*t3x)
            + c3p * (A[44] + A[45]*t1x + A[46]*t2x + A[47]*t3x))
        + s4 * ((A[48] + A[49]*t1x + A[50]*t2x + A[51]*t3x)
            + c1p * (A[52] + A[53]*t1x + A[54]*t2x + A[55]*t3x)
            + c2p * (A[56] + A[57]*t1x + A[58]*t2x + A[59]*t3x)
            + c3p * (A[60] + A[61]*t1x + A[62]*t2x + A[63]*t3x))
        + s5 * ((A[64] + A[65]*t1x + A[66]*t2x + A[67]*t3x)
            + c1p * (A[68] + A[69]*t1x + A[70]*t2x + A[71]*t3x)
            + c2p * (A[72] + A[73]*t1x + A[74]*t2x + A[75]*t3x)
            + c3p * (A[76] + A[77]*t1x + A[78]*t2x + A[79]*t3x));

    // Repeat for BY using PNONY and B array
    s1 = fexp(ct, PNONY[0]);
    s2 = fexp(ct, PNONY[1]);
    s3 = fexp(ct, PNONY[2]);
    s4 = fexp(ct, PNONY[3]);
    s5 = fexp(ct, PNONY[4]);

    *By = 
        s1 * (s1p*(B[0]+B[1]*t1y+B[2]*t2y+B[3]*t3y)
            + s2p*(B[4]+B[5]*t1y+B[6]*t2y+B[7]*t3y)
            + s3p*(B[8]+B[9]*t1y+B[10]*t2y+B[11]*t3y)
            + s4p*(B[12]+B[13]*t1y+B[14]*t2y+B[15]*t3y))
        + s2 * (s1p*(B[16]+B[17]*t1y+B[18]*t2y+B[19]*t3y)
            + s2p*(B[20]+B[21]*t1y+B[22]*t2y+B[23]*t3y)
            + s3p*(B[24]+B[25]*t1y+B[26]*t2y+B[27]*t3y)
            + s4p*(B[28]+B[29]*t1y+B[30]*t2y+B[31]*t3y))
        + s3 * (s1p*(B[32]+B[33]*t1y+B[34]*t2y+B[35]*t3y)
            + s2p*(B[36]+B[37]*t1y+B[38]*t2y+B[39]*t3y)
            + s3p*(B[40]+B[41]*t1y+B[42]*t2y+B[43]*t3y)
            + s4p*(B[44]+B[45]*t1y+B[46]*t2y+B[47]*t3y))
        + s4 * (s1p*(B[48]+B[49]*t1y+B[50]*t2y+B[51]*t3y)
            + s2p*(B[52]+B[53]*t1y+B[54]*t2y+B[55]*t3y)
            + s3p*(B[56]+B[57]*t1y+B[58]*t2y+B[59]*t3y)
            + s4p*(B[60]+B[61]*t1y+B[62]*t2y+B[63]*t3y))
        + s5 * (s1p*(B[64]+B[65]*t1y+B[66]*t2y+B[67]*t3y)
            + s2p*(B[68]+B[69]*t1y+B[70]*t2y+B[71]*t3y)
            + s3p*(B[72]+B[73]*t1y+B[74]*t2y+B[75]*t3y)
            + s4p*(B[76]+B[77]*t1y+B[78]*t2y+B[79]*t3y));

    // Finally, BZ using PNONZ and C array with fexp1
    s1 = fexp1(ct, PNONZ[0]);
    s2 = fexp1(ct, PNONZ[1]);
    s3 = fexp1(ct, PNONZ[2]);
    s4 = fexp1(ct, PNONZ[3]);
    s5 = fexp1(ct, PNONZ[4]);

    *Bz = 
        s1 * ((C[0]+C[1]*t1z+C[2]*t2z+C[3]*t3z)
            + c1p*(C[4]+C[5]*t1z+C[6]*t2z+C[7]*t3z)
            + c2p*(C[8]+C[9]*t1z+C[10]*t2z+C[11]*t3z)
            + c3p*(C[12]+C[13]*t1z+C[14]*t2z+C[15]*t3z))
        + s2 * ((C[16]+C[17]*t1z+C[18]*t2z+C[19]*t3z)
            + c1p*(C[20]+C[21]*t1z+C[22]*t2z+C[23]*t3z)
            + c2p*(C[24]+C[25]*t1z+C[26]*t2z+C[27]*t3z)
            + c3p*(C[28]+C[29]*t1z+C[30]*t2z+C[31]*t3z))
        + s3 * ((C[32]+C[33]*t1z+C[34]*t2z+C[35]*t3z)
            + c1p*(C[36]+C[37]*t1z+C[38]*t2z+C[39]*t3z)
            + c2p*(C[40]+C[41]*t1z+C[42]*t2z+C[43]*t3z)
            + c3p*(C[44]+C[45]*t1z+C[46]*t2z+C[47]*t3z))
        + s4 * ((C[48]+C[49]*t1z+C[50]*t2z+C[51]*t3z)
            + c1p*(C[52]+C[53]*t1z+C[54]*t2z+C[55]*t3z)
            + c2p*(C[56]+C[57]*t1z+C[58]*t2z+C[59]*t3z)
            + c3p*(C[60]+C[61]*t1z+C[62]*t2z+C[63]*t3z))
        + s5 * ((C[64]+C[65]*t1z+C[66]*t2z+C[67]*t3z)
            + c1p*(C[68]+C[69]*t1z+C[70]*t2z+C[71]*t3z)
            + c2p*(C[72]+C[73]*t1z+C[74]*t2z+C[75]*t3z)
            + c3p*(C[76]+C[77]*t1z+C[78]*t2z+C[79]*t3z));
}
   

/*
 * Function: xksi
 * ------------------------
 * Computes a scalar coordinate transformation used in the Tsyganenko 1996 (T96)
 * geomagnetic field model.
 *
 * This function is part of the "stretching" system of coordinates that maps
 * dipole field lines into a stretched tail-like geometry, better reflecting
 * the real shape of the magnetosphere.
 *
 * Inputs:
 *   x, y, z : GSM position coordinates in Earth radii (Re)
 *
 * Returns:
 *   XKSI : a transformed scalar that incorporates stretching of coordinates 
 *          based on the position vector and empirical parameters. It reflects
 *          the difference between a geometrical scaling factor (α) and an angular
 *          factor (ϕ) related to latitude within the stretched geometry.
 *
 * References:
 *   Tsyganenko, N. A. (1995, 1996) - NB#3 (notebooks), P.26-27
 */
double xksi(double x, double y, double z) {
    // Empirical stretch coefficients (Axx, Bxx, Cxx, R0, DR)
    const double A11A12 = 0.305662, A21A22 = -0.383593, A41A42 = 0.2677733;
    const double A51A52 = -0.097656, A61A62 = -0.636034;
    const double B11B12 = -0.359862, B21B22 = 0.424706;
    const double C61C62 = -0.126366, C71C72 = 0.292578;
    const double R0 = 1.21563, DR = 7.50937;

    // Noon-midnight latitude mapping (in radians)
    const double TNOON = 0.3665191;   // ~69 degrees
    const double DTETA = 0.09599309;  // width ~5.5 degrees

    // Precompute DR² to avoid recomputation
    double dr2 = DR * DR;

    // Compute radial distance and its powers
    double x2 = x * x;
    double y2 = y * y;
    double z2 = z * z;
    double r2 = x2 + y2 + z2;
    double r = std::sqrt(r2);

    // Normalize position vector
    double xr = x / r;
    double yr = y / r;
    double zr = z / r;

    // Compute position-dependent stretch magnitude (PR)
    // PR increases with distance beyond R0 in a smoothed way
    double pr;
    if (r < R0) {
        pr = 0.0;
    } else {
        pr = std::sqrt((r - R0) * (r - R0) + dr2) - DR;
    }

    // Stretched coordinates (F, G, H) — warped versions of x, y, z
    double f = x + pr * (A11A12 + A21A22 * xr + A41A42 * xr * xr +
                         A51A52 * yr * yr + A61A62 * zr * zr);
    double g = y + pr * (B11B12 * yr + B21B22 * xr * yr);
    double h = z + pr * (C61C62 * zr + C71C72 * xr * zr);

    // Prepare composite metrics from F, G, H
    double f2 = f * f;
    double g2 = g * g;
    double h2 = h * h;

    double fgh = f2 + g2 + h2;                   // squared length of (f, g, h)
    double fgh32 = std::pow(std::sqrt(fgh), 3);  // |FGH|^3

    double fchsq = f2 + g2;                      // projection onto the "equatorial" plane

    // Special case: avoid division by zero on the Z-axis (f² + g² ~ 0)
    if (fchsq < 1e-5) {
        return -1.0;
    }

    // Compute stretched parameters
    double sqfchsq = std::sqrt(fchsq);
    double alpha = fchsq / fgh32;  // geometric scaling factor

    // Map F coordinate into an effective magnetic latitude angle
    double theta = TNOON + 0.5 * DTETA * (1.0 - f / sqfchsq);
    double phi = std::pow(std::sin(theta), 2);  // angular mapping term

    // Final scalar transformation
    return alpha - phi;  // this value is often used as an argument in tapering functions
}

double fexp(double s, double a) {
    const double E = 2.718281828459;

    if (a < 0.0) {
        return std::sqrt(-2.0 * a * E) * s * std::exp(a * s * s);
    } else {
        return s * std::exp(a * (s * s - 1.0));
    }
}

double fexp1(double s, double a) {
    if (a <= 0.0) {
        return std::exp(a * s * s);
    } else {
        return std::exp(a * (s * s - 1.0));
    }
}

double tksi(double xksi, double xks0, double dxksi) {
    static bool initialized = false;
    static double tdz3;

    if (!initialized) {
        tdz3 = 2.0 * std::pow(dxksi, 3);
        initialized = true;
    }

    double tksii = 0.0;

    if (xksi - xks0 < -dxksi) {
        tksii = 0.0;
    } else if (xksi - xks0 >= dxksi) {
        tksii = 1.0;
    } else if (xksi >= xks0 - dxksi && xksi < xks0) {
        double br3 = std::pow(xksi - xks0 + dxksi, 3);
        tksii = 1.5 * br3 / (tdz3 + br3);
    } else if (xksi >= xks0 && xksi < xks0 + dxksi) {
        double br3 = std::pow(xksi - xks0 - dxksi, 3);
        tksii = 1.0 + 1.5 * br3 / (tdz3 - br3);
    }

    return tksii;
}



void t96Dipole(double psi, double x, double y, double z, 
               double* Bx, double* By, double* Bz) {
    
    // Precompute trigonometric values
    double sps = std::sin(psi);
    double cps = std::cos(psi);

    // Intermediate variables
    double P = x * x;
    double T = y * y;
    double U = z * z;
    double V = 3.0 * z * x;

    // Compute dipole scaling factor (constant from Fortran)
    double R2 = P + T + U;
    double Q = 30574.0 / std::pow(R2, 2.5);  // Equivalent to sqrt(R2)^5

    // Compute components in GSM
    *Bx = Q * ((T + U - 2.0 * P) * sps - V * cps);
    *By = -3.0 * y * Q * (x * sps + z * cps);
    *Bz = Q * ((P + T - 2.0 * U) * cps - V * sps);
}
