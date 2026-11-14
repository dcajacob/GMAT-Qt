#include "FileUtil.hpp"
#include <sstream>
#include <iostream>
#include <stdexcept>
struct Case{const char*name;std::string body;std::string error;Gmat::WrapperDataType type;int rows,cols;};
int main(){
 const Case cases[]={
  {"single-variable","Create Variable out;\nBeginMissionSequence;\nout = 1;\n","",Gmat::VARIABLE_WT,-1,-1},
  {"ignored-duplicate-declaration","Create Variable out;\nfunction [out] = Ignored(input)\nCreate Variable out;\n","",Gmat::VARIABLE_WT,-1,-1},
  {"ignored-type-and-shape-change","Create Array out[2,3];\nfunction [out] = Ignored(input)\nCreate Variable out;\n","",Gmat::ARRAY_WT,2,3},
  {"later-create-cannot-rescue-first","BeginMissionSequence;\nfunction [out] = Ignored(input)\nCreate Variable out;\n","Missing output declaration",Gmat::VARIABLE_WT,-1,-1},
  {"later-global-cannot-rescue-first","BeginMissionSequence;\nfunction [out] = Ignored(input)\nGlobal out;\n","Missing output declaration",Gmat::VARIABLE_WT,-1,-1},
  {"first-duplicate-still-rejected","Create Variable out;\nCreate Variable out;\nfunction [out] = Ignored(input)\nCreate Variable other;\n","Multiple declaration",Gmat::VARIABLE_WT,-1,-1},
  {"comment-and-keyword-prefix","% function [out] = Ignored(input)\nCreate Variable functionValue;\nfunctionValue = 1;\nCreate Variable out;\n","",Gmat::VARIABLE_WT,-1,-1},
  {"tab-delimited-header","Create Variable out;\n\tfunction\t[out] = Ignored(input)\nCreate Variable out;\n","",Gmat::VARIABLE_WT,-1,-1},
  {"bracket-adjacent-header","Create Variable out;\nfunction[out] = Ignored(input)\nCreate Variable out;\n","",Gmat::VARIABLE_WT,-1,-1}
 };
 int failed=0;for(const auto&test:cases){try{std::istringstream stream(test.body);StringArray inputs{"input"},outputs{"out"};std::string error;IntegerArray rows,cols;auto types=GmatFileUtil::GetFunctionOutputTypes(&stream,inputs,outputs,error,rows,cols);if(test.error.empty()){if(!error.empty()||types.size()!=1||rows.size()!=1||cols.size()!=1||types[0]!=test.type||rows[0]!=test.rows||cols[0]!=test.cols)throw std::runtime_error("Incorrect first-function output metadata: "+error);}else if(error.find(test.error)==std::string::npos)throw std::runtime_error("Expected first-function error missing: "+error);std::cout<<"PASS "<<test.name<<'\n';}catch(const std::exception&e){++failed;std::cerr<<"FAIL "<<test.name<<": "<<e.what()<<'\n';}}
 std::cout<<"Function-output scope cases: "<<9-failed<<"/9 passed\n";return failed?1:0;
}
