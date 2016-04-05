#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <boost/optional.hpp>
#include <QCursor>

namespace Utils {

struct CursorOverrideRAII {
    CursorOverrideRAII(Qt::CursorShape shape);
    ~CursorOverrideRAII();
};

class MeasureTimeRAII {
  public:
    MeasureTimeRAII(std::string const &what, std::function<void(std::string const &)> notify,
                    boost::optional<size_t> num = boost::optional<size_t>());
    ~MeasureTimeRAII();

  private:
    const std::chrono::steady_clock::time_point _start;
    const std::string _what;
    const std::function<void(std::string const &)> _notify;
    const boost::optional<size_t> _num;
};
}
