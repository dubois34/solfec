/*
 * nts.c
 * Copyright (C) 2010 Tomasz Koziara (t.koziara AT gmail.com)
 * -------------------------------------------------------------------
 * Newton constraints solver
 */

/* This file is part of Solfec.
 * Solfec is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Solfec is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Solfec. If not, see <http://www.gnu.org/licenses/>. */

#if MPI
#include <mpi.h>
#endif

#include <stdlib.h>
#include <time.h>
#include "dom.h"
#include "bgs.h"
#include "nts.h"
#include "alg.h"
#include "bla.h"
#include "err.h"
#include "mrf.h"

/* non-monotone globalization merit function value selection */
static double nonmonotone_merit_function (double *values, int length, double merit, int index)
{
  values [index % length] = merit;

  for (int n = 0; n < length; n ++)
    if (values [n] > merit) merit = values [n];

  return merit;
}

/* line search */
static double line_search (LINSYS *sys, double reference, double *merit)
{
  double auxiliary,
	 alpha,
	 gamma;

  int imax,
      iter;

  auxiliary = 2.0 * (*merit);
  alpha = 1.0;
  gamma = 0.1;
  imax = 32;

  for (iter = 0; iter < imax; iter ++)
  {
    (*merit) = LINSYS_Merit (sys, alpha);

    if ((*merit) <= (reference - gamma*alpha*auxiliary)) break;
    else alpha *= 0.9;
  }

  return alpha;
}

/* update constraint reactions */
static double reactions_update (NEWTON *nt, LINSYS *sys, LOCDYN *ldy, double *nonmonvalues, int iter, double *merit)
{
  double reference, alpha;

  if (iter && nt->variant != FIXED_POINT) /* skip line search during the first iteration */
  {
    (*merit) = LINSYS_Merit (sys, 0.0);

    reference = nonmonotone_merit_function (nonmonvalues, nt->nonmonlength, (*merit), iter);

    alpha = line_search (sys, reference, merit);
  }
  else alpha = 1.0;

  return LINSYS_Advance (sys, alpha); /* R = R + alpha * DR, U = U(R) */
}

/* create solver */
NEWTON* NEWTON_Create (LINVAR variant, double epsilon, int maxiter, double meritval)
{
  NEWTON *nt;

  ERRMEM (nt = MEM_CALLOC (sizeof (NEWTON)));

  nt->variant = variant;
  nt->epsilon = epsilon;
  nt->maxiter = maxiter;
  nt->meritval = meritval;
  nt->nonmonlength = 5;
  nt->linmaxiter = 10;
  nt->rerhist = NULL;
  nt->merhist = NULL;
  nt->verbose = 1;

  return nt;
}

/* create on constraints subset (subset == NULL => entire set); needs to be destroyed and created again for every
 * new LOCDYN state but allows for more efficient multiple solves in parallel due to single initialization */
NEWTON* NEWTON_Subset_Create (LINVAR variant, LOCDYN *ldy, SET *subset, double epsilon, int maxiter, double meritval)
{
  NEWTON *nt;

  nt = NEWTON_Create (variant, epsilon, maxiter, meritval);
  nt->sys = LINSYS_Create (variant, ldy, subset);

  return nt;
}

/* run solver */
void NEWTON_Solve (NEWTON *nt, LOCDYN *ldy)
{
  double merit, error, *nonmonvalues;
  char fmt [512];
  LINSYS *sys;
  DOM *dom;

  dom = ldy->dom;

  if (dom->verbose)
  {
    sprintf (fmt, "NEWTON: (LIN its/res: %%%dd/%%.2e) iteration: %%%dd  error:  %%.2e  merit: %%.2e\n",
      (int)log10 (nt->linmaxiter) + 1, (int)log10 (nt->maxiter) + 1);
  }

  ERRMEM (nonmonvalues = MEM_CALLOC (nt->nonmonlength * sizeof (double)));
  nt->rerhist = realloc (nt->rerhist, nt->maxiter * sizeof (double));
  nt->merhist = realloc (nt->merhist, nt->maxiter * sizeof (double));

  if (nt->sys) sys = nt->sys;
  else sys = LINSYS_Create (nt->variant, ldy, NULL);

  /* TODO: idea => enable MULTIPLY_TRANSPOSED when poor merit improvements occur */

  merit = error = 1.0;
  nt->iters = 0;

#if 0
  error = LINSYS_Test (sys, nt->meritval, nt->linmaxiter);
#if MPI
  if (dom->rank == 0)
#endif
  printf ("NEWTON: W inversion relative accuract: %.2e\n", error);
#endif

  do
  {
    LINSYS_Update (sys); /* assemble A, b */

    LINSYS_Solve (sys,  0.01 * merit, nt->linmaxiter); /* FIXME */

    error = reactions_update (nt, sys, ldy, nonmonvalues, nt->iters, &merit); /* R(i+1) */

    nt->rerhist [nt->iters] = error;
    nt->merhist [nt->iters] = merit;

#if MPI
    if (dom->rank == 0)
#endif
    if (dom->verbose && nt->verbose) printf (fmt, LINSYS_Iters (sys), LINSYS_Resnorm (sys), nt->iters, error, merit);

  } while (++ nt->iters < nt->maxiter && (error > nt->epsilon || merit > nt->meritval));

  merit = MERIT_Function (ldy, 0);
 #if MPI
  if (dom->rank == 0)
#endif
  if (dom->verbose && nt->verbose) printf ("NEWTON: final energy merit: %g\n", merit);

  if (nt->sys == NULL) LINSYS_Destroy (sys);
  free (nonmonvalues);
}

/* write labeled satate values */
void NEWTON_Write_State (NEWTON *nt, PBF *bf)
{
}

/* return variant string */
char* NEWTON_Variant (NEWTON *nt)
{
  switch (LINEARIZATION_VARIANT (nt->variant))
  {
    case NONSMOOTH_HSW: return "NONSMOOTH_HSW";
    case NONSMOOTH_HYBRID: return "NONSMOOTH_HYBRID";
    case FIXED_POINT: return "FIXED_POINT";
    case NONSMOOTH_VARIATIONAL: return "NONSMOOTH_VARIATIONAL";
    case SMOOTHED_VARIATIONAL: return "SMOOTHED_VARIATIONAL";
  }

  return NULL;
}

/* destroy solver */
void NEWTON_Destroy (NEWTON *nt)
{
  if (nt->sys) LINSYS_Destroy (nt->sys);
  free (nt->rerhist);
  free (nt->merhist);
  free (nt);
}
