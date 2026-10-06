/** Case cross_file: the receivers' translation unit (see app.cpp). */
#include "../route_out_of_line.hpp"

namespace app {
void Controller::receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
void Logger::receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
}
