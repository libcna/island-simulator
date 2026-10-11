#pragma once

#include <array>
#include <mutex>
#include <ostream>
#include <streambuf>
#include <string_view>

namespace island
{
    // Install before starting worker threads and remove after joining them. The sink must
    // not write to the mirrored stream. Preserve stderr while bounding platform-log traffic.
    class DiagnosticStreamMirror final : public std::streambuf
    {
    public:
        using Sink = void (*)(std::string_view) noexcept;
        static constexpr std::size_t ChunkSize = 2048;
        static constexpr std::size_t MaximumChunks = 128;

        DiagnosticStreamMirror(std::ostream& stream, Sink sink)
            : stream_(stream), original_(stream.rdbuf()), sink_(sink)
        {
            stream_.rdbuf(this);
        }

        ~DiagnosticStreamMirror() override
        {
            // All application workers must already have stopped here.
            Emit();
            stream_.rdbuf(original_);
        }

        DiagnosticStreamMirror(const DiagnosticStreamMirror&) = delete;
        DiagnosticStreamMirror& operator=(const DiagnosticStreamMirror&) = delete;

    protected:
        std::streamsize xsputn(const char* data, std::streamsize count) override
        {
            std::scoped_lock lock(mutex_);
            const auto written = original_->sputn(data, count);
            for (std::streamsize i = 0; i < written; ++i) Append(data[i]);
            return written;
        }

        int_type overflow(int_type value) override
        {
            if (traits_type::eq_int_type(value, traits_type::eof())) return traits_type::not_eof(value);
            std::scoped_lock lock(mutex_);
            const char character = traits_type::to_char_type(value);
            const auto result = original_->sputc(character);
            if (!traits_type::eq_int_type(result, traits_type::eof())) Append(character);
            return result;
        }

        int sync() override
        {
            std::scoped_lock lock(mutex_);
            // cerr's unitbuf flushes after each insertion; do not split compiler messages.
            return original_->pubsync();
        }

    private:
        void Append(char character) noexcept
        {
            if (chunks_ >= MaximumChunks) return;
            if (character == '\n') {
                Emit();
            } else {
                buffer_[size_++] = character;
                if (size_ == buffer_.size()) Emit();
            }
        }

        void Emit() noexcept
        {
            if (size_ == 0) return;
            sink_(std::string_view(buffer_.data(), size_));
            size_ = 0;
            if (++chunks_ == MaximumChunks)
                sink_("island-simulator: stderr platform mirror limit reached; original stderr continues");
        }

        std::ostream& stream_;
        std::streambuf* original_;
        Sink sink_;
        std::mutex mutex_;
        std::array<char, ChunkSize> buffer_{};
        std::size_t size_ = 0;
        std::size_t chunks_ = 0;
    };
}
