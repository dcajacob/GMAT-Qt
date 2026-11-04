#ifndef PlotOriginScope_hpp
#define PlotOriginScope_hpp

#include "Publisher.hpp"
#include <map>
class SpaceObject;

// Read-only Cartesian state for a spacecraft used by a plot coordinate system.
// The value is available only during conversion of its synchronous packet.
class GMAT_API PlotOriginScope
{
public:
   // Index the view dependencies once for the entire immutable publication.
   class GMAT_API Packet
   {
   public:
      Packet(const Real *data, Integer count, SpaceObject *origin);
      Packet(const Real *data, Integer count, const std::vector<SpaceObject*> &objects);
   private:
      friend class PlotOriginScope;
      const Real *data;
      Integer count;
      std::vector<SpaceObject*> objects;
      const std::vector<Publisher::IntegrationStep> *steps;
      std::map<SpaceObject*,std::map<GmatTime,const Rvector6*>> byObject;
   };
   PlotOriginScope(const Packet &packet, const GmatTime &epoch);
   ~PlotOriginScope();
   PlotOriginScope(const PlotOriginScope &) = delete;
   PlotOriginScope &operator=(const PlotOriginScope &) = delete;
   bool Available() const { return states.size() == packet.objects.size(); }
   SpaceObject *MissingObject() const;
   static bool Epoch(const SpaceObject *object, Real mjd, GmatTime &epoch);
   static const Rvector6 *Find(const SpaceObject *object, Real mjd);
   static const Rvector6 *Find(const SpaceObject *object, const GmatTime &epoch);
private:
   const PlotOriginScope *previous;
   const Packet &packet;
   GmatTime epoch;
   std::map<const SpaceObject*,Rvector6> states;
   bool Matches(const SpaceObject *candidate) const;
};
#endif
