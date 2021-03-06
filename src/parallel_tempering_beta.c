/*
    APEMoST - Automated Parameter Estimation and Model Selection Toolkit
    Copyright (C) 2009  Johannes Buchner

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mcmc.h"
#include "gsl_helper.h"
#include "debug.h"
#include "parallel_tempering_beta.h"
#include "define_defaults.h"

void set_beta(mcmc * m, double newbeta) {
	((parallel_tempering_mcmc *) m->additional_data)->beta = newbeta;
	((parallel_tempering_mcmc *) m->additional_data)->swapcount = 0;
}
double get_beta(const mcmc * m) {
	return ((parallel_tempering_mcmc *) m->additional_data)->beta;
}
void inc_swapcount(mcmc * m) {
	((parallel_tempering_mcmc *) m->additional_data)->swapcount++;
}
unsigned long get_swapcount(const mcmc * m) {
	return ((parallel_tempering_mcmc *) m->additional_data)->swapcount;
}

void print_current_positions(const mcmc ** chains, const int n_beta) {
	int i;
	printf("printing chain parameters: \n");
	for (i = 0; i < n_beta; i++) {
		printf("\tchain %d: swapped %lu times: ", i, get_swapcount(chains[i]));
		printf("\tchain %d: current %f: ", i, get_prob(chains[i]));
		dump_vectorln(get_params(chains[i]));
		printf("\tchain %d: best %f: ", i, get_prob_best(chains[i]));
		dump_vectorln(get_params_best(chains[i]));

	}
	fflush(stdout);
}

double equidistant_beta(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return beta_0 + i * (1 - beta_0) / (n_beta - 1);
}
double equidistant_temperature(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return 1 / (1 / beta_0 + i * (1 - 1 / beta_0) / (n_beta - 1));
}
double chebyshev_temperature(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return 1 / (1 / beta_0 + (1 - 1 / beta_0) / 2 * (1 - cos(i * M_PI / (n_beta
			- 1))));
}
double chebyshev_beta(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return beta_0 + (1 - beta_0) / 2 * (1 - cos(i * M_PI / (n_beta - 1)));
}

double equidistant_stepwidth(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return beta_0 + pow(i * 1.0 / (n_beta - 1), 2) * (1 - beta_0);
}
double chebyshev_stepwidth(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return beta_0 + (1 - beta_0) * pow((1 - cos(i * M_PI / (n_beta - 1))) / 2,
			2);
}
double hot_chains(const unsigned int i, const unsigned int n_beta,
		const double beta_0) {
	return beta_0 + 0* i * n_beta;
}

double get_chain_beta(unsigned int i, unsigned int n_beta, double beta_0) {
	if (n_beta == 1)
		return 1.0;
	/* this reverts the order so that beta(0) = 1.0. */
	return BETA_ALIGNMENT(n_beta - i - 1, n_beta, beta_0);
}

double calc_beta_0(mcmc * m, gsl_vector * stepwidth_factors) {
	double max;
	gsl_vector * range = dup_vector(get_params_max(m));
	gsl_vector_sub(range, get_params_min(m));
	gsl_vector_scale(range, BETA_0_STEPWIDTH);
	gsl_vector_div(range, get_steps(m));
	gsl_vector_div(range, stepwidth_factors);
	max = pow(gsl_vector_max(range), -0.5);
	gsl_vector_free(range);
	return max;
}

