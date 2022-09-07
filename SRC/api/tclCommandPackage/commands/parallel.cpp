
#ifdef _PARALLEL_PROCESSING
#  include <DistributedDisplacementControl.h>
#  include <ShadowSubdomain.h>
#  include <Metis.h>
#  include <ShedHeaviest.h>
#  include <DomainPartitioner.h>
#  include <GraphPartitioner.h>
#  include <TclPackageClassBroker.h>
#  include <Subdomain.h>
#  include <SubdomainIter.h>
#  include <MachineBroker.h>
#  include <MPIDiagonalSOE.h>
#  include <MPIDiagonalSolver.h>
// parallel analysis
#  include <StaticDomainDecompositionAnalysis.h>
#  include <TransientDomainDecompositionAnalysis.h>

#  define MPIPP_H
#  include <DistributedSuperLU.h>
#  include <DistributedProfileSPDLinSOE.h>

// MachineBroker *theMachineBroker = 0;
   int  OPS_PARALLEL_PROCESSING = 0;
   int  OPS_NUM_SUBDOMAINS = 0;
   bool OPS_PARTITIONED = false;
   bool OPS_USING_MAIN_DOMAIN = false;
   bool setMPIDSOEFlag = false;
   int  OPS_MAIN_DOMAIN_PARTITION_ID = 0;
   PartitionedDomain     theDomain;
   DomainPartitioner     *OPS_DOMAIN_PARTITIONER = 0;
   GraphPartitioner      *OPS_GRAPH_PARTITIONER = 0;
   LoadBalancer          *OPS_BALANCER = 0;
   TclPackageClassBroker *OPS_OBJECT_BROKER = 0;
   MachineBroker         *OPS_MACHINE = 0;
   Channel               **OPS_theChannels = 0;  

#  elif defined(_PARALLEL_INTERPRETERS)

  bool setMPIDSOEFlag = false;
  
  // parallel analysis
  #include <DistributedDisplacementControl.h>
  
  Domain theDomain;
#endif

int getPID(ClientData,  Tcl_Interp *, int, TCL_Char **argv);
int getNP( ClientData,  Tcl_Interp *, int, TCL_Char **argv);
int opsBarrier(ClientData, Tcl_Interp *, int, TCL_Char **argv);
int opsSend(ClientData, Tcl_Interp *, int, TCL_Char **argv);
int opsRecv(ClientData, Tcl_Interp *, int,TCL_Char **argv);
int opsPartition(ClientData, Tcl_Interp *, int, TCL_Char **argv);

void Init_Parallel() {
  Tcl_CreateCommand(interp, "getNP",     &getNP, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "getPID",    &getPID, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "barrier",   &opsBarrier, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "send",      &opsSend, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "recv", i    &opsRecv, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "partition", &opsPartition, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
}


int
getPID(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char **argv)
{
  int pid = 0;
#ifdef _PARALLEL_INTERPRETERS
  if (theMachineBroker != 0)
    pid = theMachineBroker->getPID();
#endif

#ifdef _PARALLEL_PROCESSING
  if (theMachineBroker != 0)
    pid = theMachineBroker->getPID();
#endif

  // now we copy the value to the tcl string that is returned
  char buffer[30];
  sprintf(buffer, "%d", pid);
  Tcl_SetResult(interp, buffer, TCL_VOLATILE);

  return TCL_OK;
}

int
getNP(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char **argv)
{
  int np = 1;
#ifdef _PARALLEL_INTERPRETERS
  if (theMachineBroker != 0)
    np = theMachineBroker->getNP();
#endif

#ifdef _PARALLEL_PROCESSING
  if (theMachineBroker != 0)
    np = theMachineBroker->getNP();
#endif

  // now we copy the value to the tcl string that is returned
  char buffer[30];
  sprintf(buffer, "%d", np);
  Tcl_SetResult(interp, buffer, TCL_VOLATILE);

  return TCL_OK;
}


#ifdef _PARALLEL_PROCESSING
int
partitionModel(int eleTag)
{
  if (OPS_PARTITIONED == true)
    return 0;

  int result = 0;

  if (OPS_theChannels != 0)
    delete[] OPS_theChannels;

  OPS_theChannels = new Channel *[OPS_NUM_SUBDOMAINS];

  // create some subdomains
  for (int i = 1; i <= OPS_NUM_SUBDOMAINS; i++) {
    if (i != OPS_MAIN_DOMAIN_PARTITION_ID) {
      ShadowSubdomain *theSubdomain =
          new ShadowSubdomain(i, *OPS_MACHINE, *OPS_OBJECT_BROKER);
      theDomain.addSubdomain(theSubdomain);
      OPS_theChannels[i - 1] = theSubdomain->getChannelPtr();
    }
  }

  // create a partitioner & partition the domain
  if (OPS_DOMAIN_PARTITIONER == 0) {
    //      OPS_BALANCER = new ShedHeaviest();
    OPS_GRAPH_PARTITIONER = new Metis;
    // OPS_DOMAIN_PARTITIONER = new DomainPartitioner(*OPS_GRAPH_PARTITIONER,
    // *OPS_BALANCER);
    OPS_DOMAIN_PARTITIONER = new DomainPartitioner(*OPS_GRAPH_PARTITIONER);
    theDomain.setPartitioner(OPS_DOMAIN_PARTITIONER);
  }

  // opserr << "commands.cpp - partition numPartitions: " << OPS_NUM_SUBDOMAINS
  // << endln;

  result = theDomain.partition(OPS_NUM_SUBDOMAINS, OPS_USING_MAIN_DOMAIN,
                               OPS_MAIN_DOMAIN_PARTITION_ID, eleTag);

  if (result < 0)
    return result;

  OPS_PARTITIONED = true;

  DomainDecompositionAnalysis *theSubAnalysis;
  SubdomainIter &theSubdomains = theDomain.getSubdomains();
  Subdomain *theSub = 0;

  // create the appropriate domain decomposition analysis
  while ((theSub = theSubdomains()) != 0) {
    if (the_static_analysis != 0) {
      theSubAnalysis = new StaticDomainDecompositionAnalysis(
          *theSub, *theHandler, *theNumberer, *the_analysis_model, *theAlgorithm,
          *theSOE, *theStaticIntegrator, theTest, false);

    } else {
      theSubAnalysis = new TransientDomainDecompositionAnalysis(
          *theSub, *theHandler, *theNumberer, *the_analysis_model, *theAlgorithm,
          *theSOE, *theTransientIntegrator, theTest, false);
    }
    theSub->setDomainDecompAnalysis(*theSubAnalysis);
  }

  return result;
}

#endif

int
opsBarrier(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char **argv)
{
#ifdef _PARALLEL_INTERPRETERS
  return MPI_Barrier(MPI_COMM_WORLD);
#endif

  return TCL_OK;
}

int
opsSend(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char **argv)
{
#ifdef _PARALLEL_INTERPRETERS
  if (argc < 2)
    return TCL_OK;

  int otherPID = -1;
  int myPID = theMachineBroker->getPID();
  int np = theMachineBroker->getNP();
  const char *dataToSend = argv[argc - 1];
  int msgLength = strlen(dataToSend) + 1;

  const char *gMsg = dataToSend;
  //  strcpy(gMsg, dataToSend);

  if (strcmp(argv[1], "-pid") == 0 && argc > 3) {

    if (Tcl_GetInt(interp, argv[2], &otherPID) != TCL_OK) {
      opserr << "send -pid pid? data? - pid: " << argv[2] << " invalid\n";
      return TCL_ERROR;
    }

    if (otherPID > -1 && otherPID != myPID && otherPID < np) {

      MPI_Send((void *)(&msgLength), 1, MPI_INT, otherPID, 0, MPI_COMM_WORLD);
      MPI_Send((void *)gMsg, msgLength, MPI_CHAR, otherPID, 1, MPI_COMM_WORLD);

    } else {
      opserr << "send -pid pid? data? - pid: " << otherPID << " invalid\n";
      return TCL_ERROR;
    }

  } else {
    if (myPID == 0) {
      MPI_Bcast((void *)(&msgLength), 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast((void *)gMsg, msgLength, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
      opserr << "send data - only process 0 can do a broadcast - you may need "
                "to kill the application";
      return TCL_ERROR;
    }
  }

#endif

  return TCL_OK;
}

int
opsRecv(ClientData clientData, Tcl_Interp *interp, int argc, TCL_Char **argv)
{
#ifdef _PARALLEL_INTERPRETERS
  if (argc < 2)
    return TCL_OK;

  int otherPID = 0;
  int myPID = theMachineBroker->getPID();
  int np = theMachineBroker->getNP();
  TCL_Char *varToSet = argv[argc - 1];

  int msgLength = 0;
  char *gMsg = 0;

  if (strcmp(argv[1], "-pid") == 0 && argc > 3) {

    bool fromAny = false;

    if ((strcmp(argv[2], "ANY") == 0) || (strcmp(argv[2], "ANY_SOURCE") == 0) ||
        (strcmp(argv[2], "MPI_ANY_SOURCE") == 0)) {
      fromAny = true;
    } else {
      if (Tcl_GetInt(interp, argv[2], &otherPID) != TCL_OK) {
        opserr << "recv -pid pid? data? - pid: " << argv[2] << " invalid\n";
        return TCL_ERROR;
      }
    }

    if (otherPID > -1 && otherPID < np) {
      MPI_Status status;

      if (fromAny == false)
        if (myPID != otherPID)
          MPI_Recv((void *)(&msgLength), 1, MPI_INT, otherPID, 0,
                   MPI_COMM_WORLD, &status);
        else {
          opserr << "recv -pid pid? data? - " << otherPID
                 << " cant receive from self!\n";
          return TCL_ERROR;
        }
      else {
        MPI_Recv((void *)(&msgLength), 1, MPI_INT, MPI_ANY_SOURCE, 0,
                 MPI_COMM_WORLD, &status);
        otherPID = status.MPI_SOURCE;
      }

      if (msgLength > 0) {
        gMsg = new char[msgLength];

        MPI_Recv((void *)gMsg, msgLength, MPI_CHAR, otherPID, 1, MPI_COMM_WORLD,
                 &status);

        Tcl_SetVar(interp, varToSet, gMsg, TCL_LEAVE_ERR_MSG);
      }

    } else {
      opserr << "recv -pid pid? data? - " << otherPID << " invalid\n";
      return TCL_ERROR;
    }
  } else {

    if (myPID != 0) {
      MPI_Bcast((void *)(&msgLength), 1, MPI_INT, 0, MPI_COMM_WORLD);

      if (msgLength > 0) {
        gMsg = new char[msgLength];

        MPI_Bcast((void *)gMsg, msgLength, MPI_CHAR, 0, MPI_COMM_WORLD);

        Tcl_SetVar(interp, varToSet, gMsg, TCL_LEAVE_ERR_MSG);
      }

    } else {
      opserr << "recv data - only process 0 can do a broadcast - you may need "
                "to kill the application";
      return TCL_ERROR;
    }
  }

#endif

  return TCL_OK;
}


int
opsPartition(ClientData clientData, Tcl_Interp *interp, int argc,
             TCL_Char **argv)
{
#ifdef _PARALLEL_PROCESSING
  int eleTag;
  if (argc == 2) {
    if (Tcl_GetInt(interp, argv[1], &eleTag) != TCL_OK) {
      ;
    }
  }
  partitionModel(eleTag);
#endif
  return TCL_OK;
}
