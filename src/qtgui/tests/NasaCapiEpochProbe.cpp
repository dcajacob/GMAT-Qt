// Test-only, read-only inspection of the engine selected by the public C ABI.
// This library is neither installed nor registered as a production plugin.
#include "ODEModel.hpp"
#include "PropSetup.hpp"
#include "PropagationStateManager.hpp"
extern ODEModel *ode;
extern PropSetup *pSetup;
extern "C" int GmatCapiObserveEpochs(double *out,int capacity) {
   if(!ode || !pSetup || !out || capacity<5 || ode->GetNumForces()!=1)return -1;
   auto *state=pSetup->GetPropStateManager()->GetState();
   auto *force=ode->GetForce(0);
   out[0]=state->GetEpoch();
   out[1]=state->GetEpochGT().GetMjd();
   out[2]=ode->GetRealParameter(ode->GetParameterID("Epoch"));
   out[3]=force->GetRealParameter(force->GetParameterID("Epoch"));
   out[4]=force->GetGmatTimeParameter(force->GetParameterID("Epoch")).GetMjd();
   return 5;
}
