#include "RunSmoother.hpp"
#include "ExtendedKalmanFilter.hpp"
#include "Smoother.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool value, const char *message)
{
   if (!value) throw std::runtime_error(message);
}

class FilterProbe : public ExtendedKalmanFilter
{
public:
   explicit FilterProbe(const std::string &name) : ExtendedKalmanFilter(name) {}
   int reads = 0;
   std::vector<UpdateInfoType> GetUpdateStats() override
   {
      ++reads;
      return {};
   }
   void GetObjectBuffer(ObjectArray &buffer) override { buffer.clear(); }
};

class EstimatorProbe : public RunEstimator
{
public:
   EstimatorProbe(const std::string &name, std::uint64_t sequence)
   {
      theEstimator = new FilterProbe(name);
      successfulExecutionSequence = sequence;
   }
   FilterProbe *Filter() { return static_cast<FilterProbe*>(theEstimator); }
   void CompleteAt(std::uint64_t sequence) { successfulExecutionSequence = sequence; }
};

class SmootherProbe : public Smoother
{
public:
   SmootherProbe() : Smoother("FPS")
   {
      filterName = "EKF";
      filter = new ExtendedKalmanFilter(filterName);
   }
};

class SmootherCommandProbe : public RunSmoother
{
public:
   SmootherCommandProbe() { theEstimator = new SmootherProbe; }
};

class BranchProbe : public GmatCommand
{
public:
   BranchProbe() : GmatCommand("HistoryBranchProbe") {}
   std::vector<GmatCommand*> children;
   bool Execute() override { return true; }
   bool HasLocalClones() override { return false; }
   bool RenameRefObject(UnsignedInt, const std::string &, const std::string &) override { return false; }
   GmatBase *Clone() const override { return new BranchProbe; }
   GmatCommand *GetChildCommand(Integer index = 0) override
   {
      return index >= 0 && index < static_cast<Integer>(children.size()) ? children[index] : nullptr;
   }
};
}

int main()
{
   try
   {
      BranchProbe root, branch;
      EstimatorProbe early("EKF", 20), nested("EKF", 10), wrong("Other", 30), future("EKF", 0);
      SmootherCommandProbe smoother;
      std::vector<GmatCommand*> all{&root, &early, &branch, &nested, &wrong, &smoother, &future};
      // GmatCommand owns its next pointer. Detach the test graph even on failure.
      struct Detach
      {
         std::vector<GmatCommand*> &nodes;
         ~Detach() { for (auto *node : nodes) node->ForceSetNext(nullptr); }
      } detach{all};
      auto link = [](GmatCommand &a, GmatCommand &b)
      {
         a.ForceSetNext(&b);
         b.ForceSetPrevious(&a);
      };
      link(root, early); link(early, branch); link(branch, wrong);
      link(wrong, smoother); link(smoother, future);
      branch.children = {&nested};
      nested.ForceSetPrevious(&branch);
      nested.ForceSetNext(&branch); // Real branch-end graphs return to their owner.

      require(smoother.Initialize(), "Lookup initialization failed");
      require(early.Filter()->reads == 1 && nested.Filter()->reads == 0 &&
              wrong.Filter()->reads == 0 && future.Filter()->reads == 0,
              "Latest completion must win over source order, wrong name and future command");
      early.CompleteAt(5);
      require(smoother.Initialize(), "Nested lookup failed");
      require(early.Filter()->reads == 1 && nested.Filter()->reads == 1,
              "Latest nested completion was missed");
      early.CompleteAt(0); nested.CompleteAt(0);
      require(smoother.Initialize(), "Empty history lookup failed");
      require(early.Filter()->reads == 1 && nested.Filter()->reads == 1 &&
              wrong.Filter()->reads == 0 && future.Filter()->reads == 0,
              "Skipped or future filter was selected");

      early.CompleteAt(40);
      RunEstimator copied(early);
      copied.ForceSetNext(nullptr);
      require(copied.GetSuccessfulExecutionSequence() == 0, "Copy inherited execution history");
      RunEstimator assigned;
      assigned = early;
      assigned.ForceSetNext(nullptr);
      require(assigned.GetSuccessfulExecutionSequence() == 0, "Assignment inherited execution history");
      early.RunComplete();
      require(early.GetSuccessfulExecutionSequence() == 0, "Mission cleanup retained execution history");
      std::cout << "NASA_HISTORY_SELECTION passed: chronological, nested, cyclic, named, skipped, future, copy, assignment, cleanup\n";
      return 0;
   }
   catch (const BaseException &error) { std::cerr << error.GetFullMessage() << '\n'; }
   catch (const std::exception &error) { std::cerr << error.what() << '\n'; }
   return 1;
}
