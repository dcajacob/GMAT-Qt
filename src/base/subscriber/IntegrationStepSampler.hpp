#ifndef IntegrationStepSampler_hpp
#define IntegrationStepSampler_hpp

#include "Publisher.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

// Per-runtime-trajectory cadence, independent of publication/provider counts.
// This class owns values only; endpoint pointers never escape synchronous use.
class IntegrationStepSampler
{
public:
   void Reset() { trajectories.clear(); marks.clear(); scope.clear(); }

   bool Select(const Publisher::IntegrationStep &step,
               const Publisher::CartesianSample &sample, Integer skip, bool *newArc = nullptr)
   {
      if (newArc) *newArc = false;
      if (skip < 0 || (!step.initial && step.startSeconds == step.endSeconds))
         return false;
      std::optional<Rvector6> restartState;
      const auto restart = std::find_if(step.restartStates.begin(), step.restartStates.end(),
         [&](const Publisher::CartesianSample &value) { return value.object == sample.object; });
      if (restart != step.restartStates.end()) restartState = restart->state;
      auto found = trajectories.find(sample.object);
      if (found == trajectories.end())
      {
         // A missing initial point must not silently become an extra step.
         if (!step.initial) return false;
         trajectories.emplace(sample.object, Cursor{sample.epoch, sample.state, restartState, skip, false});
         return true;
      }
      Cursor &cursor = found->second;
      // Non-precision spacecraft serialize command starts through Real MJD;
      // the preceding endpoint can retain more GmatTime bits. Compare their
      // common published precision only for initial records, and still require
      // bit-identical Cartesian state. Native restart equivalents avoid a
      // false discontinuity from changing moving-origin ephemeris clocks.
      // Accepted advances and display samples keep their full precision.
      bool same = cursor.epoch == sample.epoch ||
         (step.initial && cursor.epoch.GetMjd() == sample.epoch.GetMjd());
      const bool nativeRestart = step.initial && cursor.restartState && restartState;
      const Rvector6 &before = nativeRestart ? *cursor.restartState : cursor.state;
      const Rvector6 &after = nativeRestart ? *restartState : sample.state;
      for (Integer i = 0; i < 6 && same; ++i)
         same = before[i] == after[i];
      if (same) return false; // Repeated command-initial or duplicate endpoint.
      cursor.epoch = sample.epoch;
      cursor.state = sample.state;
      cursor.restartState = restartState;
      if (step.initial)
      {
         // A real discontinuity (including a same-epoch state assignment) starts
         // a new arc. Solver rollback restores its saved cursor explicitly.
         if (newArc) *newArc = true;
         cursor.remaining = skip;
         cursor.breakNext = false;
         return true;
      }
      if (cursor.remaining > 0) { --cursor.remaining; return false; }
      if (newArc) *newArc = cursor.breakNext;
      cursor.breakNext = false;
      cursor.remaining = skip; // No skip+1 arithmetic, even for INT_MAX.
      return true;
   }

   // A frame with an unavailable reference cannot connect to the next point.
   // This flag follows the trajectory cursor through solver rollback.
   void MarkUnavailable(const GmatBase *object)
   {
      auto it = trajectories.find(object);
      if (it != trajectories.end()) it->second.breakNext = true;
   }

   void Action(const std::string &action, const std::string &data)
   {
      if (action == "SolverScope") scope = data;
      else if (action == "MarkBreak") marks[scope].push_back(trajectories);
      else if (action == "ClearFromBreak")
      {
         auto found = marks.find(scope);
         if (found == marks.end() || found->second.empty()) return;
         auto &saved = found->second;
         int index = -1;
         try { if (!data.empty()) index = std::stoi(data); } catch (...) { return; }
         if (index < 0) index = static_cast<int>(saved.size()) - 1;
         if (static_cast<size_t>(index) < saved.size()) trajectories = saved[index];
      }
   }

private:
   struct Cursor { GmatTime epoch; Rvector6 state; std::optional<Rvector6> restartState; Integer remaining; bool breakNext; };
   using Cursors = std::map<const GmatBase *, Cursor>;
   Cursors trajectories;
   std::map<std::string, std::vector<Cursors>> marks;
   std::string scope;
};
#endif
