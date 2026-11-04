/* Reuse every original C numeric/identity helper unchanged. This entry point
 * adds the failed-run/active-branch traversal and successful reload boundary. */
#define main normal_summary_main
#include "NasaCapiSummaryTests.c"
#undef main
static int stopped_identities(const char *raw,int runtime_error) {
  int total=0;for(const char *p=raw;(p=strstr(p," Command: "));++p)++total;
  for(const char *p=raw;(p=strstr(p,"No summary data for "));++p)++total;
  CHECK(total==(runtime_error?25:24));const char *previous=raw;
  for(int i=0;i<22;++i){const char *p=locate(raw,i);CHECK(p!=NULL && p>=previous);CHECK(locate(p+1,i)==NULL);previous=p;}
  if(runtime_error){const char *bad=strstr(raw,"Equation Command: BadArgument\n"),*error=strstr(raw,"Equation Command: RuntimeError\n");CHECK(bad!=NULL && error!=NULL && bad>locate(raw,4) && error>bad && error<locate(raw,5));}
  else {const char *halt=strstr(raw,"Stop Command: Halt\n");if(!halt)halt=strstr(raw,"No summary data for Stop command Halt\n");CHECK(halt!=NULL && halt>locate(raw,4) && halt<locate(raw,5));}return 0;
}
static int unavailable(const char *raw,int index) {
  const char *start=locate(raw,index);CHECK(start!=NULL);const char *end=strstr(start,"\n-----------------------------------\n");CHECK(end!=NULL);const char *sat=strstr(start,"Spacecraft       : A"),*warning=strstr(start,"did not execute due to control logic statements");CHECK((sat==NULL || sat>end) && warning!=NULL && warning<end);return 0;
}
static int first_report(void) {
  FILE *f=fopen("output/cache.csv","r");CHECK(f!=NULL);const double expected[]={0,7000,0,0,0,7.5460532872678359,0,0,0,9000,0,0,0,6.6549934593840474,0};
  for(int i=0;i<15;++i){double value=0;CHECK(fscanf(f," %lf",&value)==1 && isfinite(value) && fabs(value-expected[i])<1e-10);if(i<14)CHECK(fgetc(f)==',');}int ch;do{ch=fgetc(f);}while(ch=='\n'||ch=='\r'||ch==' ');CHECK(ch==EOF);fclose(f);return 0;
}
static void matrix(double values[6][2][8],int count) {
  putchar('[');for(int row=0;row<count;++row){printf("%s[",row?",":"");for(int sat=0;sat<2;++sat){printf("%s[",sat?",":"");for(int i=0;i<8;++i)printf("%s%.17g",i?",":"",values[row][sat][i]);putchar(']');}putchar(']');}putchar(']');
}
int main(int argc,char **argv) {
  CHECK(argc==4 && (strcmp(argv[2],"stopped")==0 || strcmp(argv[2],"runtimeerror")==0));const int runtime_error=strcmp(argv[2],"runtimeerror")==0,commands=runtime_error?25:24;void *probe=dlopen(argv[3],RTLD_NOW);CHECK(probe!=NULL);int (*metadata)(uint64_t*)=NULL;*(void**)(&metadata)=dlsym(probe,"GmatCapiSummaryMetadata");CHECK(metadata!=NULL);CHECK(StartGmat()==0);
  FILE *maps=fopen("/proc/self/maps","r");CHECK(maps!=NULL);char line[4096];while(fgets(line,sizeof(line),maps)){char *path=strchr(line,'/');if(path)printf("NASA_LOADED %s",path);}fclose(maps);
  CHECK(LoadScript(argv[1])==0);CHECK(RunScript()==(runtime_error?-5:-4));CHECK(strcmp(getLastMessage(),runtime_error?"Exception thrown during the run":"Execution interrupted by user")==0);CHECK(first_report()==0);
  uint64_t before=0,after=0;CHECK(metadata(&before)==commands);int count=CountObjects();char *raw=copy(GetRunSummary());CHECK(raw!=NULL && save("summary-stopped.txt",raw)==0);CHECK(stopped_identities(raw,runtime_error)==0);CHECK(metadata(&after)==commands && before==after);
  CHECK(cached(raw,0,0,0,800,0)==0);CHECK(cached(raw,4,10,0,800,1)==0);CHECK(unavailable(raw,6)==0 && unavailable(raw,18)==0 && unavailable(raw,13)==0);
  double primary[6][2][8];memcpy(primary,observed,sizeof(primary));char *bytes=read("output/cache.csv");CHECK(bytes!=NULL && strcmp(getLastMessage(),raw)==0);CHECK(save("cache-before-recovery.csv",bytes)==0);CHECK(strcmp(GetRunSummary(),raw)==0 && CountObjects()==count);CHECK(metadata(&after)==commands && before==after);char *again=read("output/cache.csv");CHECK(again!=NULL && strcmp(bytes,again)==0);free(bytes);free(again);
  CHECK(LoadScript("recovery.script")==0 && RunScript()==0);CHECK(strcmp(getLastMessage(),"Mission run succeeded!")==0);CHECK(metadata(&before)==23);char *recovered=copy(GetRunSummary());CHECK(recovered!=NULL && save("summary-recovered.txt",recovered)==0);CHECK(identities(recovered)==0 && metadata(&after)==23 && before==after);
  CHECK(cached(recovered,0,0,0,800,0)==0);CHECK(cached(recovered,4,10,0,800,1)==0);CHECK(cached(recovered,6,20,0,800,2)==0);CHECK(cached(recovered,18,20,0,800,3)==0);CHECK(cached(recovered,19,20,1234,800,4)==0);CHECK(cached(recovered,21,20,1234,991,5)==0);CHECK(report()==0);
  printf("NASA_CHECK {\"checkpoint\":\"c-api-summary-%s\",\"outcome\":\"passed\",\"assertions\":%d,\"commands\":%d,\"named_commands\":%d,\"metadata_unchanged\":true,\"cached_fields\":%d,\"status\":%d,\"report_rows_before_recovery\":1,\"recovery_runs\":1,\"summary_bytes\":%zu,\"cached\":",argv[2],assertions,commands,commands-1,compared,runtime_error?-5:-4,strlen(raw));matrix(primary,2);printf(",\"recovered_cached\":");matrix(observed,6);puts("}");free(raw);free(recovered);return 0;
}
