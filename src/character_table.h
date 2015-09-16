//
//  character_table.h
//  libmsym
//
//  Created by Marcus Johansson on 28/11/14.
//  Copyright (c) 2014 Marcus Johansson. 
//
//  Distributed under the MIT License ( See LICENSE file or copy at http://opensource.org/licenses/MIT )
//

#ifndef __MSYM__CHARACTER_TABLE_h
#define __MSYM__CHARACTER_TABLE_h

#include "msym.h"
#include "symop.h"

#define C4PI (-1.61803398874989484820458683436563811772030917980576286213544) //(2*cos(4*M_PI/5))
#define C2PI (0.618033988749894848204586834365638117720309179805762862135448) //(2*cos(2*M_PI/5))



void decomposeRepresentation(msym_character_table_t *ct, double rspan[ct->d], double dspan[ct->d]);

void directProduct(int l, double irrep1[l], double irrep2[l], double pspan[l]);

msym_error_t generateCharacterTable(msym_point_group_type_t type, int n, int sopsl, msym_symmetry_operation_t sops[sopsl], msym_character_table_t **ct);



#endif /* defined(__MSYM__CHARACTER_TABLE_h) */
