#include "DeFile.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
int checks = 0;
void require(bool condition, const char *message)
{
   ++checks;
   if (!condition) throw std::runtime_error(message);
}
// Independent little-endian DE405 decoding and long-double Clenshaw
// evaluation. Production uses a forward Chebyshev recurrence.
struct Reference
{
   std::vector<unsigned char> bytes;
   long double first, last, span;
   explicit Reference(const char *path)
   {
      std::ifstream file(path, std::ios::binary);
      bytes.assign(std::istreambuf_iterator<char>(file), {});
      require(bytes.size() > 3 * 1018 * 8, "Missing DE405 reference data");
      first = number(2652) - 2430000.L;
      last = number(2660) - 2430000.L;
      span = number(2668);
      require(first == .5L && last == 95008.5L && span == 32.L,
              "Unexpected shipped DE405 domain");
   }
   double number(size_t offset) const
   {
      if (offset + 8 > bytes.size()) throw std::runtime_error("Truncated DE405 reference");
      uint64_t bits = 0;
      for (int i = 0; i < 8; ++i) bits |= uint64_t(bytes[offset + i]) << (8 * i);
      double value;
      std::memcpy(&value, &bits, 8);
      return value;
   }
   std::array<long double, 6> evaluate(long double epoch, bool angles) const
   {
      require(epoch >= first && epoch <= last, "Reference epoch outside file");
      const size_t records = size_t((last - first) / span);
      const size_t index = std::min(size_t((epoch - first) / span), records - 1);
      const size_t record = (index + 2) * 1018 * 8;
      const long double start = number(record) - 2430000.L;
      const int count = angles ? 10 : 13, granules = angles ? 4 : 8;
      const int pointer = angles ? 899 : 441;
      const long double width = span / granules;
      const int granule = std::min(int((epoch - start) / width), granules - 1);
      const long double x = 2 * (epoch - start - granule * width) / width - 1;
      std::array<long double, 6> result{};
      for (int axis = 0; axis < 3; ++axis)
      {
         const size_t offset = record + 8 * (pointer - 1 + (3 * granule + axis) * count);
         long double b1 = 0, b2 = 0, d1 = 0, d2 = 0;
         for (int k = count - 1; k > 0; --k)
         {
            const long double b = 2 * x * b1 - b2 + number(offset + 8 * k);
            const long double d = 2 * b1 + 2 * x * d1 - d2;
            b2 = b1; b1 = b; d2 = d1; d1 = d;
         }
         result[axis] = x * b1 - b2 + number(offset);
         result[axis + 3] = (b1 + x * d1 - d2) * 2 / (width * 86400.L);
      }
      return result;
   }
};
template<class Action> void rejects(Action action)
{
   bool rejected = false;
   try { action(); } catch (BaseException &) { rejected = true; }
   require(rejected, "Unsupported DE epoch was accepted");
}
}

int main(int argc, char **argv)
{
   try
   {
      require(argc == 2, "Supply shipped DE405 file");
      Reference reference(argv[1]);
      auto *clock = TimeSystemConverter::Instance();
      // MathSpec TimeSystems.tex: independent TT-based two-sine model.
      // Split-epoch subtraction exposes errors hidden by a large MJD double.
      double maximumClockError = 0, maximumPositionError = 0;
      for (double tt : {.5, 10000.25, 21545., 30000.75, 50000.25, 95008.5})
      {
         const long double mean = (357.5277233L + 35999.05034L *
            (static_cast<long double>(tt) - 21545.L) / 36525.L) * std::acos(-1.L) / 180.L;
         const long double offset = .001658L * std::sin(mean) + .00001385L * std::sin(2 * mean);
         const GmatTime input(tt);
         const GmatTime tdb = clock->Convert(input, TimeSystemConverter::TTMJD,
            TimeSystemConverter::TDBMJD, 2430000.);
         const double error = std::abs((tdb - input).GetMjd() * 86400. - double(offset));
         maximumClockError = std::max(maximumClockError, error);
         require(error < 2e-11, "TDB conversion disagrees with TT-based MathSpec");
         const GmatTime back = clock->Convert(tdb, TimeSystemConverter::TDBMJD,
            TimeSystemConverter::TTMJD, 2430000.);
         require(std::abs((back - input).GetMjd() * 86400.) < 2e-11,
                 "Precise TT/TDB inverse is inconsistent");
         const double scalar = clock->Convert(tt, TimeSystemConverter::TTMJD,
            TimeSystemConverter::TDBMJD, 2430000.);
         const double ulpSeconds = (std::nextafter(tt, std::numeric_limits<double>::infinity()) - tt) * 86400.;
         require(std::abs((scalar - tt) * 86400. - double(offset)) < ulpSeconds + 2e-11,
                 "Scalar TDB conversion disagrees with TT-based MathSpec");
         const double scalarBack = clock->Convert(scalar, TimeSystemConverter::TDBMJD,
            TimeSystemConverter::TTMJD, 2430000.);
         require(std::abs(scalarBack - tt) * 86400. <= ulpSeconds,
                 "Scalar TT/TDB inverse is inconsistent");
      }
      const std::vector<double> epochs{95008.5, .5, .5 + 1./86400., .5001,
         .5 + 60./86400., 21545., 95008.5 - 60./86400., 95008.5 - 1./86400., .5};
      for (bool overrideTime : {false, true})
      {
         const auto system = overrideTime ? TimeSystemConverter::TTMJD : TimeSystemConverter::TDBMJD;
         DeFile reader(Gmat::DE_DE405, argv[1], Gmat::DE_BINARY);
         auto a1 = [&](double epoch) { return clock->Convert(GmatTime(epoch), system,
            TimeSystemConverter::A1MJD, 2430000.); };
         for (bool precise : {false, true})
            for (double epoch : epochs)
            {
               const auto expected = reference.evaluate(epoch, false);
               const auto expectedAngles = reference.evaluate(epoch, true);
               const GmatTime input = a1(epoch);
               const double scalar = clock->Convert(epoch, system, TimeSystemConverter::A1MJD, 2430000.);
               std::cout << "query epoch=" << epoch << " precise=" << precise
                         << " TT_override=" << overrideTime << '\n';
               const Real *state = precise ? reader.GetPosVel(DeFile::MOON_ID, input, overrideTime) :
                  reader.GetPosVel(DeFile::MOON_ID, A1Mjd(scalar), overrideTime);
               for (int i = 0; i < 6; ++i)
               {
                  const double error = std::abs(state[i] - double(expected[i]));
                  if (i < 3) maximumPositionError = std::max(maximumPositionError, error);
                  // At the last epoch scalar MJD has a 1.26us ULP.
                  require(std::isfinite(state[i]) && error < (i < 3 ? 2e-6 : 2e-11),
                          "DE state differs from independent binary coefficients");
               }
               Real angles[3], rates[3];
               if (precise) reader.GetAnglesAndRates(input, angles, rates, overrideTime);
               else reader.GetAnglesAndRates(A1Mjd(scalar), angles, rates, overrideTime);
               for (int i = 0; i < 3; ++i)
               {
                  require(std::isfinite(angles[i]) && std::abs(angles[i] - double(expectedAngles[i])) < 5e-11,
                          "DE libration differs from independent binary coefficients");
                  require(std::isfinite(rates[i]) && std::abs(rates[i] - double(expectedAngles[i+3])) < 2e-17,
                          "DE libration rate differs from independent binary coefficients");
               }
            }
         for (double epoch : {.5 - 1./86400., 95008.5 + 1./86400.})
         {
            const GmatTime bad = a1(epoch), good = a1(21545.);
            for (int body : {DeFile::MOON_ID, DeFile::EARTH_ID})
            {
               rejects([&] { reader.GetPosVel(body, bad, overrideTime); });
               rejects([&] { reader.GetPosVel(body, A1Mjd(bad.GetMjd()), overrideTime); });
               rejects([&] { reader.GetPosDelta(body, good, bad, overrideTime); });
               rejects([&] { reader.GetPosDelta(body, bad, good, overrideTime); });
            }
            Real angles[3], rates[3];
            rejects([&] { reader.GetAnglesAndRates(bad, angles, rates, overrideTime); });
            rejects([&] { reader.GetAnglesAndRates(A1Mjd(bad.GetMjd()), angles, rates, overrideTime); });
            require(std::isfinite(reader.GetPosVel(DeFile::MOON_ID, good, overrideTime)[0]),
                    "DE reader did not recover after invalid request");
         }
         for (double start : {.5, 95008.5 - 1./86400.})
         {
            const double end = start + 1./86400.;
            const auto before = reference.evaluate(start, false), after = reference.evaluate(end, false);
            for (bool reverse : {false, true})
            {
               const Real *delta = reader.GetPosDelta(DeFile::MOON_ID,
                  a1(reverse ? end : start), a1(reverse ? start : end), overrideTime);
               for (int i = 0; i < 3; ++i)
                  require(std::isfinite(delta[i]) && std::abs(delta[i] -
                     double((reverse ? -1 : 1) * (after[i] - before[i]))) < 2e-6,
                     "DE one-second position delta differs from independent coefficients");
            }
         }
         for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
         {
            Real angles[3], rates[3];
            rejects([&] { reader.GetPosVel(DeFile::MOON_ID, A1Mjd(bad), overrideTime); });
            rejects([&] { reader.GetAnglesAndRates(A1Mjd(bad), angles, rates, overrideTime); });
         }
      }
      std::cout.precision(17);
      std::cout << "PASS DE domain/state/libration/recovery, local deltas and MathSpec clock; checks="
                << checks << "; max_position_error_km=" << maximumPositionError
                << "; max_clock_error_seconds=" << maximumClockError << '\n';
      return 0;
   }
   catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
   catch (std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
