/*
 * cvx.h
 * Copyright (C) 2008, Tomasz Koziara (t.koziara AT gmail.com)
 * --------------------------------------------------------------
 * collection of convex polyhedrons
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

#include "mat.h"
#include "mot.h"
#include "tri.h"

#ifndef ELEMENT_TYPE
#define ELEMENT_TYPE
typedef struct element ELEMENT;
#endif

#ifndef __cvx__
#define __cvx__

typedef struct elepnt ELEPNT;
struct elepnt
{
  ELEMENT *ele;
  double pnt [3]; /* local element point */
};

typedef struct convex CONVEX;
struct convex
{
  double *ref, /* reference vertices */
	 *cur, /* current vecrtices */
         *pla; /* planes */

  ELEPNT *epn; /* element points corresponding to vertices */
  
  int *surface, /* surface identifiers */
      *fac; /* faces */
 
  CONVEX **adj; /* adjacency */

  ELEMENT **ele; /* intersected elements */

  short nver, /* vertices count */
        nfac, /* faces count */
        nadj, /* number of neighbours */
        nele, /* number of intersected elements */
	flag; /* auxiliary flag used internally */

  int volume; /* volume identifier */

  BULK_MATERIAL *mat;

  CONVEX *next; /* next in a list */
};

/* append convices list 'cvx' with a newly created convex (ver, nver, fac, nfac, surface);
 * vertices 'ver' are 3-vectors => x1, y1, z1, x2, y2, z2, ... (until x'nver' ...)
 * faces are assembled as follows => m1, v1, v2, ..., vm1, m2, v1, v2, ..., vm2, ... (until m'nfac' ...)
 * surfaces are simply => s1, s2, s3, ..., s'nfac'; 'cvx' can be NULL => this creates a single convex */
CONVEX* CONVEX_Create (CONVEX *cvx, double *ver, int nver, int *fac, int nfac, int *surfaces, int volume);

/* append convices list 'cvx' with a convex hull of the input point set */
CONVEX* CONVEX_Hull (CONVEX *cvx, double *pnt, int npnt, int surface, int volume);

/* glue two convex lists */
CONVEX* CONVEX_Glue (CONVEX *cvx, CONVEX *cvy);

/* compute adjacency data of convices;
 * no other function affects the adjacency */
void CONVEX_Compute_Adjacency (CONVEX *cvx);

/* create a copy of a list */
CONVEX* CONVEX_Copy (CONVEX *cvx);

/* scaling of a list  */
void CONVEX_Scale (CONVEX *cvx, double *vector);

/* translation of a list */
void CONVEX_Translate (CONVEX *cvx, double *vector);

/* rotation of a list */
void CONVEX_Rotate (CONVEX *cvx, double *point, double *vector, double angle);

/* cut through convices with a plane; return triangulated cross-section; vertices in the triangles
 * point to the memory allocated after the triangles memory; adjacency is not maintained;
 * TRI->ptr stores a pointer to the geometrical object that has been cut by the triangle */
TRI* CONVEX_Cut (CONVEX *cvx, double *point, double *normal, int *m);

/* split convices in two lists with plane defined by (point, normal); adjacencies between
 * the split lists elements need to be recomputed; surfid corresponds to the new surface;
 * topoadj != 0 implies cutting from the point and through the topological adjacency only */
void CONVEX_Split (CONVEX *cvx, double *point, double *normal, short topoadj, int surfid[2], CONVEX **one, CONVEX **two);

/* is convex set separable into disjoint parts */
int CONVEX_Separable (CONVEX *cvx);

/* separate convex set into disjoint parts */
CONVEX** CONVEX_Separate (CONVEX *cvx, int *m);

/* compute partial characteristic: 'vo'lume and static momenta
 * 'sx', 'sy, 'sz' and 'eul'er tensor; assume that all input data is initially zero; */
void CONVEX_Char_Partial (CONVEX *cvx, int ref, double *vo, double *sx, double *sy, double *sz, double *eul);

/* get characteristics of the convices in list:
 * volume, mass center, and Euler tensor (centered) */
void CONVEX_Char (CONVEX *cvx, int ref, double *volume, double *center, double *euler);

/* compute convex colume (referential
 * if ref == 1, or current otherwise) */
double CONVEX_Volume (CONVEX *cvx, int ref);

/* update extents of an individual convex */
void CONVEX_Extents (void *data, CONVEX *cvx, double *extents);

/* update oriented extents of an individual convex */
void CONVEX_Oriented_Extents (CONVEX *cvx, double *vx, double *vy, double *vz, double *extents);

/* compute extents of convex list */
void CONVEX_List_Extents (CONVEX *cvx, double *extents);

/* compute oriented extents of convex list */
void CONVEX_List_Oriented_Extents (CONVEX *cvx, double *vx, double *vy, double *vz, double *extents);

/* return first not NULL bulk material for a convex list */
void* CONVEX_First_Bulk_Material (CONVEX *cvx);

/* return convex containing the point */
CONVEX* CONVEX_Containing_Point (CONVEX *cvx, double *point);

/* does this convex (not a list) contain a spatial point? */
int CONVEX_Contains_Point (void *dummy, CONVEX *cvx, double *point);

/* return distance of a spatial point to the convex */
double CONVEX_Spatial_Point_Distance (void *dummy, CONVEX *cvx, double *point);

/* update convex list according to the given motion */
void CONVEX_Update (CONVEX *cvx, void *body, void *shp, MOTION motion);

/* test wether two convices are adjacent */
int CONVEX_Adjacent (CONVEX *one, CONVEX *two);

/* return 6-vector (normal, point) planes of convex faces */
double* CONVEX_Planes (CONVEX *cvx);

/* free list of convices */
void CONVEX_Destroy (CONVEX *cvx);

/* pack convex(es) into double and integer buffers (d and i buffers are of initial
 * dsize and isize, while the final numberof of doubles and ints is packed) */
void CONVEX_Pack (CONVEX *cvx, int *dsize, double **d, int *doubles, int *isize, int **i, int *ints);

/* unpack convex(es) from double and integer buffers (unpacking starts at dpos and ipos
 * in d and i and no more than a specific number of doubles and ints can be red) */
CONVEX* CONVEX_Unpack (void *solfec, int *dpos, double *d, int doubles, int *ipos, int *i, int ints);

/* export MBFCP definition */
void CONVEX_2_MBFCP (CONVEX *cvx, FILE *out);

#endif
