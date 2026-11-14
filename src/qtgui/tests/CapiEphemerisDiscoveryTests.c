/* Additional pure-C witnesses; reuse the previous bounded numeric helpers. */
#define main previous_capi_propagate_main
#include "CapiPropagateAcceptanceTests.c"
#undef main

static int inspect(struct Craft *craft,int craft_count,struct Model *models,int count) {
  CHECK(epochs(craft,craft_count)==0);
  /* Zero is the documented unnamed-lookup success status, not a model ID. */
  CHECK(FindOdeModel("")==0);
  CHECK(GetStateSize()==models[0].dimension);
  for(int i=0;i<count;++i) {
    struct Model *m=&models[i];m->id=FindOdeModel(m->name);CHECK(m->id>=1000);
    CHECK(SetModel(m->id)==m->id && GetStateSize()==m->dimension);
    CHECK(GetState()!=NULL);
    for(int j=0;j<m->cartesian;++j)CHECK(close_value(GetState()[j],m->state[j],j<3?2e-5:3e-8));
    printf("NASA_EPHEMERIS_MODEL {\"name\":\"%s\",\"dimension\":%d,\"state\":",m->name,m->dimension);
    vector(GetState(),m->dimension);printf("}\n");
    memcpy(m->assigned,GetState(),(size_t)m->dimension*sizeof(double));
    m->assigned[0]+=(i+1)*.125;
    CHECK(SetState(m->epoch,m->assigned,m->dimension)==0);
    int dimension=0;double *d=GetDerivatives(0.,1,&dimension);
    CHECK(d!=NULL && dimension==m->dimension);
    double *v=m->assigned,r=sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    for(int j=0;j<3;++j) {
      CHECK(close_value(d[j],v[j+3],1e-12));
      CHECK(close_value(d[j+3],-398600.4418*v[j]/(r*r*r),1e-12));
    }
  }
  for(int i=count-1;i>=0;--i) {
    struct Model *m=&models[i];CHECK(FindOdeModel(m->name)==m->id);
    CHECK(SetModelByName(m->name)==m->id && GetStateSize()==m->dimension);
    for(int j=0;j<m->dimension;++j)CHECK(GetState()[j]==m->assigned[j]);
  }
  /* Lookups must not run the SPK commands or move any epoch. */
  CHECK(epochs(craft,craft_count)==0);
  for(int i=0;i<craft_count;++i) {
    double actual=0;CHECK(GetSpacecraftEpoch(craft[i].name,&actual)==0);
    printf("NASA_UNADVANCED {\"name\":\"%s\",\"actual_A1Mjd\":%.17g,\"expected_A1Mjd\":%.17g}\n",
           craft[i].name,actual,craft[i].epoch);
  }
  CHECK(FindOdeModel("InternalODEModel")==-2);
  CHECK(GetStateSize()==0 && GetState()==NULL);
  return 0;
}

int main(int argc,char **argv) {
  setvbuf(stdout,NULL,_IONBF,0);
  CHECK(argc==5);
  FILE *f=fopen(argv[2],"r");CHECK(f!=NULL);
  int load_status,run_status,craft_count,model_count;
  CHECK(fscanf(f,"%d %d %d %d",&load_status,&run_status,&craft_count,&model_count)==4);
  CHECK(craft_count==4 && model_count==2);
  struct Craft craft[4];struct Model models[2];memset(models,0,sizeof(models));
  for(int i=0;i<craft_count;++i)CHECK(fscanf(f,"%63s %lf",craft[i].name,&craft[i].epoch)==2);
  for(int i=0;i<model_count;++i) {
    struct Model *m=&models[i];
    CHECK(fscanf(f,"%63s %d %d %lf",m->name,&m->dimension,&m->cartesian,&m->epoch)==4);
    CHECK((m->dimension==6 || m->dimension==42) && m->cartesian==6);
    for(int j=0;j<6;++j)CHECK(fscanf(f,"%lf",&m->state[j])==1);
  }
  CHECK(fclose(f)==0 && StartGmat()==0);
  FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);
  char line[4096];while(fgets(line,sizeof(line),maps)) {
    char *path=strchr(line,'/');if(path)printf("NASA_LOADED %s",path);
  }
  fclose(maps);
  int actual_load=LoadScript(argv[1]);
  printf("NASA_LOAD_STATUS %d\nNASA_LOAD_MESSAGE_BEGIN\n%s\nNASA_LOAD_MESSAGE_END\n",actual_load,getLastMessage());
  CHECK(actual_load==load_status);
  if(actual_load==0) {
    int actual_run=RunScript();
    printf("NASA_RUN_STATUS %d\nNASA_RUN_MESSAGE_BEGIN\n%s\nNASA_RUN_MESSAGE_END\n",actual_run,getLastMessage());
    CHECK(actual_run==run_status);
    if(actual_run==0 || actual_run==-4)CHECK(inspect(craft,craft_count,models,model_count)==0);
    else {
      CHECK(GetStateSize()==0 && GetState()==NULL && FindOdeModel("FM1")==-1);
      double value=12345.;CHECK(GetSpacecraftEpoch("A",&value)==-2 && value==12345.);
    }
  } else {
    CHECK(strstr(getLastMessage(),"EphemerisPolicy")!=NULL);
    CHECK(GetStateSize()==0 && GetState()==NULL && FindOdeModel("FM1")==-1);
  }
  /* Recovery uses the same known-good ephemeris-before-numerical script. */
  CHECK(LoadAndRunScript(argv[3])==0);
  CHECK(inspect(craft,craft_count,models,model_count)==0);
  printf("NASA_CHECK {\"checkpoint\":\"c-api-ephemeris-discovery\",\"outcome\":\"passed\",\"assertions\":%d,\"load_status\":%d,\"run_status\":%d,\"expected_rejection\":%s,\"public_ABI\":true,\"GUI_executions\":0}\n",
         assertions,actual_load,run_status,(load_status || (run_status && run_status!=-4))?"true":"false");
  return 0;
}
