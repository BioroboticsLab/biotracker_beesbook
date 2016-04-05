#include "Utils.h"

#include <iostream>
#include <sstream>

#include <QApplication>

namespace Utils {

CursorOverrideRAII::CursorOverrideRAII(Qt::CursorShape shape) {
    QApplication::setOverrideCursor(shape);
}

CursorOverrideRAII::~CursorOverrideRAII() {
    QApplication::restoreOverrideCursor();
}

MeasureTimeRAII::MeasureTimeRAII(const std::string &what, std::function<void (const std::string &)> notify,
                                 boost::optional<size_t> num)
    : _start(std::chrono::steady_clock::now()),
      _what(what),
      _notify(notify),
      _num(num) {
}

MeasureTimeRAII::~MeasureTimeRAII() {
    const auto end = std::chrono::steady_clock::now();
    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - _start).count();
    std::stringstream message;
    message << _what << " finished in " << dur << "ms.";
    if (_num) {
        const auto avg = dur / _num.get();
        message << " (Average: " << avg << "ms)";
    }
    message << std::endl;
    _notify(message.str());
    // display message right away
    QApplication::processEvents();
}

}
