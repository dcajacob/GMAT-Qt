/* Real C11 client: mixed spacecraft configurations through the public ABI. */
#ifdef __cplusplus
#error Compile this acceptance client as C
#endif
#include "CInterfaceFunctions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int assertions;
#define CHECK(x) do { ++assertions; if (!(x)) { \
 fprintf(stderr,"C_API_CONFIG_ASSERTION line %d: %s; message: %s\n", \
 __LINE__,#x,getLastMessage());return 1;} } while(0)

int main(int argc,char **argv) {
 CHECK(argc==3 && strcmp(argv[2],"spacecraftconfiguration")==0);
 CHECK(StartGmat()==0);
 FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);
 char line[4096];while(fgets(line,sizeof(line),maps)) {
  char *path=strchr(line,'/');if(path)printf("NASA_LOADED %s",path);
 }
 fclose(maps);
 CHECK(LoadScript(argv[1])==0);
 int found[31]={0},count=CountObjects();CHECK(count>=37);
 for(int i=0;i<count;++i) {
  const char *name=GetObjectName(i);CHECK(name!=NULL);
  for(int j=0;j<31;++j) {
   char expected[20];snprintf(expected,sizeof(expected),"CFG%02d",j);
   if(strcmp(name,expected)==0){CHECK(!found[j]);found[j]=1;}
  }
 }
 for(int i=0;i<31;++i)CHECK(found[i]);
 CHECK(RunScript()==0);
 FILE *expected=fopen("expected-epochs.txt","r");CHECK(expected!=NULL);
 for(int i=0;i<31;++i) {
  char name[20];double epoch,tolerance,actual=-1;
  snprintf(name,sizeof(name),"CFG%02d",i);
  CHECK(fscanf(expected,"%lf %lf",&epoch,&tolerance)==2);
  CHECK(GetSpacecraftEpoch(name,&actual)==0);
  CHECK(isfinite(actual)&&fabs(actual-epoch)*86400<=tolerance);
  printf("NASA_CONFIG_EPOCH %s %.17g %.17g\n",name,actual,epoch);
 }
 CHECK(fclose(expected)==0);
 printf("NASA_CHECK {\"checkpoint\":\"c-api-spacecraft-configuration\",\"outcome\":\"passed\",\"assertions\":%d,\"spacecraft\":31,\"orbit_profiles\":6,\"epoch_profiles\":10,\"attitude_profiles\":15,\"public_ABI\":true,\"GUI_executions\":0}\n",assertions);
 return 0;
}
