#include "DeFile.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

// Decode the supplied little-endian DE405 independently of DeFile. Clenshaw
// evaluation uses split epochs and long-double arithmetic, rather than the
// production forward recurrence or its position-difference implementation.
struct Reference
{
   std::vector<unsigned char> bytes;
   long double first, last, span, massRatio;
   explicit Reference(const char *path)
   {
      std::ifstream file(path, std::ios::binary);
      bytes.assign(std::istreambuf_iterator<char>(file), {});
      require(bytes.size() > 3 * 1018 * 8, "Missing DE405 reference data");
      first = number(2652) - 2430000.L;
      last = number(2660) - 2430000.L;
      span = number(2668);
      massRatio = number(2688);
      require(first == .5L && last == 95008.5L && span == 32.L &&
              massRatio > 80 && massRatio < 82, "Unexpected DE405 header");
   }
   double number(size_t offset) const
   {
      require(offset + 8 <= bytes.size(), "Truncated DE405 reference");
      uint64_t bits = 0;
      for (int i = 0; i < 8; ++i) bits |= uint64_t(bytes[offset + i]) << (8 * i);
      double value;
      std::memcpy(&value, &bits, 8);
      return value;
   }
   int integer(size_t offset) const
   {
      require(offset + 4 <= bytes.size(), "Truncated DE405 coefficient table");
      uint32_t value = 0;
      for (int i = 0; i < 4; ++i) value |= uint32_t(bytes[offset + i]) << (8 * i);
      return int(value);
   }
   std::array<long double, 3> column(int body, const GmatTime &epoch) const
   {
      const long double fraction = (epoch.GetSec() +
         static_cast<long double>(epoch.GetFracSec())) / 86400.L;
      const long double day = epoch.GetDays();
      const long double fromStart = (day - first) + fraction;
      require(fromStart >= 0 && fromStart <= last - first,
              "Reference epoch outside file");
      const size_t records = size_t((last - first) / span);
      const size_t index = std::min(size_t(fromStart / span), records - 1);
      const size_t record = (index + 2) * 1018 * 8;
      const long double start = number(record) - 2430000.L;
      const int pointer = integer(2696 + body * 12);
      const int count = integer(2700 + body * 12);
      const int granules = integer(2704 + body * 12);
      require(pointer > 0 && count > 2 && granules > 0,
              "Invalid reference coefficient table");
      const long double width = span / granules;
      const int granule = std::min(int(((day - start) + fraction) / width), granules - 1);
      // Subtract integral days before adding the fractional day, retaining
      // nanosecond-scale offsets at an interior granule/record boundary.
      const long double x = 2 * ((day - start - granule * width) + fraction) / width - 1;
      std::array<long double, 3> result{};
      for (int axis = 0; axis < 3; ++axis)
      {
         const size_t offset = record + 8 * (pointer - 1 + (3 * granule + axis) * count);
         long double b1 = 0, b2 = 0;
         for (int k = count - 1; k > 0; --k)
         {
            const long double b = 2 * x * b1 - b2 + number(offset + 8 * k);
            b2 = b1; b1 = b;
         }
         result[axis] = x * b1 - b2 + number(offset);
      }
      return result;
   }
   std::array<long double, 3> position(int body, const GmatTime &epoch) const
   {
      if (body == 2) return {};
      auto result = column(body, epoch);
      if (body != 9)
      {
         const auto barycenter = column(2, epoch), moon = column(9, epoch);
         for (int i = 0; i < 3; ++i)
            result[i] -= barycenter[i] - moon[i] / (massRatio + 1);
      }
      return result;
   }
};

GmatTime epoch(double day, double seconds = 0)
{
   GmatTime value(day);
   value.AddSeconds(seconds);
   return value;
}
struct Case { const char *name; GmatTime first, second; double bound; };
}

int main(int argc, char **argv)
{
   try
   {
      require(argc == 2, "Supply shipped DE405 file");
      Reference reference(argv[1]);
      auto *clock = TimeSystemConverter::Instance();
      // Fix the limits before repairing production. Wide intervals retain the
      // existing DE-domain test's 2e-6 km bound; short intervals require 2e-9 km.
      const std::vector<Case> cases{
         {"same-granule", epoch(32002.5), epoch(32002.5, 1), 2e-9},
         {"same-granule-100ns", epoch(32002.5), epoch(32002.5, 1e-7), 2e-9},
         {"granule-one-second", epoch(32004.5, -.5), epoch(32004.5, .5), 2e-9},
         {"granule-100ns", epoch(32004.5, -5e-8), epoch(32004.5, 5e-8), 2e-9},
         {"record-one-second", epoch(32000.5, -.5), epoch(32000.5, .5), 2e-9},
         {"record-100ns", epoch(32000.5, -5e-8), epoch(32000.5, 5e-8), 2e-9},
         {"granules-wide", epoch(32001.5), epoch(32013.5), 2e-6},
         {"records-wide", epoch(31999.5), epoch(32065.5), 2e-6},
         {"exact-record", epoch(31999.5), epoch(32000.5), 2e-6},
         {"first-to-interior", epoch(.5), epoch(64.5), 2e-6},
         {"interior-to-last", epoch(94944.5), epoch(95008.5), 2e-6}
      };
      int samples = 0, failures = 0;
      double maximumError = 0, maximumStateError = 0, maximumScalarStateError = 0;
      std::cout << std::setprecision(17);
      for (bool tt : {false, true})
      {
         const auto system = tt ? TimeSystemConverter::TTMJD : TimeSystemConverter::TDBMJD;
         DeFile reader(Gmat::DE_DE405, argv[1], Gmat::DE_BINARY);
         auto a1 = [&](const GmatTime &value) { return clock->Convert(value,
            system, TimeSystemConverter::A1MJD, 2430000.); };
         // G=1, G>1 and the Earth-centered zero route, including the lunar
         // corrections used by non-lunar GetPosDelta requests.
         for (int body : {9, 3, 10, 0, 2})
            for (const auto &test : cases)
               for (bool reverse : {false, true})
                  for (bool warmFromLater : {false, true})
                  {
                     ++samples;
                     try
                     {
                        const auto &first = reverse ? test.second : test.first;
                        const auto &second = reverse ? test.first : test.second;
                        const auto before = reference.position(body, first);
                        const auto after = reference.position(body, second);
                        DeFile stateReader(Gmat::DE_DE405, argv[1], Gmat::DE_BINARY);
                        stateReader.GetPosVel(body, a1(epoch(warmFromLater ? 90000.5 : .5)), tt);
                        const Real *state = stateReader.GetPosVel(body, a1(first), tt);
                        double stateError = 0;
                        for (int i = 0; i < 3; ++i)
                        {
                           require(std::isfinite(state[i]), "Nonfinite boundary state");
                           stateError = std::max(stateError, double(std::abs(
                              static_cast<long double>(state[i]) - before[i])));
                        }
                        maximumStateError = std::max(maximumStateError, stateError);
                        const double scalarA1 = a1(first).GetMjd();
                        const double scalarCoefficient = clock->Convert(scalarA1,
                           TimeSystemConverter::A1MJD, system, 2430000.);
                        const auto scalarExpected = reference.position(body, GmatTime(scalarCoefficient));
                        DeFile scalarReader(Gmat::DE_DE405, argv[1], Gmat::DE_BINARY);
                        scalarReader.GetPosVel(body,
                           A1Mjd(a1(epoch(warmFromLater ? 90000.5 : .5)).GetMjd()), tt);
                        const Real *scalarState = scalarReader.GetPosVel(body, A1Mjd(scalarA1), tt);
                        double scalarStateError = 0;
                        for (int i = 0; i < 3; ++i)
                        {
                           require(std::isfinite(scalarState[i]), "Nonfinite scalar boundary state");
                           scalarStateError = std::max(scalarStateError, double(std::abs(
                              static_cast<long double>(scalarState[i]) - scalarExpected[i])));
                        }
                        maximumScalarStateError = std::max(maximumScalarStateError, scalarStateError);
                        // Exercise cache replacement in both directions before
                        // every query without using production state as truth.
                        reader.GetPosVel(body, a1(epoch(warmFromLater ? 90000.5 : .5)), tt);
                        const Real *actual = reader.GetPosDelta(body, a1(first), a1(second), tt);
                        double error = 0;
                        for (int i = 0; i < 3; ++i)
                        {
                           require(std::isfinite(actual[i]), "Nonfinite position delta");
                           error = std::max(error, double(std::abs(
                              static_cast<long double>(actual[i]) - (after[i] - before[i]))));
                        }
                        maximumError = std::max(maximumError, error);
                        const bool passed = error <= test.bound && stateError <= 2e-6 && scalarStateError <= 2e-6;
                        if (!passed) ++failures;
                        std::cout << "DELTA_CHECK case=" << test.name << " body=" << body
                                  << " TT=" << tt << " reverse=" << reverse
                                  << " warm_later=" << warmFromLater << " error_km=" << error
                                  << " bound_km=" << test.bound << " state_error_km=" << stateError
                                  << " scalar_state_error_km=" << scalarStateError
                                  << " state_bound_km=2e-6 passed=" << passed << '\n';
                     }
                     catch (BaseException &e)
                     {
                        ++failures;
                        std::cout << "DELTA_EXCEPTION " << test.name << ' ' << body << ' '
                                  << e.GetFullMessage() << '\n';
                     }
                  }
      }
      std::cout << "DELTA_SUMMARY samples=" << samples << " failures=" << failures
                << " maximum_error_km=" << maximumError
                << " maximum_state_error_km=" << maximumStateError
                << " maximum_scalar_state_error_km=" << maximumScalarStateError << '\n';
      return failures ? 1 : 0;
   }
   catch (BaseException &e) { std::cerr << "FAIL " << e.GetFullMessage() << '\n'; }
   catch (std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; }
   return 1;
}
