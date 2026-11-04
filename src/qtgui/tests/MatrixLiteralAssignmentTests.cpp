#include "Array.hpp"
#include "ArrayWrapper.hpp"
#include "Assignment.hpp"
#include "BaseException.hpp"
#include "ElementWrapper.hpp"
#include "StringWrapper.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message)
{
   if(!value)throw std::runtime_error(message);
}

int main()
{
   try
   {
      int accepted=0,rejected=0;
      for(auto dimensions:{std::make_pair(1,1),std::make_pair(2,3),std::make_pair(6,6)})
      {
         Array array("Target");array.SetSize(dimensions.first,dimensions.second);
         ArrayWrapper destination;destination.SetDescription("Target");destination.SetRefObject(&array);
         std::string literal="[";
         for(int row=0;row<dimensions.first;++row)
         {
            if(row)literal+="; ";
            for(int col=0;col<dimensions.second;++col)
            {
               if(col)literal+=" ";
               literal+=std::to_string(10*row+col+1);
            }
         }
         literal+="]";
         StringWrapper source;source.SetDescription(literal);
         require(ElementWrapper::SetValue(&destination,&source,nullptr,nullptr,nullptr),"Matrix literal assignment failed");
         const auto &actual=array.GetRmatrixParameter("RmatValue");
         for(int row=0;row<dimensions.first;++row)
            for(int col=0;col<dimensions.second;++col)
               require(actual(row,col)==10*row+col+1,"Matrix literal changed row-major values");
         Assignment command;command.SetLHS("Target");command.SetRHS(literal);
         auto *left=new ArrayWrapper();left->SetDescription("Target");left->SetRefObject(&array);
         auto *right=new StringWrapper();right->SetDescription(literal);
         require(command.SetElementWrapper(left,"Target")&&command.SetElementWrapper(right,literal)&&command.Validate(),"Valid matrix literal failed command validation");
         ++accepted;
         const Rmatrix before=actual;
         for(const std::string invalid:{"[]","[1 2]","[invalid]","[1e999]","[inf]","[nan]","not_a_matrix"})
         {
            StringWrapper bad;bad.SetDescription(invalid);bool threw=false;
            try{ElementWrapper::SetValue(&destination,&bad,nullptr,nullptr,nullptr);}catch(BaseException&){threw=true;}
            require(threw,"Invalid matrix literal was silently accepted");
            const auto &after=array.GetRmatrixParameter("RmatValue");
            for(int row=0;row<dimensions.first;++row)
               for(int col=0;col<dimensions.second;++col)
                  require(after(row,col)==before(row,col),"Invalid matrix literal mutated destination");
            ++rejected;
         }
      }
      std::cout<<"PASS: "<<accepted<<" literal shapes and "<<rejected<<" rejected transfers; rectangular order and failure atomicity retained.\n";
      return 0;
   }
   catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';}
   catch(const std::exception &e){std::cerr<<e.what()<<'\n';}
   return 1;
}
