/* Real C11 consumer of the public ABI, with independently specified epochs.
 * No private probes, GMAT C++ headers or time-conversion helpers are used. */
#ifdef __cplusplus
#error Compile this acceptance client as C
#endif
#include "CInterfaceFunctions.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

static int assertions, readings;
#define CHECK(x) do { ++assertions; if (!(x)) { \
  fprintf(stderr,"C_API_EPOCH_ASSERTION line %d: %s; message: %s\n", \
          __LINE__,#x,getLastMessage()); return 1; } } while (0)

static int epoch(const char *stage, const char *name, double expected) {
  double actual=-987654321.25;
  CHECK(GetSpacecraftEpoch(name,&actual)==0);
  CHECK(isfinite(actual) && fabs(actual-expected)<=1e-10);
  CHECK(strstr(getLastMessage(),"A1 modified Julian days")!=NULL);
  printf("NASA_EPOCH {\"stage\":\"%s\",\"spacecraft\":\"%s\",\"actual\":%.17g,\"expected\":%.17g}\n",
         stage,name,actual,expected);
  ++readings;
  return 0;
}
static int unavailable(void) {
  double sentinel=-987654321.25;
  CHECK(GetSpacecraftEpoch("A",&sentinel)==-2);
  CHECK(sentinel==-987654321.25);
  CHECK(strstr(getLastMessage(),"initialized run sandbox")!=NULL);
  return 0;
}
static int arguments(void) {
  double sentinel=-987654321.25;
  CHECK(GetSpacecraftEpoch(NULL,&sentinel)==-1);
  CHECK(GetSpacecraftEpoch("",&sentinel)==-1);
  CHECK(GetSpacecraftEpoch("A",NULL)==-1);
  CHECK(sentinel==-987654321.25);
  return 0;
}
static int run(const char *path,int status) {
  CHECK(LoadScript(path)==0);
  CHECK(unavailable()==0);
  CHECK(RunScript()==status);
  return 0;
}
int main(int argc,char **argv) {
  CHECK(argc==3 && strcmp(argv[2],"spacecraftepoch")==0);
  CHECK(arguments()==0 && unavailable()==0);
  CHECK(StartGmat()==0);
  FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);
  char line[4096];while(fgets(line,sizeof(line),maps)) {
    char *path=strchr(line,'/');if(path)printf("NASA_LOADED %s",path);
  }
  fclose(maps);
  CHECK(dlsym(RTLD_DEFAULT,"GetSpacecraftEpoch")!=NULL);
  CHECK(unavailable()==0);
  CHECK(run(argv[1],0)==0);
  CHECK(epoch("post-run","A",21545.125+60./86400.)==0);
  CHECK(epoch("post-run","B",21600.875+30./86400.)==0);
  CHECK(epoch("unpropagated","C",22000.0625)==0);
  CHECK(arguments()==0);
  const char *invalid[]={"Missing","a","FM1","EarthMJ2000Eq"};
  for(int i=0;i<4;++i) {
    double sentinel=12345.5;
    CHECK(GetSpacecraftEpoch(invalid[i],&sentinel)==-3);
    CHECK(sentinel==12345.5 && strlen(getLastMessage())>0);
  }
  int count=CountObjects();
  for(int model=1;model<=2;++model) {
    int id=FindOdeModel(model==1?"FM1":"FM2");CHECK(id>=1000);
    CHECK(SetModel(id)==id && GetStateSize()==42);
    double state[42];CHECK(GetState()!=NULL);memcpy(state,GetState(),sizeof(state));
    CHECK(epoch("selected-model","B",21600.875+30./86400.)==0);
    CHECK(epoch("selected-model","A",21545.125+60./86400.)==0);
    CHECK(memcmp(state,GetState(),sizeof(state))==0 && CountObjects()==count);
  }
  /* Repeated runs start from configuration, rather than accumulating time. */
  CHECK(RunScript()==0);
  CHECK(epoch("repeat","A",21545.125+60./86400.)==0);
  CHECK(epoch("repeat","B",21600.875+30./86400.)==0);
  CHECK(LoadScript("missing-epoch.script")!=0 && unavailable()==0);
  CHECK(run("prepare.script",0)==0);
  CHECK(epoch("prepare","A",21545.125)==0);
  CHECK(epoch("prepare","B",21600.875)==0);
  CHECK(run("stopped.script",-4)==0);
  CHECK(epoch("stopped","A",21545.125+10./86400.)==0);
  CHECK(epoch("stopped","B",21600.875)==0);
  CHECK(run("runtime-error.script",-5)==0);
  CHECK(epoch("runtime-error","A",21545.125+10./86400.)==0);
  CHECK(epoch("runtime-error","B",21600.875)==0);
  CHECK(run("initialization-error.script",-2)==0 && unavailable()==0);
  CHECK(run("replacement.script",0)==0);
  CHECK(epoch("reload-backwards","A",23000.25-20./86400.)==0);
  CHECK(epoch("reload","B",24000.5)==0);
  double sentinel=12345.5;
  CHECK(GetSpacecraftEpoch("C",&sentinel)==-3 && sentinel==12345.5);
  CHECK(run("assigned.script",0)==0);
  CHECK(epoch("mission-assignment","A",25000.375)==0);
  CHECK(epoch("mission-assignment","B",26000.625)==0);
  CHECK(StartGmat()==0 && unavailable()==0);
  CHECK(run(argv[1],0)==0);
  CHECK(epoch("restart","A",21545.125+60./86400.)==0);
  CHECK(epoch("restart","B",21600.875+30./86400.)==0);
  printf("NASA_CHECK {\"checkpoint\":\"c-api-spacecraft-epoch\",\"outcome\":\"passed\",\"assertions\":%d,\"readings\":%d,\"units\":\"A1ModJulian\",\"public_ABI\":true,\"private_probe\":false,\"read_only\":true,\"run_calls\":9}\n",assertions,readings);
  return 0;
}
