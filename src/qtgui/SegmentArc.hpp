#pragma once
#include "PlotModel.hpp"

inline bool sameArcEndpoint(const PlotPoint &a,const PlotPoint &b)
{
   return a.epoch==b.epoch && a.x==b.x && a.y==b.y && a.z==b.z;
}
inline int arcDirection(const PlotRegularArc &arc)
{
   return (arc.last.epoch>arc.first.epoch)-(arc.last.epoch<arc.first.epoch);
}
inline const PlotPoint *firstRegularArcPose(const PlotModel &model,const QString &frame)
{
   const auto found=model.regularArcs.constFind(frame.section('.',0,0));
   if (found==model.regularArcs.cend()) return nullptr;
   for (const auto &arc:*found) if (arc.name==frame.section('.',1,1)) return &arc.last;
   return nullptr;
}
inline void recordRegularArcPose(PlotModel &model,const QString &object,const QString &provider,
                                 const QString &name,const PlotPoint &pose)
{
   auto &arcs=model.regularArcs[object];
   if (!arcs.isEmpty() && arcs.back().finalized && sameArcEndpoint(arcs.back().last,pose)) {
      if (arcs.back().providers.contains(provider)) arcs.back().finalized=false;
      else for (int first=0;first+1<arcs.size();++first) {
         if (!arcs[first].providers.contains(provider)) continue;
         bool connected=true; int direction=0;
         for (int i=first;i<arcs.size() && connected;++i) {
            const int next=arcDirection(arcs[i]);
            if (direction && next && direction!=next) connected=false;
            if (!direction) direction=next;
            if (i>first && !sameArcEndpoint(arcs[i-1].last,arcs[i].first)) connected=false;
         }
         if (!connected) continue;
         // OF retains the original segment name and unions command identities.
         for (int i=first+1;i<arcs.size();++i) {
            arcs[first].providers.unite(arcs[i].providers);
            arcs[first].members.unite(arcs[i].members);
         }
         arcs[first].last=arcs.back().last;
         arcs.resize(first+1); arcs.back().finalized=false; break;
      }
   }
   if (arcs.isEmpty() || arcs.back().finalized) {
      PlotRegularArc arc; arc.id=++model.nextRegularArc; arc.members.insert(arc.id); arc.name=name;
      arc.providers.insert(provider); arc.first=arc.last=pose; arcs.append(arc); return;
   }
   auto &arc=arcs.back(); const int direction=arcDirection(arc);
   // Like OF, duplicate epochs and nonmonotonic points do not extend an arc.
   if ((direction>0 && pose.epoch>arc.last.epoch) || (direction<0 && pose.epoch<arc.last.epoch) ||
       (!direction && pose.epoch!=arc.last.epoch)) arc.last=pose;
}
