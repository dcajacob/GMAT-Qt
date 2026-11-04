/* External, pure-C client of the shipped CInterface ABI. Oracles below use
 * only Newton's equation and C math; no GMAT conversion/integration helpers. */
#ifdef __cplusplus
#error This acceptance client must be compiled as C
#endif
#include "CInterfaceFunctions.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

static const double mu = 398600.4415;
static int assertions;
#define CHECK(x) do { ++assertions; if (!(x)) { \
  fprintf(stderr,"C_API_ASSERTION line %d: %s; last message: %s\n", \
          __LINE__, #x, getLastMessage()); return 1; } } while (0)

static int near(double a, double b, double tol) {
  return isfinite(a) && isfinite(b) && fabs(a-b)<=tol;
}
static void newton(const double *s, double *d) {
  double r=sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
  for(int i=0;i<3;++i) { d[i]=s[i+3]; d[i+3]=-mu*s[i]/(r*r*r); }
}
static void rk4(double *s, double seconds) {
  const int steps=(int)ceil(fabs(seconds)/0.1);
  double h=seconds/steps;
  for(int k=0;k<steps;++k) {
    double a[6],b[6],c[6],d[6],t[6]; newton(s,a);
    for(int i=0;i<6;++i)t[i]=s[i]+h*a[i]/2.;
    newton(t,b);
    for(int i=0;i<6;++i)t[i]=s[i]+h*b[i]/2.;
    newton(t,c);
    for(int i=0;i<6;++i)t[i]=s[i]+h*c[i];
    newton(t,d);
    for(int i=0;i<6;++i)s[i]+=h*(a[i]+2*b[i]+2*c[i]+d[i])/6.;
  }
}
static void jacobian(const double *s, double a[36]) {
  double r2=s[0]*s[0]+s[1]*s[1]+s[2]*s[2], r=sqrt(r2);
  memset(a,0,36*sizeof(double));
  for(int i=0;i<3;++i) {
    a[i*6+i+3]=1.;
    for(int j=0;j<3;++j)
      a[(i+3)*6+j]=mu*(3*s[i]*s[j]/(r2*r2*r)-(i==j?1./(r2*r):0.));
  }
}
static void array(const double *s,int n) {
  putchar('['); for(int i=0;i<n;++i)printf("%s%.17g",i?",":"",s[i]); putchar(']');
}
int main(int argc,char **argv) {
  CHECK(argc==3 || argc==4);
  int epoch=strcmp(argv[2],"epoch")==0;
  int stm=strcmp(argv[2],"stm")==0 || epoch;
  CHECK(stm || strcmp(argv[2],"basic")==0);
  int (*observe_epochs)(double *,int)=NULL;
  if(epoch) {
    CHECK(argc==4);void *handle=dlopen(argv[3],RTLD_NOW);CHECK(handle!=NULL);
    *(void **)(&observe_epochs)=dlsym(handle,"GmatCapiObserveEpochs");
    CHECK(observe_epochs!=NULL);
  }
  CHECK(StartGmat()==0);
  /* Identify the actual loaded project libraries, including runtime plugins. */
  FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);
  char mapline[4096];
  while(fgets(mapline,sizeof(mapline),maps)) {
    char *path=strchr(mapline,'/');
    if(path)printf("NASA_LOADED %s",path);
  }
  fclose(maps);
  CHECK(strstr(getLastMessage(),"initialized")!=NULL);
  CHECK(LoadScript("nonexistent-acceptance-input.script")!=0);
  CHECK(strlen(getLastMessage())>0);
  CHECK(LoadScript(argv[1])==0);
  CHECK(strstr(getLastMessage(),"success")!=NULL);
  const char *expected[]={"EarthMJ2000Eq","EarthMJ2000Ec","EarthFixed","EarthICRF",
    "SolarSystemBarycenter","A","B","FM1","FM2","P1","P2","A.ElapsedSecs","B.ElapsedSecs"};
  int found[13]={0};
  CHECK(CountObjects()==13);
  for(int i=0;i<13;++i) {
    const char *name=GetObjectName(i); CHECK(name!=NULL);
    int index=-1;
    for(int j=0;j<13;++j)if(strcmp(name,expected[j])==0)index=j;
    CHECK(index>=0 && !found[index]); found[index]=1;
  }
  CHECK(RunScript()==0);
  CHECK(strcmp(getLastMessage(),"Mission run succeeded!")==0);
  int ids[2]; ids[0]=FindOdeModel("FM1"); ids[1]=FindOdeModel("FM2");
  CHECK(ids[0]>=1000 && ids[1]>=1000 && ids[0]!=ids[1]);
  double initial[2][6]={{7000,0,0,0,0,0},{0,9000,0,0,0,0}};
  initial[0][4]=sqrt(mu/7000); initial[1][5]=sqrt(mu/9000);
  double epochs[2][5]={{0}};
  double seconds[2]={60,30},post[2][42]={{0}},derivatives[2][42]={{0}},given[2][42]={{7100,-250,330,.2,7.2,-.3},{-300,9400,1200,6.1,-.2,.1}};
  int n=stm?42:6;
  for(int m=0;m<2;++m) {
    CHECK(SetModel(ids[m])==ids[m]); CHECK(GetStateSize()==n);
    char expected_description[2048]="",line[80];
    for(int i=1;i<=6;++i) {
      snprintf(line,sizeof(line),"   %c.CartesianState.%d\n",'A'+m,i);
      strcat(expected_description,line);
    }
    if(stm)for(int i=1;i<=36;++i) {
      snprintf(line,sizeof(line),"   %c.STM.%d\n",'A'+m,i);
      strcat(expected_description,line);
    }
    CHECK(strcmp(GetStateDescription(),expected_description)==0);
    CHECK(GetState()!=NULL); memcpy(post[m],GetState(),n*sizeof(double));
    double r=m?9000:7000, w=sqrt(mu/(r*r*r)),s=sin(w*seconds[m]),c=cos(w*seconds[m]);
    double analytic[6]={r*c,r*s,0,-r*w*s,r*w*c,0};
    if(m) { analytic[0]=0;analytic[1]=r*c;analytic[2]=r*s;
      analytic[3]=0;analytic[4]=-r*w*s;analytic[5]=r*w*c; }
    for(int i=0;i<6;++i)CHECK(near(post[m][i],analytic[i],i<3?2e-5:3e-8));
    if(stm)for(int j=0;j<6;++j) {
      double plus[6],minus[6],epsilon=j<3?.01:1e-5;
      memcpy(plus,initial[m],sizeof(plus));memcpy(minus,initial[m],sizeof(minus));
      plus[j]+=epsilon;minus[j]-=epsilon;rk4(plus,seconds[m]);rk4(minus,seconds[m]);
      for(int i=0;i<6;++i)CHECK(near(post[m][6+i*6+j],(plus[i]-minus[i])/(2*epsilon),3e-6));
    }
    int dimension=0;double *d=GetDerivatives(0.,1,&dimension),want[6];
    CHECK(d!=NULL && dimension==n);newton(post[m],want);
    for(int i=0;i<6;++i)CHECK(near(d[i],want[i],1e-12));
    if(stm)for(int i=0;i<6;++i)given[m][6+i*6+i]=1.;
    CHECK(SetState(21545.25+m,given[m],n)==0);
    CHECK(strstr(getLastMessage(),"updated")!=NULL);
    if(epoch) {
      CHECK(observe_epochs(epochs[m],5)==5);
      for(int i=0;i<5;++i) {
        printf("C_API_EPOCH model=%d element=%d actual=%.17g expected=%.17g\n",m,i,epochs[m][i],21545.25+m);
        CHECK(epochs[m][i]==21545.25+m);
      }
      CHECK(SetState(-1.,given[m],n)==-1);
      CHECK(strcmp(getLastMessage(),"Error setting the epoch on the ODE model")==0);
      for(int i=0;i<n;++i)CHECK(GetState()[i]==given[m][i]);
      CHECK(observe_epochs(epochs[m],5)==5);
      for(int i=0;i<5;++i)CHECK(epochs[m][i]==21545.25+m);
    }
    for(int i=0;i<n;++i)CHECK(GetState()[i]==given[m][i]);
    /* Independent A-matrix: STM rate at identity equals the Jacobian. */
    dimension=0;d=GetDerivatives(0.,1,&dimension);newton(given[m],want);
    CHECK(d!=NULL && dimension==n);
    for(int i=0;i<6;++i)CHECK(near(d[i],want[i],1e-12));
    if(stm) {
      double a[36];jacobian(given[m],a);
      for(int i=0;i<36;++i)CHECK(near(d[6+i],a[i],2e-13));
      /* Nonidentity Phi proves STMdot=A*Phi, not merely an A-shaped array. */
      for(int i=0;i<6;++i)for(int j=0;j<6;++j)
        given[m][6+i*6+j]=(i==j?1.:0.)+.01*(i+1)*(j+2);
      dimension=ids[m];d=GetDerivativesForState(21545.75+m,given[m],n,0.,1,&dimension);
      CHECK(d!=NULL && dimension==n);
      for(int i=0;i<6;++i)for(int j=0;j<6;++j) {
        double value=0;for(int k=0;k<6;++k)value+=a[i*6+k]*given[m][6+k*6+j];
        CHECK(near(d[6+i*6+j],value,2e-12));
      }
    } else {
      dimension=ids[m];d=GetDerivativesForState(21545.75+m,given[m],n,0.,1,&dimension);
      CHECK(d!=NULL && dimension==n);
      for(int i=0;i<6;++i)CHECK(near(d[i],want[i],1e-12));
    }
    memcpy(derivatives[m],d,n*sizeof(double));
    if(epoch) {
      CHECK(observe_epochs(epochs[m],5)==5);
      for(int i=0;i<5;++i)CHECK(epochs[m][i]==21545.75+m);
    }
  }
  /* Round trips through both selectors must preserve each model's own state. */
  for(int m=1;m>=0;--m) {
    CHECK(SetModelByName(m?"FM2":"FM1")==ids[m]);
    for(int i=0;i<n;++i)CHECK(GetState()[i]==given[m][i]);
    if(epoch) {
      CHECK(observe_epochs(epochs[m],5)==5);
      for(int i=0;i<5;++i)CHECK(epochs[m][i]==21545.75+m);
    }
  }
  CHECK(SetModel(-100)==-1);
  CHECK(strstr(getLastMessage(),"not in the table")!=NULL);
  printf("NASA_CHECK {\"checkpoint\":\"c-api-%s\",\"outcome\":\"passed\",\"assertions\":%d,\"objects\":13,\"models\":2,\"dimension\":%d,\"post_run\":[",argv[2],assertions,n);
  array(post[0],n);putchar(',');array(post[1],n);
  printf("],\"assigned\":[");array(given[0],n);putchar(',');array(given[1],n);
  printf("],\"derivatives\":[");array(derivatives[0],n);putchar(',');array(derivatives[1],n);
  if(epoch) {printf("],\"epochs\":[");array(epochs[0],5);putchar(',');array(epochs[1],5);}
  printf("]}\n");
  return 0;
}
