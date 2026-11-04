/* Independent external C ABI client. Expected command identities come from
 * the literal input mission, not an engine graph walker or GUI snapshot. */
#ifdef __cplusplus
#error Summary acceptance must compile as C
#endif
#include "CInterfaceFunctions.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>

static int assertions,compared;
#define CHECK(x) do { ++assertions; if (!(x)) {fprintf(stderr,"C_API_SUMMARY_ASSERT line %d: %s; latest interface text: %.160s\n",__LINE__,#x,getLastMessage());return 1;} } while(0)
static const char *names[]={"Before","ResetCounter","OuterLoop","BranchChoice","FirstArc","Alternate","SecondArc","CloseChoice","CloseLoop","CounterLoop","Increment","CloseCounter","NeverChoice","Unexecuted","CloseNever","ScriptBlock","ScriptValue","CloseScript","After","LateState","LateMass","Late"};
static const char *types[]={"Report","GMAT","For","If","Propagate","Else","Propagate","EndIf","EndFor","While","GMAT","EndWhile","If","Propagate","EndIf","BeginScript","GMAT","EndScript","Report","GMAT","GMAT","Report"};
static double observed[6][2][8];
static int save(const char *name,const char *data) {FILE *f=fopen(name,"wb");CHECK(f!=NULL);CHECK(fwrite(data,1,strlen(data),f)==strlen(data));CHECK(fclose(f)==0);return 0;}
static char *read(const char *name) {FILE *f=fopen(name,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);rewind(f);char *data=malloc((size_t)n+1);if(data){if(fread(data,1,(size_t)n,f)!=(size_t)n){free(data);data=NULL;}else data[n]=0;}fclose(f);return data;}
static char *copy(const char *s) {if(!s)return NULL;char *p=malloc(strlen(s)+1);if(p)strcpy(p,s);return p;}
static const char *locate(const char *summary,int index) {
  char normal[160],unavailable[160];const char *native=types[index],*display=strcmp(native,"GMAT")==0?"Equation":strcmp(native,"EndScript")==0?"ScriptEvent":native;
  snprintf(normal,sizeof(normal),"%s Command: %s\n",display,names[index]);snprintf(unavailable,sizeof(unavailable),"No summary data for %s command %s\n",native,names[index]);
  const char *a=strstr(summary,normal),*b=strstr(summary,unavailable);return a?a:b;
}
static int identities(const char *summary) {
  int total=0;for(const char *p=summary;(p=strstr(p," Command: "));++p)++total;
  for(const char *p=summary;(p=strstr(p,"No summary data for "));++p)++total;
  CHECK(total==23 && strstr(summary,"No summary data for BeginMissionSequence command ")!=NULL);
  const char *previous=summary;
  for(int i=0;i<22;++i){const char *p=locate(summary,i);if(!p)fprintf(stderr,"Missing command %d %s %s\n",i,types[i],names[i]);CHECK(p!=NULL && p>=previous);CHECK(locate(p+1,i)==NULL);previous=p;printf("C_API_SUMMARY_COMMAND %d %s %s\n",i,types[i],names[i]);}
  return 0;
}
static int field(const char *data,const char *label,double *value) {
  int found=0;size_t n=strlen(label);
  for(const char *line=data;line && *line;line=strchr(line,'\n')) {
    if(*line=='\n')++line;
    const char *p=line;while(*p==' ')++p;
    if(strncmp(p,label,n)==0){p+=n;while(*p==' ')++p;if(*p=='='){char *end;*value=strtod(p+1,&end);CHECK(end!=p+1 && isfinite(*value));++found;}}
    if(!strchr(line,'\n'))break;
  }
  CHECK(found==1);return 0;
}
static int cached(const char *summary,int id,double seconds,double shift,double mass,int row) {
  const char *start=locate(summary,id);CHECK(start!=NULL);const char *end=strstr(start,"\n-----------------------------------\n");CHECK(end!=NULL);size_t n=(size_t)(end-start);char *block=malloc(n+1);CHECK(block!=NULL);memcpy(block,start,n);block[n]=0;
  const char *labels[]={"X","Y","Z","VX","VY","VZ","Cd","Dry mass"};
  for(int sat=0;sat<2;++sat) {
    char title[64];snprintf(title,sizeof(title),"Spacecraft       : %c",'A'+sat);const char *first=strstr(block,title);CHECK(first!=NULL);const char *last=strstr(first+1,"Spacecraft       : ");if(!last)last=block+n;size_t length=(size_t)(last-first);char *part=malloc(length+1);CHECK(part!=NULL);memcpy(part,first,length);part[length]=0;CHECK(strstr(part,"Coordinate System: EarthMJ2000Eq")!=NULL);
    double r=sat?9000:7000,w=sqrt(398600.4415/(r*r*r)),angle=w*seconds;double expected[]={r*cos(angle)+shift,r*sin(angle),0,-r*w*sin(angle),r*w*cos(angle),0,2.5,mass};
    if(sat){expected[0]=0;expected[1]=9000;expected[2]=0;expected[3]=0;expected[4]=0;expected[5]=sqrt(398600.4415/9000);expected[6]=1.75;expected[7]=950;}
    for(int i=0;i<8;++i){double actual=0;CHECK(field(part,labels[i],&actual)==0);observed[row][sat][i]=actual;double tol=i<3?2e-5:i<6?4e-8:1e-7;if(fabs(actual-expected[i])>tol)fprintf(stderr,"Cached field %s %c %s actual %.17g expected %.17g\n",names[id],'A'+sat,labels[i],actual,expected[i]);CHECK(fabs(actual-expected[i])<=tol);++compared;}
    free(part);
  }
  free(block);return 0;
}
static int report(void) {
  FILE *f=fopen("output/cache.csv","r");CHECK(f!=NULL);
  for(int row=0;row<3;++row){double v[15];for(int i=0;i<15;++i){CHECK(fscanf(f," %lf",&v[i])==1);if(i<14)CHECK(fgetc(f)==',');}double seconds=row?20:0,r=7000,w=sqrt(398600.4415/(r*r*r));double expected[]={seconds,r*cos(w*seconds)+(row==2?1234:0),r*sin(w*seconds),0,-r*w*sin(w*seconds),r*w*cos(w*seconds),0,0,0,9000,0,0,0,sqrt(398600.4415/9000),row?42:0};for(int i=0;i<15;++i)CHECK(isfinite(v[i]) && fabs(v[i]-expected[i])<(i==0?1e-4:i==14?1e-12:2e-5));}
  int ch;do{ch=fgetc(f);}while(ch=='\n'||ch=='\r'||ch==' ');CHECK(ch==EOF);fclose(f);return 0;
}
int main(int argc,char **argv) {
  CHECK(argc==4 && strcmp(argv[2],"summary")==0);void *probe=dlopen(argv[3],RTLD_NOW);CHECK(probe!=NULL);int (*metadata)(uint64_t*)=NULL;*(void**)(&metadata)=dlsym(probe,"GmatCapiSummaryMetadata");CHECK(metadata!=NULL);CHECK(StartGmat()==0);FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);char line[4096];while(fgets(line,sizeof(line),maps)){char *path=strchr(line,'/');if(path)printf("NASA_LOADED %s",path);}fclose(maps);
  CHECK(LoadScript(argv[1])==0);CHECK(strstr(getLastMessage(),"success")!=NULL);CHECK(RunScript()==0);CHECK(strcmp(getLastMessage(),"Mission run succeeded!")==0);CHECK(report()==0);
  uint64_t before=0,after=0;CHECK(metadata(&before)==23);int count=CountObjects();char *raw=copy(GetRunSummary());CHECK(raw!=NULL && strlen(raw)>100);CHECK(save("summary-first.txt",raw)==0);CHECK(identities(raw)==0);CHECK(metadata(&after)==23 && before==after);
  CHECK(cached(raw,0,0,0,800,0)==0);CHECK(cached(raw,4,10,0,800,1)==0);CHECK(cached(raw,6,20,0,800,2)==0);CHECK(cached(raw,18,20,0,800,3)==0);CHECK(cached(raw,19,20,1234,800,4)==0);CHECK(cached(raw,21,20,1234,991,5)==0);
  const char *unexecuted=locate(raw,13),*end=strstr(unexecuted,"\n-----------------------------------\n");CHECK(end!=NULL);const char *sat=strstr(unexecuted,"Spacecraft       : A");CHECK(sat==NULL || sat>end);const char *warning=strstr(unexecuted,"did not execute due to control logic statements");CHECK(warning!=NULL && warning<end);
  /* Keep the shipped shared-message-buffer contract: textual API getters
   * publish their return text through getLastMessage, as GetObjectName does. */
  CHECK(strcmp(getLastMessage(),raw)==0);char *bytes=read("output/cache.csv");CHECK(bytes!=NULL);CHECK(strcmp(GetRunSummary(),raw)==0);CHECK(metadata(&after)==23 && before==after);CHECK(CountObjects()==count);char *again=read("output/cache.csv");CHECK(again!=NULL && strcmp(again,bytes)==0);free(again);free(bytes);
  CHECK(LoadScript("nonexistent-summary-recovery.script")!=0);CHECK(strlen(getLastMessage())>0);CHECK(LoadScript(argv[1])==0 && RunScript()==0);CHECK(strcmp(getLastMessage(),"Mission run succeeded!")==0);CHECK(strcmp(GetRunSummary(),raw)==0);CHECK(report()==0);
  printf("NASA_CHECK {\"checkpoint\":\"c-api-summary\",\"outcome\":\"passed\",\"assertions\":%d,\"commands\":23,\"named_commands\":22,\"metadata_unchanged\":true,\"cached_fields\":%d,\"report_rows\":3,\"recovery_runs\":1,\"summary_bytes\":%zu,\"cached\":[",assertions,compared,strlen(raw));
  for(int row=0;row<6;++row){printf("%s[",row?",":"");for(int sat=0;sat<2;++sat){printf("%s[",sat?",":"");for(int i=0;i<8;++i)printf("%s%.17g",i?",":"",observed[row][sat][i]);putchar(']');}putchar(']');}puts("]}");free(raw);return 0;
}
