#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include "mcmc.h"
#include "gsl_helper.h"
#include "debug.h"

/**
 * for allocation, we don't want to call alloc too often, rather grow in steps
 */
#define ALLOCATION_CHUNKS 1

/**
 * we have to provide at leas new_iter + 1 space.
 * @param new_iter iteration
 */
static unsigned long get_new_size(unsigned long new_iter) {
	if (ALLOCATION_CHUNKS > 1)
		return ((new_iter + 1) / ALLOCATION_CHUNKS + 1) * ALLOCATION_CHUNKS;
	else
		return new_iter + 1;
}

/**
 * make space available as set in m->size
 */
static void resize(mcmc * m, unsigned long new_size) {
	unsigned long orig_size = m->size;
	unsigned long i;
	/* TODO: we can not allocate more than the int-space
	 * if we have more iterations than that, we need to move our data to the
	 * disk.
	 */
	IFSEGV
		dump_i("resizing params_distr to", (int)new_size);
	if (new_size < orig_size) {
		IFSEGV
			debug("shrinking -> freeing vectors");

		for (i = orig_size; i > new_size; i--) {
			IFSEGV
				dump_ul("freeing vector", i - 1);
			gsl_vector_free(m->params_distr[i - 1]);
		}
	}
	IFSEGV
		debug("reallocating space");
	m->params_distr = (gsl_vector**) realloc(m->params_distr, (int) new_size
			* sizeof(gsl_vector*));
	IFSEGV
		dump_p("params_distr", (void*)m->params_distr);
	if (new_size != 0) {
		assert(m->params_distr != NULL);
		IFSEGV
			dump_p("params_distr[0]", (void*)m->params_distr[0]);
	}

	if (new_size > orig_size) {
		IFSEGV
			debug("growing -> allocating vectors");
		for (i = orig_size; i < new_size; i++) {
			IFSEGV
				dump_ul("allocating vector", i);
			m->params_distr[i] = gsl_vector_alloc(get_n_par(m));
			assert(m->params_distr[i] != NULL);
		}
	}
	m->size = new_size;
	IFSEGV
		debug("done resizing");
}

void mcmc_prepare_iteration(mcmc * m, const unsigned long iter) {
	unsigned long new_size = get_new_size(iter);
	if (m->size != new_size) {
		resize(m, new_size);
	}
}

void init_seed(mcmc * m) {
	gsl_rng_env_setup();
	m->random = gsl_rng_alloc(gsl_rng_default);
}

mcmc * mcmc_init(const unsigned int n_pars) {
	mcmc * m;
	IFSEGV
		debug("allocating mcmc struct");
	m = (mcmc*) malloc(sizeof(mcmc));
	assert(m != NULL);
	m->n_iter = 0;
	m->size = 0;
	m->n_par = n_pars;
	m->accept = 0;
	m->reject = 0;
	m->prob = -1e+10;
	m->prob_best = -1e+10;

	init_seed(m);

	m->params = gsl_vector_alloc(m->n_par);
	assert(m->params != NULL);
	m->params_best = gsl_vector_alloc(m->n_par);
	assert(m->params_best != NULL);
	m->params_distr = NULL;
	mcmc_prepare_iteration(m, 0);
	assert(m->params_distr != NULL);

	m->params_accepts = (long*) calloc(m->n_par, sizeof(long));
	assert(m->params_accepts != NULL);
	m->params_rejects = (long*) calloc(m->n_par, sizeof(long));
	assert(m->params_rejects != NULL);
	m->params_step = gsl_vector_calloc(m->n_par);
	assert(m->params_step != NULL);
	m->params_min = gsl_vector_calloc(m->n_par);
	assert(m->params_min != NULL);
	m->params_max = gsl_vector_calloc(m->n_par);
	assert(m->params_max != NULL);

	m->params_descr = (const char**) calloc(m->n_par, sizeof(char*));

	m->x_dat = NULL;
	m->y_dat = NULL;
	m->model = NULL;
	IFSEGV
		debug("allocating mcmc struct done");
	return m;
}

mcmc * mcmc_free(mcmc * m) {
	unsigned int i;

	gsl_rng_free(m->random);
	IFSEGV
		debug("freeing params");
	gsl_vector_free(m->params);
	IFSEGV
		debug("freeing params_best");
	gsl_vector_free(m->params_best);
	IFSEGV
		debug("freeing params_distr");
	resize(m, 0);

	IFSEGV
		debug("freeing params_descr");
	for (i = 0; i < get_n_par(m); i++) {
		free((char*) m->params_descr[i]);
	}
	free(m->params_descr);

	IFSEGV
		debug("freeing accepts/rejects");
	free(m->params_accepts);
	free(m->params_rejects);
	IFSEGV
		debug("freeing step/min/max");
	gsl_vector_free(m->params_step);
	gsl_vector_free(m->params_min);
	gsl_vector_free(m->params_max);
	if (m->x_dat != NULL)
		gsl_vector_free((gsl_vector*) m->x_dat);
	if (m->y_dat != NULL)
		gsl_vector_free((gsl_vector*) m->y_dat);
	if (m->model != NULL)
		gsl_vector_free(m->model);
	free(m);
	m = NULL;
	return NULL;
}

void mcmc_check(const mcmc * m) {
	(void) m;
	assert(m != NULL);
	assert(m->n_par > 0);
	assert(m->model != NULL);
	assert(m->model->size > 0);
	assert(m->x_dat != NULL);
	assert(m->x_dat->size == m->model->size);
	assert(m->y_dat != NULL);
	assert(m->y_dat->size == m->x_dat->size);
	assert(m->params != NULL);
	assert(m->params->size == m->n_par);
	assert(m->params_best != NULL);
	assert(m->params_best->size == m->n_par);
	assert(m->size >= m->n_iter);
	assert(m->params_step != NULL);
}
