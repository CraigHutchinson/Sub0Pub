#include "receivers.hpp"

namespace app {
void Controller::receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
void Logger::receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
}
