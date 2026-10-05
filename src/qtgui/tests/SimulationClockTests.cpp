#include "PlotSimulationClock.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition,const char *message) {
   if (!condition) throw std::runtime_error(message);
}
void near(double actual,double expected,const char *message) {
   require(std::abs(actual-expected)<1e-12,message);
}
void same(const PlotSimulationClocks::State &actual,const PlotSimulationClocks::State &expected,const char *message) {
   require(actual.epoch==expected.epoch && actual.minimum==expected.minimum && actual.maximum==expected.maximum &&
      actual.rate==expected.rate && actual.multiplier==expected.multiplier && actual.hasRange==expected.hasRange &&
      actual.playing==expected.playing && actual.mode==expected.mode,message);
}
template<class Operation> void rejects(Operation operation,const char *message) {
   bool rejected=false;
   try { operation(); } catch (const std::runtime_error &) { rejected=true; }
   require(rejected,message);
}

void independentClock() {
   PlotSimulationClocks clocks;
   require(!clocks.enabled("A") && clocks.root("A")=="A" && clocks.members("A").isEmpty(),"Unconfigured clock is active");
   clocks.configure("A",std::nullopt);
   clocks.publish("A",10);
   require(!clocks.state("A").hasRange && !clocks.playing(),"Absent clock override admitted publication");
   clocks.configure("A",3600);
   clocks.publish("A",10); clocks.publish("A",20);
   require(clocks.state("A").minimum==10 && clocks.state("A").maximum==20 && !clocks.state("A").playing,"Independent publication envelope changed");
   clocks.setPlaying("A",true);
   near(clocks.state("A").epoch,10,"Forward Play did not restart at minimum");
   require(clocks.advance(24)==QStringList{"A"},"Independent advance did not name root");
   near(clocks.state("A").epoch,11,"Seconds/day rate conversion disagrees");
   clocks.setMultiplier("A",0.5); clocks.advance(48);
   near(clocks.state("A").epoch,12,"Playback multiplier disagrees");
   clocks.seek("A",-100); near(clocks.state("A").epoch,10,"Seek did not clamp minimum");
   clocks.seek("A",100); near(clocks.state("A").epoch,20,"Seek did not clamp maximum");
   clocks.setRate("A",0);
   require(clocks.state("A").multiplier==1,"Rate change did not clear multiplier");
   clocks.seek("A",20); clocks.setPlaying("A",false); clocks.setPlaying("A",true);
   near(clocks.state("A").epoch,10,"Paused zero-rate Play at maximum did not restart at minimum");
   clocks.advance(10000); near(clocks.state("A").epoch,10,"Zero-rate endpoint restart did not hold");
   clocks.seek("A",20); clocks.setPlaying("A",true);
   near(clocks.state("A").epoch,20,"Repeated Play restarted an already-playing zero-rate clock");
   clocks.advance(1); near(clocks.state("A").epoch,20,"Already-playing zero-rate clock did not hold maximum");
   clocks.setPlaying("A",false); clocks.setPlaying("A",true);
   near(clocks.state("A").epoch,10,"Pause then Play did not restart zero-rate maximum");
   clocks.seek("A",12); clocks.setPlaying("A",true); clocks.advance(10000);
   near(clocks.state("A").epoch,12,"Zero rate did not hold epoch");
   require(clocks.playing(),"Zero rate incorrectly paused clock");
   clocks.setRate("A",-86400); clocks.seek("A",10); clocks.setPlaying("A",false); clocks.setPlaying("A",true);
   near(clocks.state("A").epoch,20,"Reverse Play did not restart at maximum");
   clocks.advance(2); near(clocks.state("A").epoch,18,"Reverse rate advance disagrees");
   clocks.advance(100); near(clocks.state("A").epoch,10,"Reverse clock did not clamp at minimum");
   clocks.advance(1); near(clocks.state("A").epoch,10,"Reverse boundary wrapped");
   require(clocks.playing(),"Legacy clamped playback unexpectedly paused");
   clocks.pauseAll(); require(!clocks.playing() && clocks.advance(1).isEmpty(),"Pause All left clock advancing");
}

void parentEnvelopeAndDetach() {
   PlotSimulationClocks clocks;
   clocks.configure("A",3600); clocks.configure("B",1,"A"); clocks.configure("C",7200,"B");
   require(clocks.root("B")=="A" && clocks.root("C")=="A" && clocks.members("C")==QStringList{"A","B","C"},"Ultimate parent or group members disagree");
   clocks.publish("A",10); clocks.publish("A",20);
   clocks.publish("B",30); clocks.publish("B",31);
   require(clocks.state("C").minimum==30 && clocks.state("C").maximum==31 && clocks.state("C").epoch==31,"Child publisher used union instead of own envelope");
   clocks.publish("A",12);
   require(clocks.state("B").minimum==10 && clocks.state("B").maximum==20 && clocks.state("B").epoch==12,"Root publisher lost its lifetime envelope");
   clocks.publish("C",-2); clocks.publish("C",-1);
   require(clocks.state("A").minimum==-2 && clocks.state("A").maximum==-1,"Nested child did not replace active envelope");
   clocks.setRate("C",3); clocks.setMultiplier("B",2); clocks.setMode("C",0);
   clocks.configure("B",1,"A");
   require(clocks.state("A").rate==3 && clocks.state("A").multiplier==2 && clocks.state("A").mode==0,"Child control or unchanged configure altered parent settings");
   clocks.publish("B",32); clocks.setPlaying("C",true); clocks.seek("B",31);
   const auto inherited=clocks.state("C");
   clocks.configure("C",7200);
   require(clocks.root("C")=="C" && clocks.members("A")==QStringList{"A","B"} && clocks.members("C")==QStringList{"C"},"Detach did not split group");
   same(clocks.state("C"),inherited,"Detach did not retain inherited rate/epoch/range/mode/multiplier/play state");
   clocks.setRate("A",7);
   same(clocks.state("C"),inherited,"Detached clock still followed former parent");
   clocks.publish("C",0);
   require(clocks.state("C").minimum==-2 && clocks.state("C").maximum==0 && !clocks.state("C").playing,"Detached publication did not restore publisher's own envelope");
}

void childBeforeParent() {
   PlotSimulationClocks clocks;
   clocks.configure("Child",100,"Late");
   require(clocks.enabled("Late") && clocks.root("Child")=="Late","Missing parent placeholder is not available to child");
   clocks.publish("Child",5); clocks.publish("Child",7); clocks.seek("Child",6);
   clocks.setMultiplier("Child",2); clocks.setMode("Child",0); clocks.setPlaying("Child",true);
   auto expected=clocks.state("Child");
   // First configuration supplies the saved parent rate. Its placeholder state
   // already carries the child's epoch/envelope and controls; those survive.
   expected.rate=-10;
   clocks.configure("Late",-10);
   same(clocks.state("Child"),expected,"First parent configuration discarded placeholder clock state");
   require(clocks.members("Child")==QStringList{"Child","Late"},"Configured parent missing from shared group");
   clocks.setRate("Child",7); clocks.configure("Late",-10);
   require(clocks.state("Late").rate==7,"Unchanged saved parent settings reset user runtime rate");
   clocks.publish("Late",40);
   require(clocks.state("Child").minimum==40 && clocks.state("Child").maximum==40 && clocks.state("Child").epoch==40,"Late parent's own publication envelope was not authoritative");
}

void invalidSettingsAreAtomic() {
   PlotSimulationClocks clocks;
   clocks.configure("A",100); clocks.configure("B",200,"A"); clocks.configure("C",300,"B");
   clocks.publish("A",10); clocks.publish("A",20); clocks.seek("C",12); clocks.setPlaying("B",true);
   clocks.setMultiplier("C",2); clocks.setMode("B",0);
   const auto original=clocks.state("A");
   const auto names=clocks.members("A");
   rejects([&]{clocks.configure("A",100,"C");},"Cycle accepted");
   require(clocks.root("A")=="A" && clocks.root("B")=="A" && clocks.root("C")=="A" && clocks.members("A")==names,"Rejected cycle changed graph");
   same(clocks.state("C"),original,"Rejected cycle changed clock state");
   rejects([&]{clocks.configure("Self",1,"Self");},"Self-parent accepted");
   require(!clocks.enabled("Self") && clocks.members("Self").isEmpty(),"Rejected self-parent left configured clock");
   rejects([&]{clocks.configure("C",std::nullopt,"A");},"Parent without simulation rate accepted");
   const double nan=std::numeric_limits<double>::quiet_NaN();
   const double infinity=std::numeric_limits<double>::infinity();
   for (double invalid : {nan,infinity,-infinity}) {
      rejects([&]{clocks.configure("A",invalid);},"Nonfinite configured rate accepted");
      rejects([&]{clocks.setRate("C",invalid);},"Nonfinite runtime rate accepted");
      rejects([&]{clocks.setMultiplier("B",invalid);},"Nonfinite multiplier accepted");
      clocks.publish("A",invalid); clocks.seek("C",invalid);
      require(clocks.advance(invalid).isEmpty(),"Nonfinite wall time advanced clock");
   }
   rejects([&]{clocks.setMultiplier("A",0);},"Zero multiplier accepted");
   rejects([&]{clocks.setMultiplier("A",-1);},"Negative multiplier accepted");
   require(clocks.advance(-1).isEmpty(),"Negative wall time advanced clock");
   same(clocks.state("A"),original,"Invalid mutation changed existing clock state");
   require(clocks.members("A")==names && clocks.root("C")=="A","Invalid mutation changed existing graph");
}

void disabledRootAndReset() {
   PlotSimulationClocks clocks;
   clocks.configure("A",86400); clocks.publish("A",1); clocks.publish("A",3); clocks.setPlaying("A",true);
   clocks.configure("A",std::nullopt);
   const auto disabled=clocks.state("A");
   require(!clocks.enabled("A") && !clocks.playing() && clocks.advance(1).isEmpty(),"Removed standalone override left timer active");
   same(clocks.state("A"),disabled,"Disabled standalone clock advanced");
   // A root without its own override still owns the enabled child's group clock.
   clocks.configure("Child",5,"A");
   require(clocks.enabled("A"),"Metadata-absent root disabled its enabled child");
   clocks.publish("Child",10); clocks.publish("Child",12); clocks.setRate("Child",86400); clocks.setPlaying("Child",true);
   require(clocks.advance(1)==QStringList{"A"},"Implicit group root did not advance");
   near(clocks.state("Child").epoch,11,"Implicit root clock rate conversion disagrees");
   clocks.reset("Child");
   require(!clocks.state("A").hasRange && !clocks.playing(),"Publisher reset did not clear active range and pause");
   clocks.publish("Child",50);
   require(clocks.state("A").minimum==50 && clocks.state("A").maximum==50,"Reset retained stale publisher range");
   clocks.clear();
   require(!clocks.enabled("A") && !clocks.enabled("Child") && !clocks.playing() && clocks.members("Child").isEmpty() && !clocks.state("Child").hasRange && clocks.root("Child")=="Child","Clear retained clock graph or state");
}
}

int main() {
   try {
      independentClock(); parentEnvelopeAndDetach(); childBeforeParent();
      invalidSettingsAreAtomic(); disabledRootAndReset();
      std::cout << "Simulation clock signed rates, parent envelopes, detach, placeholder and atomic validation PASS\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr << "Simulation clock FAIL: " << error.what() << '\n';
      return 1;
   }
}
