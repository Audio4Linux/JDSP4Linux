#include <math.h>
#include "wdfcircuits_triode.h"
void compute(Triode *triode, Real Pbb, Real Gbb, Real Kbb)
{
	//	Real Kb_o = Kb;
	//	Real Gb_o = Gb;
	//	Real Pb_o = Pb;

	//	Kb = (2.0*vk-Kbb);
	//	Gb = (2.0*vg-Gbb);
	//	Pb = (2.0*vp-Pbb);

	triode->Kb = Kbb;
	triode->Gb = Gbb;
	triode->Pb = Pbb;

	//Step 3: compute wave reflections inside the triode
	Real vp0, vp1;
	triode->vg = zeroffg(triode, -10.0, 10.0, TOLERANCE);
	//v.vg = v.secantfg(&vg0,&vg1);

	vp0 = triode->e;
	vp1 = 0.0;
	triode->vp = zeroffp(triode, vp0, vp1, TOLERANCE);
	//v.vp = v.secantfp(&vp0,&vp1);

	triode->vk = triode->Kb - triode->Kr*(triode->g*pow(log(1.0 + exp(triode->c*(triode->vp / triode->mu + triode->vg))) / triode->c, triode->gamma));

	triode->Kb = (2.0*triode->vk - triode->Kb);
	triode->Gb = (2.0*triode->vg - triode->Gb);
	triode->Pb = (2.0*triode->vp - triode->Pb);
}

Real ffg(Triode *triode, Real VG)
{
	return (triode->Gb - triode->Gr*(triode->gg*pow(log(1.0 + exp(triode->cg*VG)) / triode->cg, triode->e) + triode->ig0) - VG);
}

Real fgdash(Triode *triode, Real VG)
{
	Real a1 = exp(triode->cg*VG);
	Real b1 = -triode->e*pow(log(a1 + 1.0) / triode->cg, triode->e - 1.0);
	Real c1 = a1 / (a1 + 1.0)*triode->gg*triode->Gr;
	return b1*c1;
}

Real ffp(Triode *triode, Real VP)
{
	static int prepared = 0;
	if (!prepared)
	{
		//go go series expansion
		const double L2 = log(2.0);
		const double scale = pow(L2, triode->gamma - 2.0) / (8.0*pow(triode->c, triode->gamma));
		triode->ffp_raw[0] = 8.0*L2*L2*scale;
		triode->ffp_raw[1] = triode->gamma * triode->c*L2 * 4.0 * scale;
		triode->ffp_raw[2] = (triode->c*triode->c*triode->gamma*scale)*(triode->gamma + L2 - 1.0);
		prepared = 1;
	}
	return triode->ffp_coeff[0] + triode->ffp_coeff[1] * VP + triode->ffp_coeff[2] * VP*VP;
}

Real fpdash(Triode *triode, Real VP)
{
	Real a1 = exp(triode->c*(triode->vg + VP / triode->mu));
	Real b1 = a1 / (triode->mu*(a1 + 1.0));
	Real c1 = triode->g*triode->gamma*triode->Pr*pow(log(a1 + 1.0) / triode->c, triode->gamma - 1.0);
	return c1*b1;
}

void TriodeInit(Triode *triode)
{
	triode->vg = 0.0;
	triode->vk = 0.0;
	triode->vp = 0.0;

	Real r = 1.0;

	while (1.0 < (Real)(1.0 + r))
		r = r / 2.0;

	r *= 2.0;
	triode->r8_epsilon = r;
}

Real zeroffp(Triode *triode, Real a, Real b, Real t)
{
	Real c;
	Real d;
	Real e;
	Real fa;
	Real fb;
	Real fc;
	Real m;
	Real macheps;
	Real p;
	Real q;
	Real r;
	Real s;
	Real sa;
	Real sb;
	Real tol;
	const double c0 = triode->ffp_raw[0], c1 = triode->ffp_raw[1], c2 = triode->ffp_raw[2];
	triode->ffp_coeff[0] = triode->Pb + triode->Pr * (triode->Gb / triode->Gr + c2 * triode->vg*triode->vg - triode->vg / triode->Gr + c1 * triode->vg + c0);
	triode->ffp_coeff[1] = (2.0*c2*triode->vg + c1)*(triode->Pr / triode->mu) - 1.0;
	triode->ffp_coeff[2] = c2 * triode->Pr / (triode->mu*triode->mu);
	//
	//	Make local copies of A and B.
	//
	sa = a;
	sb = b;
	fa = ffp(triode, sa);
	fb = ffp(triode, sb);

	c = sa;
	fc = fa;
	e = sb - sa;
	d = e;

	macheps = triode->r8_epsilon;

	for (;;)
	{
		if (fabs(fc) < fabs(fb))
		{
			sa = sb;
			sb = c;
			c = sa;
			fa = fb;
			fb = fc;
			fc = fa;
		}

		tol = 2.0 * macheps * fabs(sb) + t;
		m = 0.5 * (c - sb);

		if (fabs(m) <= tol || fb == 0.0)
			break;

		if (fabs(e) < tol || fabs(fa) <= fabs(fb))
		{
			e = m;
			d = e;
		}
		else
		{
			s = fb / fa;

			if (sa == c)
			{
				p = 2.0 * m * s;
				q = 1.0 - s;
			}
			else
			{
				q = fa / fc;
				r = fb / fc;
				p = s * (2.0 * m * q * (q - r) - (sb - sa) * (r - 1.0));
				q = (q - 1.0) * (r - 1.0) * (s - 1.0);
			}

			if (0.0 < p)
				q = -q;
			else
				p = -p;

			s = e;
			e = d;

			if (2.0 * p < 3.0 * m * q - fabs(tol * q) && p < fabs(0.5 * s * q))
				d = p / q;
			else
			{
				e = m;
				d = e;
			}
		}
		sa = sb;
		fa = fb;

		if (tol < fabs(d))
			sb = sb + d;
		else if (0.0 < m)
			sb = sb + tol;
		else
			sb = sb - tol;

		fb = ffp(triode, sb);

		if ((0.0 < fb && 0.0 < fc) || (fb <= 0.0 && fc <= 0.0))
		{
			c = sa;
			fc = fa;
			e = sb - sa;
			d = e;
		}
	}
	return sb;
}

Real zeroffg(Triode *triode, Real a, Real b, Real t)
{
	Real c;
	Real d;
	Real e;
	Real fa;
	Real fb;
	Real fc;
	Real m;
	Real macheps;
	Real p;
	Real q;
	Real r;
	Real s;
	Real sa;
	Real sb;
	Real tol;
	//
	//	Make local copies of A and B.
	//
	sa = a;
	sb = b;
	fa = ffg(triode, sa);
	fb = ffg(triode, sb);

	c = sa;
	fc = fa;
	e = sb - sa;
	d = e;

	macheps = triode->r8_epsilon;

	for (;;)
	{
		if (fabs(fc) < fabs(fb))
		{
			sa = sb;
			sb = c;
			c = sa;
			fa = fb;
			fb = fc;
			fc = fa;
		}

		tol = 2.0 * macheps * fabs(sb) + t;
		m = 0.5 * (c - sb);

		if (fabs(m) <= tol || fb == 0.0)
			break;

		if (fabs(e) < tol || fabs(fa) <= fabs(fb))
		{
			e = m;
			d = e;
		}
		else
		{
			s = fb / fa;

			if (sa == c)
			{
				p = 2.0 * m * s;
				q = 1.0 - s;
			}
			else
			{
				q = fa / fc;
				r = fb / fc;
				p = s * (2.0 * m * q * (q - r) - (sb - sa) * (r - 1.0));
				q = (q - 1.0) * (r - 1.0) * (s - 1.0);
			}

			if (0.0 < p)
				q = -q;
			else
				p = -p;

			s = e;
			e = d;

			if (2.0 * p < 3.0 * m * q - fabs(tol * q) && p < fabs(0.5 * s * q))
				d = p / q;
			else
			{
				e = m;
				d = e;
			}
		}
		sa = sb;
		fa = fb;

		if (tol < fabs(d))
			sb = sb + d;
		else if (0.0 < m)
			sb = sb + tol;
		else
			sb = sb - tol;

		fb = ffg(triode, sb);

		if ((0.0 < fb && 0.0 < fc) || (fb <= 0.0 && fc <= 0.0))
		{
			c = sa;
			fc = fa;
			e = sb - sa;
			d = e;
		}
	}
	return sb;
}

void updateRValues(TubeStageCircuit *ckt, Real C_Ci, Real C_Ck, Real C_Co, Real E_E250, Real E_Vi, Real R_E250, Real R_Rg, Real R_Ri, Real R_Rk, Real R_Ro, Real R_Vi, Real sampleRate, Triode tube)
{
    ckt->Cia = 0.0;
    ckt->Cka = 0.0;
    ckt->Coa = 0.0;
    ckt->on = 0;
    TriodeInit(&ckt->t);
    ckt->t = tube;
    Real ViR = R_Vi;
    ckt->ViE = E_Vi;
    Real CiR = 1.0 / (2.0*C_Ci*sampleRate);
    Real CkR = 1.0 / (2.0*C_Ck*sampleRate);
    Real CoR = 1.0 / (2.0*C_Co*sampleRate);
    Real RoR = R_Ro;
    Real RgR = R_Rg;
    Real RiR = R_Ri;
    Real RkR = R_Rk;
    Real E250R = R_E250;
    ckt->E250E = E_E250;
    Real S0_3R = (CiR + ViR);
    ckt->S0_3Gamma1 = CiR / (CiR + ViR);
    Real P0_1R = S0_3R;
    Real P0_2R = RiR;
    Real P0_3R = 1.0 / (1.0 / P0_1R + 1.0 / P0_2R);
    ckt->P0_3Gamma1 = 1.0 / P0_1R / (1.0 / P0_1R + 1.0 / P0_2R);
    ckt->S1_3Gamma1 = RgR / (RgR + P0_3R);
    Real I3_1R = CkR;
    Real I3_2R = RkR;
    ckt->I3_3Gamma1 = 1.0 / I3_1R / (1.0 / I3_1R + 1.0 / I3_2R);
    Real S2_3R = (CoR + RoR);
    ckt->S2_3Gamma1 = CoR / (CoR + RoR);
    Real P2_1R = S2_3R;
    Real P2_2R = E250R;
    ckt->P2_3Gamma1 = 1.0 / P2_1R / (1.0 / P2_1R + 1.0 / P2_2R);
    ckt->t.Kr = ckt->I3_3Gamma1;
    ckt->t.Pr = ckt->S2_3Gamma1;
    ckt->t.Gr = ckt->S1_3Gamma1;
}

Real advanc(TubeStageCircuit *ckt, Real VE)
{
    ckt->ViE = VE;
    Real Ckb = ckt->Cka;
    Real I3_3b3 = -ckt->I3_3Gamma1*(-Ckb);
    Real Cib = ckt->Cia;
    Real S0_3b3 = -(Cib)-(ckt->ViE);
    Real P0_3b3 = -ckt->P0_3Gamma1*(-S0_3b3);
    Real S1_3b3 = -(P0_3b3);
    Real Cob = ckt->Coa;
    Real S2_3b3 = -Cob;
    Real P2_3b3 = ckt->E250E - ckt->P2_3Gamma1*(ckt->E250E - S2_3b3);
    //Tube:    K       G      P
    compute(&ckt->t, I3_3b3, S1_3b3, P2_3b3);
    Real b1 = ckt->t.Kb;
	Real b2 = ckt->t.Gb;
    Real b3 = ckt->t.Pb;
    //Set As
    Real I3_3b1 = b1 - Ckb - ckt->I3_3Gamma1*(-Ckb);
    ckt->Cka = I3_3b1;
    Real S1_3b2 = -(-(b2)-ckt->S1_3Gamma1*(-(P0_3b3)+-(b2)));
    Real P0_3b1 = S1_3b2 - S0_3b3 - ckt->P0_3Gamma1*(-S0_3b3);
    Real S0_3b1 = -(-(Cib)-ckt->S0_3Gamma1*(-(Cib)+-(ckt->ViE)+-(P0_3b1)));
    ckt->Cia = S0_3b1;
    Real P2_3b1 = b3 + ckt->E250E - S2_3b3 - ckt->P2_3Gamma1*(ckt->E250E - S2_3b3);
    Real S2_3b1 = -(-(Cob)-ckt->S2_3Gamma1*(-(Cob) + -(P2_3b1)));
    ckt->Coa = S2_3b1;
    Real S2_3b2 = -(-(Cob)+-(P2_3b1)-ckt->S2_3Gamma1*(-(Cob) + -(P2_3b1)));
    Real Roa = S2_3b2;
    return -(Roa);
}
void warmup_tubes(TubeStageCircuit *ckt, int warmupDuration)
{
    ckt->on = 0;
    for (int i = 0; i < warmupDuration; i++)
        advanc(ckt, 0.0);
    ckt->on = 1;
}