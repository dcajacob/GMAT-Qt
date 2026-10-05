#pragma once
#include <QMap>
#include <QString>
#include <QStringList>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

// Absolute A1MJD clocks are owned independently of viewer windows. A child's
// controls address its ultimate parent, including the most recent member range.
class PlotSimulationClocks
{
public:
   struct State {
      double epoch=0,minimum=0,maximum=0,rate=3600,multiplier=1;
      bool hasRange=false,playing=false;
      int mode=1;
   };
   void clear() { nodes.clear(); }
   void configure(const QString &name,std::optional<double> rate,const QString &parent={}) {
      if ((rate && !std::isfinite(*rate)) || (!parent.isEmpty() && !rate)) throw std::runtime_error("Invalid simulation clock settings");
      auto previous=nodes; auto &node=nodes[name]; const bool first=!node.configured;
      State inherited=state(name);
      node.configured=true; node.enabled=rate.has_value(); node.parent=parent;
      try { (void)root(name); } catch (...) { nodes=previous; throw; }
      if (first) node.clock.rate=rate.value_or(3600);
      else if (previous[name].parent!=parent && parent.isEmpty()) node.clock=inherited;
      else if (node.savedRate!=rate) node.clock.rate=rate.value_or(3600);
      node.savedRate=rate;
   }
   QString root(const QString &name) const {
      QString current=name; QSet<QString> seen;
      while (nodes.contains(current) && !nodes[current].parent.isEmpty()) {
         if (seen.contains(current)) throw std::runtime_error("Simulation clock parents form a cycle");
         seen.insert(current); current=nodes[current].parent;
      }
      return current;
   }
   bool enabled(const QString &name) const {
      if (nodes.value(name).enabled) return true;
      for (auto it=nodes.cbegin();it!=nodes.cend();++it) if (it->enabled && root(it.key())==name) return true;
      return false;
   }
   QStringList members(const QString &name) const {
      const auto parent=root(name); QStringList result;
      for (auto it=nodes.cbegin();it!=nodes.cend();++it) if (enabled(it.key()) && root(it.key())==parent) result.append(it.key());
      return result;
   }
   State state(const QString &name) const { return nodes.value(root(name)).clock; }
   void publish(const QString &name,double epoch) {
      if (!enabled(name) || !std::isfinite(epoch)) return;
      auto &node=nodes[name];
      if (!node.range) node.range=std::pair<double,double>{epoch,epoch};
      else { node.range->first=std::min(node.range->first,epoch); node.range->second=std::max(node.range->second,epoch); }
      auto &clock=nodes[root(name)].clock;
      clock.minimum=node.range->first; clock.maximum=node.range->second;
      clock.epoch=epoch; clock.hasRange=true; clock.playing=false;
   }
   void discard(const QString &name,bool groupSurvives) {
      if (!nodes.contains(name)) return;
      nodes[name].range.reset();
      if (!groupSurvives) {
         auto &node=nodes[root(name)]; node.clock=State(); node.configured=false;
         nodes[name].configured=false;
      }
   }
   void reset(const QString &name) {
      auto &node=nodes[name]; node.range.reset(); auto &clock=nodes[root(name)].clock;
      clock.hasRange=false; clock.playing=false;
   }
   void seek(const QString &name,double epoch) {
      auto &clock=nodes[root(name)].clock;
      if (clock.hasRange && std::isfinite(epoch)) clock.epoch=std::clamp(epoch,clock.minimum,clock.maximum);
   }
   void setPlaying(const QString &name,bool playing) {
      auto &clock=nodes[root(name)].clock;
      const bool resume=playing && !clock.playing && clock.hasRange;
      clock.playing=playing && clock.hasRange;
      if (resume && clock.rate>=0 && clock.epoch>=clock.maximum) clock.epoch=clock.minimum;
      else if (resume && clock.rate<0 && clock.epoch<=clock.minimum) clock.epoch=clock.maximum;
   }
   void setRate(const QString &name,double rate) {
      if (!std::isfinite(rate)) throw std::runtime_error("Simulation rate must be finite");
      auto &clock=nodes[root(name)].clock; clock.rate=rate; clock.multiplier=1;
   }
   void setMultiplier(const QString &name,double multiplier) {
      if (!std::isfinite(multiplier) || multiplier<=0) throw std::runtime_error("Playback multiplier must be positive");
      nodes[root(name)].clock.multiplier=multiplier;
   }
   void setMode(const QString &name,int mode) { nodes[root(name)].clock.mode=mode; }
   void pauseAll() { for (auto &node:nodes) node.clock.playing=false; }
   QStringList advance(double seconds) {
      QStringList changed; if (!std::isfinite(seconds) || seconds<0) return changed;
      for (auto it=nodes.begin();it!=nodes.end();++it) if (it->parent.isEmpty() && enabled(it.key()) && it->clock.playing && it->clock.hasRange) {
         auto &clock=it->clock;
         const long double next=clock.epoch+static_cast<long double>(clock.rate)*clock.multiplier*seconds/86400.L;
         clock.epoch=static_cast<double>(std::clamp(next,static_cast<long double>(clock.minimum),static_cast<long double>(clock.maximum)));
         // Legacy clamps without wrapping; Play remains on until paused.
         changed.append(it.key());
      }
      return changed;
   }
   bool playing() const { for (auto it=nodes.cbegin();it!=nodes.cend();++it) if (it->parent.isEmpty() && enabled(it.key()) && it->clock.playing) return true; return false; }
private:
   struct Node { bool configured=false,enabled=false; QString parent; std::optional<double> savedRate; std::optional<std::pair<double,double>> range; State clock; };
   QMap<QString,Node> nodes;
};
