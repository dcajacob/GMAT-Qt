/* External C11 witness for the implemented script/Propagate interface paths. */
#ifdef __cplusplus
#error This client must be compiled as C
#endif
#include "CInterfaceFunctions.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int assertions;
#define CHECK(x) do { ++assertions; if (!(x)) { \
  fprintf(stderr,"C_API_PROPAGATE_ASSERTION line %d: %s; message: %s\n", \
          __LINE__,#x,getLastMessage()); return 1; } } while (0)

struct Model { char name[64]; int dimension, cartesian, id; double epoch, state[84], assigned[84]; };
struct Craft { char name[64]; double epoch; };
static int close_value(double a,double b,double tolerance) {
  return isfinite(a) && isfinite(b) && fabs(a-b)<=tolerance;
}
static void vector(const double *values,int count) {
  putchar('[');
  for(int i=0;i<count;++i)printf("%s%.17g",i?",":"",values[i]);
  putchar(']');
}
static int epochs(struct Craft *craft,int count) {
  for(int i=0;i<count;++i) {
    double actual=-1;
    CHECK(GetSpacecraftEpoch(craft[i].name,&actual)==0);
    CHECK(close_value(actual,craft[i].epoch,2e-6/86400.));
  }
  return 0;
}
int main(int argc,char **argv) {
  CHECK(argc==4 && (strcmp(argv[3],"separate")==0 || strcmp(argv[3],"combined")==0));
  FILE *input=fopen(argv[2],"r");CHECK(input!=NULL);
  int model_count,craft_count,run_status;
  CHECK(fscanf(input,"%d %d %d",&model_count,&craft_count,&run_status)==3);
  CHECK(model_count>0 && model_count<=8 && craft_count>0 && craft_count<=16);
  struct Craft craft[16];struct Model models[8];
  memset(models,0,sizeof(models));
  for(int i=0;i<craft_count;++i)CHECK(fscanf(input,"%63s %lf",craft[i].name,&craft[i].epoch)==2);
  for(int i=0;i<model_count;++i) {
    struct Model *m=&models[i];
    CHECK(fscanf(input,"%63s %d %d %lf",m->name,&m->dimension,&m->cartesian,&m->epoch)==4);
    CHECK(m->dimension<=84 && m->cartesian>0 && m->cartesian<=m->dimension && m->cartesian%6==0);
    for(int j=0;j<m->cartesian;++j)CHECK(fscanf(input,"%lf",&m->state[j])==1);
  }
  CHECK(fclose(input)==0);
  CHECK(StartGmat()==0);
  FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);
  char line[4096];while(fgets(line,sizeof(line),maps)) {
    char *path=strchr(line,'/');if(path)printf("NASA_LOADED %s",path);
  }
  fclose(maps);
  CHECK(GetStateSize()==0 && GetState()==NULL);
  CHECK(FindOdeModel(models[0].name)==-1);
  if(strcmp(argv[3],"combined")==0)CHECK(LoadAndRunScript(argv[1])==run_status);
  else {
    CHECK(LoadScript(argv[1])==0);
    CHECK(GetStateSize()==0 && GetState()==NULL && SetModel(1000)==-1);
    CHECK(RunScript()==run_status);
  }
  CHECK(GetStateSize()==0 && GetState()==NULL && SetModel(1000)==-1);
  CHECK(epochs(craft,craft_count)==0);
  for(int i=0;i<model_count;++i) {
    struct Model *m=&models[i];
    m->id=FindOdeModel(m->name);CHECK(m->id>=1000);
    for(int j=0;j<i;++j)CHECK(models[j].id!=m->id);
    CHECK(SetModel(m->id)==m->id && GetStateSize()==m->dimension);
    double *state=GetState();CHECK(state!=NULL);
    for(int j=0;j<m->cartesian;++j)CHECK(close_value(state[j],m->state[j],j%6<3?2e-5:3e-8));
    printf("NASA_MODEL {\"name\":\"%s\",\"dimension\":%d,\"state\":",m->name,m->dimension);
    vector(state,m->dimension);printf("}\n");
    memcpy(m->assigned,state,(size_t)m->dimension*sizeof(double));
    /* Distinct assignments expose later lookup/preparation overwriting state. */
    for(int j=0;j<m->cartesian;++j)m->assigned[j]+=.01*(i+1)*(j+1);
    if(m->dimension==42) {
      for(int j=0;j<36;++j)m->assigned[6+j]=(j/6==j%6?1.:0.);
    }
    CHECK(SetState(m->epoch,m->assigned,m->dimension)==0);
    int dimension=0;double *derivative=GetDerivatives(0.,1,&dimension);
    CHECK(derivative!=NULL && dimension==m->dimension);
    for(int block=0;block<m->cartesian;block+=6) {
      double *s=m->assigned+block,r=sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
      for(int j=0;j<3;++j) {
        CHECK(close_value(derivative[block+j],s[j+3],1e-12));
        CHECK(close_value(derivative[block+j+3],-398600.4418*s[j]/(r*r*r),1e-12));
      }
    }
    if(m->dimension==42) {
      double *s=m->assigned,r2=s[0]*s[0]+s[1]*s[1]+s[2]*s[2],r=sqrt(r2);
      for(int row=0;row<6;++row)for(int col=0;col<6;++col) {
        double expected=row<3?(col==row+3?1.:0.):col>=3?0.:
          398600.4418*(3*s[row-3]*s[col]/(r2*r2*r)-(row-3==col?1./(r2*r):0.));
        CHECK(close_value(derivative[6+row*6+col],expected,2e-13));
      }
    }
  }
  for(int i=model_count-1;i>=0;--i) {
    struct Model *m=&models[i];
    CHECK(FindOdeModel(m->name)==m->id && SetModelByName(m->name)==m->id);
    CHECK(GetStateSize()==m->dimension && GetState()!=NULL);
    for(int j=0;j<m->dimension;++j)CHECK(GetState()[j]==m->assigned[j]);
    int dimension=m->id;
    double *d=GetDerivativesForState(m->epoch,m->assigned,m->dimension,0.,1,&dimension);
    CHECK(d!=NULL && dimension==m->dimension);
    printf("NASA_REQUERY {\"name\":\"%s\",\"dimension\":%d,\"state_preserved\":true}\n",m->name,m->dimension);
  }
  CHECK(FindOdeModel("missing-model-for-acceptance")==-2);
  CHECK(GetStateSize()==0 && GetState()==NULL);
  CHECK(SetModel(models[0].id)==models[0].id);
  CHECK(RunScript()==run_status);
  CHECK(GetStateSize()==0 && GetState()==NULL && SetModel(models[0].id)==-1);
  CHECK(epochs(craft,craft_count)==0);
  CHECK(FindOdeModel(models[0].name)>=1000);
  CHECK(LoadScript("missing-script-for-acceptance.script")!=0);
  CHECK(GetStateSize()==0 && GetState()==NULL && SetModel(models[0].id)==-1);
  double output=12345.;CHECK(GetSpacecraftEpoch(craft[0].name,&output)==-2 && output==12345.);
  CHECK(LoadAndRunScript(argv[1])==run_status);
  CHECK(epochs(craft,craft_count)==0);
  CHECK(FindOdeModel(models[0].name)>=1000);
  CHECK(LoadAndRunScript("missing-script-for-acceptance.script")!=0);
  CHECK(strstr(getLastMessage(),"script")!=NULL);
  CHECK(GetStateSize()==0 && GetState()==NULL && SetModel(models[0].id)==-1);
  CHECK(LoadAndRunScript(NULL)==-1);
  printf("NASA_CHECK {\"checkpoint\":\"c-api-propagate-paths\",\"outcome\":\"passed\",\"assertions\":%d,\"models\":%d,\"spacecraft\":%d,\"public_ABI\":true,\"GUI_executions\":0}\n",assertions,model_count,craft_count);
  return 0;
}
