//
//  symmetrize.c
//  Symmetry
//
//  Created by Marcus Johansson on 04/02/15.
//  Copyright (c) 2015 Marcus Johansson. 
//
//  Distributed under the MIT License ( See LICENSE file or copy at http://opensource.org/licenses/MIT )
//

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "symmetrize.h"
#include "orbital.h"
#include "linalg.h"

#define SQR(x) ((x)*(x))

//msym_error_t addProjectionOntoSubspace(int d, double orb[d], msym_subspace_t *ss, msym_orbital_t basis[d], double mem[d], double proj[d]);

msym_error_t symmetrizeMoleculeProject(msym_point_group_t *pg, int esl, msym_equivalence_set_t *es, msym_permutation_t **perm, msym_thresholds_t *thresholds, double *err);
msym_error_t symmetrizeMoleculeLinear(msym_point_group_t *pg, int esl, msym_equivalence_set_t *es, msym_permutation_t **perm, msym_thresholds_t *thresholds, double *err);

msym_error_t symmetrizeMolecule(msym_point_group_t *pg, int esl, msym_equivalence_set_t *es, msym_permutation_t **perm, msym_thresholds_t *thresholds, double *err){
    msym_error_t ret = MSYM_SUCCESS;
    if((pg->type == MSYM_POINT_GROUP_TYPE_Cnv || pg->type == MSYM_POINT_GROUP_TYPE_Dnh) && pg->n == 0){
        ret = symmetrizeMoleculeLinear(pg,esl,es,perm,thresholds,err);
    } else {
        ret = symmetrizeMoleculeProject(pg,esl,es,perm,thresholds,err);
    }
    
    return ret;
}
/* This is a projection into the fully symmetric space.
 * A little more computation than if we just recreate it from one atom,
 * but it is independant of the chosen atom and we can get the size
 * of the fully symmetric component.
 * The sizes of the individual equivalence sets are rather small anyways.
 */

msym_error_t symmetrizeMoleculeProject(msym_point_group_t *pg, int esl, msym_equivalence_set_t *es, msym_permutation_t **perm, msym_thresholds_t *thresholds, double *err){
    msym_error_t ret = MSYM_SUCCESS;
    double e = 0.0;
    double (*v)[3] = malloc(sizeof(double[pg->order][3]));
    for(int i = 0; i < esl;i++){
        if(es[i].length > pg->order){
            ret = MSYM_SYMMETRIZATION_ERROR;
            msymSetErrorDetails("Equivalence set (%d elements) larger than order of point group (%d)",es[i].length,pg->order);
            goto err;
        }
        memset(v, 0, sizeof(double[pg->order][3]));
        for(int j = 0; j < pg->order;j++){
            for(int k = 0; k < es[i].length;k++){
                int p = perm[i][j].p[k];
                double sv[3];
                applySymmetryOperation(&pg->sops[j], es[i].elements[k]->v, sv);
                vadd(sv, v[p], v[p]);
            }
        }
        double sl = 0.0, ol = 0.0;
        for(int j = 0; j < es[i].length;j++){
            ol += vdot(es[i].elements[j]->v,es[i].elements[j]->v);
            sl += vdot(v[j],v[j]);
            vscale(1.0/((double)pg->order), v[j], es[i].elements[j]->v);
        }
        sl /= SQR((double)pg->order);
        if(!(es[i].length == 1 && ol <= thresholds->zero)) e += (ol-sl)/ol; //e = fmax(e,(ol-sl)/ol);
    }
    
    *err = sqrt(fmax(e,0.0)); //should never be < 0, but it's a dumb way to die
err:
    free(v);
    return ret;
}

msym_error_t symmetrizeMoleculeLinear(msym_point_group_t *pg, int esl, msym_equivalence_set_t *es, msym_permutation_t **perm, msym_thresholds_t *thresholds, double *err){
    msym_error_t ret = MSYM_SUCCESS;
    double e = 0.0;
    double (*v)[3] = malloc(sizeof(double[pg->order][3]));
    double (*vinf)[3] = malloc(sizeof(double[pg->order][3]));
    msym_symmetry_operation_t *cinf = NULL;
    
    for(int i = 0; i < pg->order;i++){
        if(pg->sops[i].type == PROPER_ROTATION && pg->sops[i].order == 0) {
            cinf = &pg->sops[i];
            break;
        }
    }
    
    if(cinf == NULL){
        ret = MSYM_SYMMETRIZATION_ERROR;
        msymSetErrorDetails("Cannot find Cinf operation in linear point group");
        goto err;
    }
    
    for(int i = 0; i < esl;i++){
        if(es[i].length > pg->order){
            ret = MSYM_SYMMETRIZATION_ERROR;
            msymSetErrorDetails("Equivalence set (%d elements) larger than order of point group (%d)",es[i].length,pg->order);
            goto err;
        }
        
        memset(v, 0, sizeof(double[pg->order][3]));
        
        for(int k = 0; k < es[i].length;k++){
            vproj(es[i].elements[k]->v, cinf->v, vinf[k]);
        }
        
        for(int j = 0; j < pg->order;j++){
            for(int k = 0; k < es[i].length;k++){
                int p = perm[i][j].p[k];
                double sv[3];
                applySymmetryOperation(&pg->sops[j], vinf[k], sv);
                vadd(sv, v[p], v[p]);
            }
        }
        double sl = 0.0, ol = 0.0;
        for(int j = 0; j < es[i].length;j++){
            ol += vdot(es[i].elements[j]->v,es[i].elements[j]->v);
            sl += vdot(v[j],v[j]);
            vscale(1.0/((double)pg->order), v[j], es[i].elements[j]->v);
        }
        sl /= SQR((double)pg->order);
        if(!(es[i].length == 1 && ol <= thresholds->zero)) e = fmax(e,(ol-sl)/ol);

        
    }
    
    *err = sqrt(e);
err:
    free(v);
    free(vinf);
    return ret;
}


msym_error_t symmetrizeWavefunctions(msym_point_group_t *pg, int ssl, msym_subspace_t *ss, int *span, int basisl, msym_basis_function_t basis[basisl], double wf[basisl][basisl], double symwf[basisl][basisl]){
    msym_error_t ret = MSYM_SUCCESS;
    
    int *icomp = calloc(basisl,sizeof(*icomp));
    int *ispan = calloc(pg->ct->d,sizeof(*ispan));
    
    memset(symwf,0,sizeof(double[basisl][basisl]));
    
    int md = 1;
    //could deduce from pg type but can't be bothered
    for(int k = 0;k < pg->ct->d;k++) md = (md > pg->ct->s[k].d ? md : pg->ct->s[k].d);
    double (*mem)[basisl] = malloc(sizeof(double[2][basisl > md ? basisl : md]));
    int (*pf)[md] = calloc(basisl+1,sizeof(*pf));
    double *dmpf = mem[1];
    
    int psalcl = 0;
    
    for(int i = 0;i < ssl;i++) psalcl += ss[i].salcl;
    
    double (*psalc)[psalcl] = calloc(basisl,sizeof(*psalc));
    double (*bfd)[md] = calloc(basisl, sizeof(*bfd));
    int *psalck = calloc(pg->ct->d, sizeof(*psalck));
    
    /* Determine salc components, and build information vectors (e.g. indexing/offsets/irreps) */
    for(int o = 0;o < basisl;o++){
        double mcomp = -1.0;
        int psalci = 0;
        for(int k = 0;k < pg->ct->d;k++){
            double mabs = 0.0;
            psalck[k] = psalci;
            for(int s = 0;s < ss[k].salcl;s++){
                msym_salc_t *salc = &ss[k].salc[s];
                double (*space)[salc->fl] = (double (*)[salc->fl]) salc->pf;
                double psalcabs = 0.0;
                for(int d = 0;d < salc->d;d++){
                    memset(mem[0], 0, sizeof(double[basisl]));
                    for(int j = 0; j < salc->fl;j++){
                        mem[0][salc->f[j] - basis] = space[d][j];
                    }
                    vlproj(basisl, wf[o], mem[0], mem[1]);
                    double pabssqr = vlsumsqr(basisl, mem[1]);
                    mabs += pabssqr;
                    psalcabs += pabssqr;
                    bfd[o][d] += pabssqr;
                }
                psalc[o][psalci] = sqrt(psalcabs);
                psalci++;
            }
            if(mabs > mcomp){
                icomp[o] = k;
                mcomp = mabs;
            }
        }
        ispan[icomp[o]]++;
    }
    
    for(int k = 0;k < pg->ct->d;k++){
        if(ispan[k] != span[k]*pg->ct->s[k].d){
            msymSetErrorDetails("Projected orbitals do not span the expected irredicible representations. Expected %d%s, got %d",span[k],pg->ct->s[k].name,ispan[k]);
            ret = MSYM_SYMMETRIZATION_ERROR;
            goto err;
        }
    }
    
    /* Find parner functions */
    for(int o = 0;o < basisl;o++){
        int ko = icomp[o], dim = pg->ct->s[ko].d, found = 0;
        
        for(int i = 1;i < md;i++){
            pf[o][i] = -1;
            pf[basisl][i] = -1;
        };
        
        for(int i = 0;i < o && !found;i++){
            for(int j = 1;j < md && !found;j++) found = pf[i][j] == o;
        }
        
        if(found || dim <= 1) continue;
        
        for(int i = 0;i < md;i++){dmpf[i] = DBL_MAX;}
        
        for(int po = 0; po < basisl;po++){
            if(icomp[po] != ko || o == po) continue;
            vlsub(psalcl,psalc[o],psalc[po],mem[0]); //don't use mem[1], dmpf is an alias,
            double c = vlabs(psalcl, mem[0]), mc = 0.0;
            int mic = 0;
            for(int i = 1;i < dim;i++){
                double diff = fabs(dmpf[i] - c);
                if(c < dmpf[i] && (diff > mc)){
                    mic = i;
                    mc = diff;
                }
            }
            if(mic > 0){
                dmpf[mic] = c;
                pf[o][mic] = po;
                pf[basisl][mic] = po;
            }
        }
        
        for(int i = 1;i < dim;i++){
            int index = pf[basisl][i];
            if(index > 0) {
                pf[o][0]++;
                pf[index][0]--;
            }
        }
    }
    
    
    /* verify that we have partners for everything */
    for(int o = 0;o < basisl;o++){
        int dim = pg->ct->s[icomp[o]].d;
        if(abs(pf[o][0])+1 != dim){
            msymSetErrorDetails("Unexpected number of partner functions for wave function %d (expected %d got %d)", o,dim,abs(pf[o][0])+1);
            ret = MSYM_SYMMETRIZATION_ERROR;
            goto err;
        }
        
        for(int i = 0;pf[o][0] >= 0 && i < dim;i++){
            if(pf[o][i] == -1){
                msymSetErrorDetails("Could not determine partner function %d of wave function %d",i, o);
                ret = MSYM_SYMMETRIZATION_ERROR;
                goto err;
            }
        }
        
        printf("basis %d (%s) partner functions = %d",o,pg->ct->s[icomp[o]].name,o);
        for(int i = 1;i < dim;i++){
            printf(",%d",pf[o][i]);
        }
        printf("\n");
        
    }
    
    for(int o = 0;o < basisl;o++){
        int k = icomp[o];
        int dim = pg->ct->s[k].d;
        
        if(pf[o][0] < 0) continue;
            
        pf[o][0] = o;
        for(int i = 0;i < dim;i++) pf[basisl][i] = -1;

        /* Get the unique dimensions for each partner function in which they have the largest component.
         * This is only needed when the symmetry is really broken but the degenerate functions can be averaged,
         * but it also keeps the ordering intact.
         * This is could be improved with a best match algorithm */
        for(int i = 0;i < dim;i++){
            double cmax = 0.0;
            for(int d = 0;d < dim;d++){
                double c = bfd[pf[o][i]][d]; //component of i:th partner in dimension d
                if(c > cmax){
                    int found = 0;
                    for(int j = 0;j < i;j++){
                        if(pf[basisl][j] == d){
                            found = 1;
                            break;
                        }
                    }
                    if(!found){
                        pf[basisl][i] = d;
                        cmax = c;
                    }
                }
            }
        }
        
        /*for(int i = 0;i < dim;i++){
            printf("partner function %d has maximum component in dimension %d\n",i,pf[basisl][i]);
        }*/
        
        /* calculate average component in each salc subspace and rotate onto the partner functions with largest component */
        for(int s = 0;s < ss[k].salcl;s++){
            int psalci = psalck[k] + s;
            double avg = 0;
            
            
            for(int d = 0;d < dim;d++) avg += psalc[pf[o][d]][psalci]; //
            avg /= dim;
            
            //printf("average component in salc %d(%s) for wf %d = %lf\n",psalci,pg->ct->s[k].name,o,avg);
            
            msym_salc_t *salc = &ss[k].salc[s];
            double (*space)[salc->fl] = (double (*)[salc->fl]) salc->pf;
            
            for(int d = 0; d < dim;d++){
                int wfi = pf[o][d], di = pf[basisl][d];
                memset(mem[0], 0, sizeof(double[basisl]));
                for(int j = 0; j < salc->fl;j++){
                    mem[0][salc->f[j] - basis] = avg*space[di][j];
                }
                vladd(basisl, mem[0], symwf[wfi], symwf[wfi]);
            }
        }
    }
    
err:
    
    free(ispan);
    free(icomp);
    free(mem);
    free(pf);
    free(psalc);
    free(bfd);
    free(psalck);

    return ret;
}

msym_error_t symmetrizeTranslation(msym_point_group_t *pg, msym_equivalence_set_t *es, msym_permutation_t *perm, int pi, double translation[3]){
    msym_error_t ret = MSYM_SUCCESS;
    double (*v)[3] = calloc(es->length,sizeof(double[3]));
    
    for(int j = 0; j < pg->order;j++){
        int p = perm[j].p[pi];
        double stranslation[3];
        applySymmetryOperation(&pg->sops[j], translation, stranslation);
        vadd(stranslation, v[p], v[p]);
    }
    
    double scale = ((double)es->length)/pg->order;
    
    for(int i = 0;i < es->length;i++){
        vscale(scale, v[i], v[i]);
        vadd(es->elements[i]->v,v[i],es->elements[i]->v);
    }
    
err:
    free(v);
    return ret;
}

/*
msym_error_t addProjectionOntoSubspace(int d, double orb[d], msym_subspace_t *ss, msym_orbital_t basis[d], double mem[d], double proj[d]){
    msym_error_t ret = MSYM_SUCCESS;
    if(ss->subspacel){
        for(int i = 0;i < ss->subspacel;i++){
            if(MSYM_SUCCESS != (ret = addProjectionOntoSubspace(d, orb, &ss->subspace[i], basis, mem, proj))) goto err;
        }
    } else {
        for(int i = 0; i < ss->d;i++){
            double (*space)[ss->basisl] = (double (*)[ss->basisl]) ss->space;
            memset(mem, 0, sizeof(double[d]));
            for(int j = 0; j < ss->basisl;j++){
                mem[ss->basis.o[j] - basis] = space[i][j];
            }
            vlproj(d, orb, mem, mem);
            vladd(d, mem, proj, proj);
        }
    }

err:
    return ret;
}
 */


