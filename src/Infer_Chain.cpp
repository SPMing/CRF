#include "CRF.h"

SEXP Infer_Chain(SEXP _crf)
{
	CRF crf(_crf);
	crf.Init_Belief();
	crf.Infer_Chain();
	return(crf._belief);
}

void CRF::Infer_Chain()
{
	/* forward pass */

	double *alpha = (double *) R_alloc(nNodes * maxState, sizeof(double));
	for (int i = 0; i < nNodes * maxState; i++)
		alpha[i] = 0;
	double *kappa = (double *) R_alloc(nNodes, sizeof(double));
	for (int i = 0; i < nNodes; i++)
		kappa[i] = 0;

	for (int i = 0; i < nStates[0]; i++)
	{
		alpha[nNodes * i] = nodePot[nNodes * i];
		kappa[0] += alpha[nNodes * i];
	}
	if (kappa[0] != 0)
	{
		for (int i = 0; i < nStates[0]; i++)
			alpha[nNodes * i] /= kappa[0];
	}

	double *p_alpha, *p0_alpha, *p_nodePot;
	double sumPot;
	for (int i = 1; i < nNodes; i++)
	{
		p0_alpha = alpha + i;
		p_nodePot = nodePot + i;
		for (int j = 0; j < nStates[i]; j++)
		{
			p_alpha = alpha + i - 1;
			sumPot = 0;
			for (int k=0; k < nStates[i-1]; k++)
			{
				sumPot += p_alpha[0] * EdgePot(i-1, k, j);
				p_alpha += nNodes;
			}
			p0_alpha[0] = sumPot * p_nodePot[0];
			kappa[i] += p0_alpha[0];
			p0_alpha += nNodes;
			p_nodePot += nNodes;
		}
		if (kappa[i] != 0)
		{
			p_alpha = alpha + i;
			for (int j = 0; j < nStates[i]; j++)
			{
				p_alpha[0] /= kappa[i];
				p_alpha += nNodes;
			}
		}
	}

	/* backward pass */

	double *beta = (double *) R_alloc(nNodes * maxState, sizeof(double));
	for (int i = 0; i < nNodes * maxState; i++)
		beta[i] = 0;

	for (int i = 0; i < nStates[nNodes-1]; i++)
		beta[nNodes-1 + nNodes * i] = 1;

	double *p_beta, *p0_beta, *p0_nodePot;
	for (int i = nNodes-2; i >= 0; i--)
	{
		p0_beta = beta + i;
		sumPot = 0;
		for (int j = 0; j < nStates[i]; j++)
		{
			p_beta = beta + i + 1;
			p_nodePot = nodePot + i + 1;
			for (int k=0; k < nStates[i+1]; k++)
			{
				p0_beta[0] += p_beta[0] * p_nodePot[0] * EdgePot(i, j, k);
				p_beta += nNodes;
				p_nodePot += nNodes;
			}
			sumPot += p0_beta[0];
			p0_beta += nNodes;
		}
		if (sumPot != 0)
		{
			p_beta = beta + i;
			for (int j = 0; j < nStates[i]; j++)
			{
				p_beta[0] /= sumPot;
				p_beta += nNodes;
			}
		}
	}

	double *p_nodeBel;
	double sumBel;
	for (int i = 0; i < nNodes; i++)
	{
		p_alpha = alpha + i;
		p_beta = beta + i;
		p_nodeBel = nodeBel + i;
		sumBel = 0;
		for (int j = 0; j < nStates[i]; j++)
		{
			p_nodeBel[0] = p_alpha[0] * p_beta[0];
			sumBel += p_nodeBel[0];
			p_alpha += nNodes;
			p_beta += nNodes;
			p_nodeBel += nNodes;
		}
		if (sumBel != 0)
		{
			p_nodeBel = nodeBel + i;
			for (int j = 0; j < nStates[i]; j++)
			{
				p_nodeBel[0] /= sumBel;
				p_nodeBel += nNodes;
			}
		}
	}

	for (int i = 0; i < nNodes-1; i++)
	{
		p_alpha = alpha + i;
		p0_beta = beta + i + 1;
		p0_nodePot = nodePot + i + 1;
		sumBel = 0;
		for (int j = 0; j < nStates[i]; j++)
		{
			p_beta = p0_beta;
			p_nodePot = p0_nodePot;
			for (int k = 0; k < nStates[i+1]; k++)
			{
				sumBel += EdgeBel(i, j, k) = p_alpha[0] * p_beta[0] * p_nodePot[0] * EdgePot(i, j, k);
				p_beta += nNodes;
				p_nodePot += nNodes;
			}
			p_alpha += nNodes;
		}
		if (sumBel != 0)
		{
			for (int j = 0; j < nStates[i]; j++)
			{
				for (int k = 0; k < nStates[i+1]; k++)
					EdgeBel(i, j, k) /= sumBel;
			}
		}
	}

	double Z = 1;
	for (int i = 0; i < nNodes; i++)
		Z *= kappa[i];
	*logZ = log(Z);
}
