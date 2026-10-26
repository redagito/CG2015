#include "AudioDataSource.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

ma_result AudioDataSource::read(ma_data_source *raw, void *output, ma_uint64 count, ma_uint64 *got)
{
    auto &s = *static_cast<AudioDataSource *>(raw);
    auto *dst = static_cast<float *>(output);
    if (s.asset->streaming())
    {
        *got = s.asset->readStream(dst, count);
        s.cursor += *got;
        return MA_SUCCESS;
    }
    auto cursor = s.cursor.load();
    const auto length = s.asset->frames();
    const auto channels = s.asset->channels();
    ma_uint64 total = 0;
    while (total < count)
    {
        if (cursor == length)
        {
            if (!s.loop.load())
                break;
            cursor = 0;
        }
        const auto n = std::min<ma_uint64>(count - total, length - cursor);
        if (dst)
            std::memcpy(dst + total * channels, s.asset->samples().data() + cursor * channels,
                        n * channels * sizeof(float));
        total += n;
        cursor += n;
    }
    s.cursor = cursor;
    *got = total;
    return total < count ? MA_AT_END : MA_SUCCESS;
}

ma_result AudioDataSource::seek(ma_data_source *raw, ma_uint64 cursor)
{
    auto &s = *static_cast<AudioDataSource *>(raw);
    if (s.asset->streaming())
        return MA_NOT_IMPLEMENTED;
    if (cursor > s.asset->frames())
        return MA_INVALID_ARGS;
    s.cursor = cursor;
    return MA_SUCCESS;
}

ma_result AudioDataSource::format(ma_data_source *raw, ma_format *f, ma_uint32 *channels,
                                  ma_uint32 *rate, ma_channel *map, size_t capacity)
{
    auto &s = *static_cast<AudioDataSource *>(raw);
    if (f)
        *f = ma_format_f32;
    if (channels)
        *channels = s.asset->channels();
    if (rate)
        *rate = 48000;
    if (map)
        ma_channel_map_init_standard(ma_standard_channel_map_default, map, capacity,
                                     s.asset->channels());
    return MA_SUCCESS;
}

ma_result AudioDataSource::getCursor(ma_data_source *raw, ma_uint64 *value)
{
    *value = static_cast<AudioDataSource *>(raw)->cursor.load();
    return MA_SUCCESS;
}

ma_result AudioDataSource::length(ma_data_source *raw, ma_uint64 *value)
{
    *value = static_cast<AudioDataSource *>(raw)->asset->frames();
    return MA_SUCCESS;
}

ma_result AudioDataSource::looping(ma_data_source *raw, ma_bool32 value)
{
    static_cast<AudioDataSource *>(raw)->loop = value != 0;
    return MA_SUCCESS;
}

const ma_data_source_vtable AudioDataSource::vtable = {
    read,
    seek,
    format,
    getCursor,
    length,
    looping,
    MA_DATA_SOURCE_SELF_MANAGED_RANGE_AND_LOOP_POINT};
