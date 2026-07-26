#include "epoch/render/techniques/gpu_profiler.hpp"

namespace epoch::render::techniques {

GpuProfiler::GpuProfiler() { gl::GenQueries(static_cast<GLsizei>(queries_.size()), queries_.data()); }
GpuProfiler::~GpuProfiler() { gl::DeleteQueries(static_cast<GLsizei>(queries_.size()), queries_.data()); }

void GpuProfiler::begin(bool enabled) noexcept {
    active_ = enabled;
    if (!active_) return;
    gl::BeginQuery(gl::time_elapsed, queries_[write_index_]);
}

void GpuProfiler::end() noexcept {
    if (!active_) return;
    gl::EndQuery(gl::time_elapsed);
    const std::size_t read_index = (write_index_ + 1u) % queries_.size();
    if (issued_previous_) {
        GLint available{};
        gl::GetQueryObjectiv(queries_[read_index], gl::query_result_available, &available);
        if (available) {
            unsigned long long nanoseconds{};
            gl::GetQueryObjectui64v(queries_[read_index], gl::query_result, &nanoseconds);
            latest_milliseconds_ = static_cast<float>(static_cast<double>(nanoseconds) / 1'000'000.0);
        }
    }
    issued_previous_ = true;
    write_index_ = read_index;
    active_ = false;
}

} // namespace epoch::render::techniques
