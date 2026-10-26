#pragma once
#include "AudioAsset.h"
#include <atomic>
#include <miniaudio.h>

// The data-source adapter consumes prepared PCM; it never decodes or opens files.
struct AudioDataSource
{
    ma_data_source_base base{};
    std::shared_ptr<const AudioAsset> asset;
    std::atomic<std::uint64_t> cursor{0};
    std::atomic<bool> loop{false};
    static const ma_data_source_vtable vtable;
    static ma_result read(ma_data_source *, void *, ma_uint64, ma_uint64 *);
    static ma_result seek(ma_data_source *, ma_uint64);
    static ma_result format(ma_data_source *, ma_format *, ma_uint32 *, ma_uint32 *, ma_channel *,
                            size_t);
    static ma_result getCursor(ma_data_source *, ma_uint64 *);
    static ma_result length(ma_data_source *, ma_uint64 *);
    static ma_result looping(ma_data_source *, ma_bool32);
};
