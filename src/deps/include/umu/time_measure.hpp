#pragma once

namespace umu {
class TimeMeasure {
 public:
  TimeMeasure(uint64_t& result) : save_(result) {
    QueryPerformanceCounter(&start_time_);
  }

  ~TimeMeasure() {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    save_ = time.QuadPart - start_time_.QuadPart;
  }

  static uint64_t Delta(uint64_t ts) {
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    return time.QuadPart - ts;
  }

 private:
  uint64_t& save_;
  LARGE_INTEGER start_time_;
};
}  // namespace umu