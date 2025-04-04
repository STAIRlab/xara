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
// Written: fmk
// Created: 04/01
//
// Description: This file contains the class definition for 
// FiberSectionWarping3d.h. FiberSectionWarping3d provides the abstraction of a 
// 3d beam section discretized by fibers. The section stiffness and
// stress resultants are obtained by summing fiber contributions.

#ifndef FiberSectionWarping3d_h
#define FiberSectionWarping3d_h

#include <FrameSection.h>
#include <Vector.h>
#include <Matrix.h>

class UniaxialMaterial;
class Response;

class FiberSectionWarping3d : public FrameSection
{
  public:
    FiberSectionWarping3d(); 
#if 0
    FiberSectionWarping3d(int tag, int numFibers, Fiber **fibers, UniaxialMaterial &torsion);
    FiberSectionWarping3d(int tag, int numFibers, UniaxialMaterial **mats,
			  SectionIntegration &si, UniaxialMaterial &torsion);    
#endif
    FiberSectionWarping3d(int tag, int numFibers, UniaxialMaterial &torsion);

    ~FiberSectionWarping3d();

    const char *getClassType(void) const {return "FiberSectionWarping3d";};

    int   setTrialSectionDeformation(const Vector &deforms); 
    const Vector &getSectionDeformation(void);

    const Vector &getStressResultant(void);
    const Matrix &getSectionTangent(void);
    const Matrix &getInitialTangent(void);

    int   commitState(void);
    int   revertToLastCommit(void);    
    int   revertToStart(void);
 
    FrameSection *getFrameCopy();
    const ID &getType();
    int getOrder (void) const;
    
    int sendSelf(int cTag, Channel &theChannel);
    int recvSelf(int cTag, Channel &theChannel, 
		 FEM_ObjectBroker &theBroker);
    void Print(OPS_Stream &s, int flag = 0);
	    
    Response *setResponse(const char **argv, int argc, 
			  OPS_Stream &s);
    int getResponse(int responseID, Information &info);

    // TODO: How is Height usually given?
    int addFiber(UniaxialMaterial &theFiber, const double Area, const double yLoc, const double zLoc, const double Height=1.0);

    // AddingSensitivity:BEGIN //////////////////////////////////////////
    int setParameter(const char **argv, int argc, Parameter &param);

    const Vector & getStressResultantSensitivity(int gradIndex, bool conditional);
    const Matrix & getSectionTangentSensitivity(int gradIndex);
    int   commitSensitivity(const Vector& sectionDeformationGradient, int gradIndex, int numGrads);

    const Vector & getSectionDeformationSensitivity(int gradIndex);
    // AddingSensitivity:END ///////////////////////////////////////////



  protected:
    
  private:
    int numFibers, sizeFibers;        // number of fibers in the section
    UniaxialMaterial **theMaterials;  // array of pointers to materials
    double   *matData;                // data for the materials [yloc and area]
    double   kData[36];               // data for ks matrix 
    double   sData[6];                // data for s vector 
   // double   Height;
    double yBar;       // Section centroid
    double zBar;
  
    static ID code;

    Vector e;          // trial section deformations 
    Vector eCommit;    // committed section deformations 
    Vector *s;         // section resisting forces  (axial force, bending moment)
    Matrix *ks;        // section stiffness

    UniaxialMaterial *theTorsion;
    
    // AddingSensitivity:BEGIN //////////////////////////////////////////
    int parameterID;
    Matrix *SHVs;
    // AddingSensitivity:END ///////////////////////////////////////////
};

#endif
