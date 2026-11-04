// Private read-only metadata observer, never installed or registered as a plugin.
// It does not supply summary text, cached values or expected command identities.
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include <cstdint>
#include <set>
#include <vector>
extern "C" int GmatCapiSummaryMetadata(std::uint64_t *out) {
   if(!out)return -1;
   std::vector<GmatCommand*> pending{Moderator::Instance()->GetFirstCommand(1)};
   std::set<GmatCommand*> visited;
   std::uint64_t hash=14695981039346656037ULL;int count=0;
   while(!pending.empty()) {
      auto *command=pending.back();pending.pop_back();
      if(!command || !visited.insert(command).second)continue;
      if(command->GetTypeName()!="NoOp") {
         ++count;
         for(const auto &text:{command->GetName(),command->GetSummaryName()}) {
            for(unsigned char c:text){hash^=c;hash*=1099511628211ULL;}
            hash^=0xff;hash*=1099511628211ULL;
         }
      }
      pending.push_back(command->GetNextWhileExecuting());
      for(int i=0;auto *child=command->GetChildCommand(i);++i)pending.push_back(child);
   }
   *out=hash;return count;
}
