/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
//
#ifndef FullGenLinSolver_h
#define FullGenLinSolver_h
//
// Written: fmk 
// Created: 11/96
// Revision: A
//
// Description: This file contains the class definition for FullGenLinSolver.
// FullGenLinSolver is a concrete subclass of LinearSOE. It stores full
// unsymmetric linear system of equations using 1d arrays in Fortran style
//
#include <LinearSOESolver.h>
class FullGenLinSOE;

class FullGenLinSolver : public LinearSOESolver
{
  public:
    FullGenLinSolver(int classTag);    
    virtual ~FullGenLinSolver();

    virtual int setLinearSOE(FullGenLinSOE &theSOE);
    
  protected:
    FullGenLinSOE *theSOE;

  private:

};

#endif

