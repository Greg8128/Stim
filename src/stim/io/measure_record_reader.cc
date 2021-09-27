/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stim/io/measure_record_reader.h"

#include <algorithm>

using namespace stim;

// Returns true if keyword is found at current position. Returns false if EOF is found at current
// position. Throws otherwise. Uses next output variable for the next character or EOF.
bool maybe_consume_keyword(FILE *in, const std::string &keyword, int &next) {
    next = getc(in);
    if (next == EOF) {
        return false;
    }

    for (char c : keyword) {
        if (c != next) {
            throw std::runtime_error("Failed to find expected string \"" + keyword + "\"");
        }
        next = getc(in);
    }

    return true;
}

// Returns true if an integer value is found at current position. Returns false otherwise.
// Uses two output variables: value to return the integer value read and next for the next
// character or EOF.
bool read_uint64(FILE *in, uint64_t &value, int &next, bool include_next = false) {
    if (!include_next) {
        next = getc(in);
    }
    if (!isdigit(next)) {
        return false;
    }

    value = 0;
    while (isdigit(next)) {
        uint64_t prev_value = value;
        value *= 10;
        value += next - '0';
        if (value < prev_value) {
            throw std::runtime_error("Integer value read from file was too big");
        }
        next = getc(in);
    }
    return true;
}

size_t MeasureRecordReader::read_records_into(simd_bit_table &out, bool major_index_is_shot_index, size_t max_shots) {
    if (!major_index_is_shot_index) {
        simd_bit_table buf(out.num_minor_bits_padded(), out.num_major_bits_padded());
        size_t r = read_records_into(buf, true, max_shots);
        buf.transpose_into(out);
        return r;
    }

    size_t rec = 0;
    max_shots = std::min(max_shots, out.num_major_bits_padded());
    while (rec < max_shots && start_record()) {
        read_bits_into_bytes({out[rec].u8, out[rec].u8 + out[rec].num_u8_padded()});
        if (!is_end_of_record()) {
            throw std::invalid_argument("Failed to read data. A shot contained more bits than expected.");
        }
        rec++;
    }
    return rec;
}

std::unique_ptr<MeasureRecordReader> MeasureRecordReader::make(
    FILE *in,
    SampleFormat input_format,
    size_t n_measurements,
    size_t n_detection_events,
    size_t n_logical_observables) {
    if (input_format != SAMPLE_FORMAT_DETS && n_detection_events != 0) {
        throw std::invalid_argument("Only DETS format support detection event records");
    }
    if (input_format != SAMPLE_FORMAT_DETS && n_logical_observables != 0) {
        throw std::invalid_argument("Only DETS format support logical observable records");
    }

    switch (input_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormat01(in, n_measurements));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatB8(in, n_measurements));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatDets(in, n_measurements, n_detection_events, n_logical_observables));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatHits(in, n_measurements));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatR8(in, n_measurements));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

size_t MeasureRecordReader::read_bits_into_bytes(PointerRange<uint8_t> out_buffer) {
    if (is_end_of_record()) {
        return 0;
    }
    char result_type = current_result_type();
    size_t n = 0;
    for (uint8_t &b : out_buffer) {
        b = 0;
        for (size_t k = 0; k < 8; k++) {
            b |= uint8_t(read_bit()) << k;
            ++n;
            if (is_end_of_record() || current_result_type() != result_type) {
                return n;
            }
        }
    }
    return n;
}

char MeasureRecordReader::current_result_type() {
    return 'M';
}

/// 01 format

MeasureRecordReaderFormat01::MeasureRecordReaderFormat01(FILE *in, size_t bits_per_record)
    : in(in), payload('\n'), position(bits_per_record), bits_per_record(bits_per_record) {
}

bool MeasureRecordReaderFormat01::read_bit() {
    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }
    if (payload == '\n' || position >= bits_per_record) {
        throw std::out_of_range("Attempt to read past end-of-record");
    }
    if (payload != '0' && payload != '1') {
        throw std::runtime_error("Expected '0' or '1' because input format was specified as '01'");
    }

    bool bit = payload == '1';
    payload = getc(in);
    ++position;
    return bit;
}

bool MeasureRecordReaderFormat01::next_record() {
    while (payload != EOF && payload != '\n') {
        payload = getc(in);
        if (position++ > bits_per_record) {
            throw std::runtime_error(
                "Line was too long for input file in 01 format. Expected " + std::to_string(bits_per_record) +
                " characters but got " + std::to_string(position));
        }
    }

    return start_record();
}

bool MeasureRecordReaderFormat01::start_record() {
    payload = getc(in);
    position = 0;
    return payload != EOF;
}

bool MeasureRecordReaderFormat01::is_end_of_record() {
    bool payload_ended = (payload == EOF || payload == '\n');
    bool expected_end = position >= bits_per_record;
    if (payload_ended && !expected_end) {
        throw std::invalid_argument("Record data (in 01 format) ended early, before expected length.");
    }
    if (!payload_ended && expected_end) {
        throw std::invalid_argument("Record data (in 01 format) did not end by the expected length.");
    }
    return payload_ended;
}

/// B8 format

MeasureRecordReaderFormatB8::MeasureRecordReaderFormatB8(FILE *in, size_t bits_per_record)
    : in(in), bits_per_record(bits_per_record), payload(0), bits_available(0), position(bits_per_record) {
}

size_t MeasureRecordReaderFormatB8::read_bits_into_bytes(PointerRange<uint8_t> out_buffer) {
    if (position >= bits_per_record) {
        return 0;
    }

    if (bits_available > 0) {
        return MeasureRecordReader::read_bits_into_bytes(out_buffer);
    }

    size_t n_bits = std::min<size_t>(8 * out_buffer.size(), bits_per_record - position);
    size_t n_bytes = (n_bits + 7) / 8;
    n_bytes = fread(out_buffer.ptr_start, sizeof(uint8_t), n_bytes, in);
    n_bits = std::min<size_t>(8 * n_bytes, n_bits);
    position += n_bits;
    return n_bits;
}

bool MeasureRecordReaderFormatB8::read_bit() {
    if (position >= bits_per_record) {
        throw std::out_of_range("Attempt to read past end-of-record");
    }

    maybe_update_payload();

    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }

    bool b = payload & 1;
    payload >>= 1;
    --bits_available;
    ++position;
    return b;
}

bool MeasureRecordReaderFormatB8::next_record() {
    while (!is_end_of_record()) {
        read_bit();
    }
    return start_record();
}

bool MeasureRecordReaderFormatB8::start_record() {
    position = 0;
    bits_available = 0;
    payload = 0;
    maybe_update_payload();
    return payload != EOF;
}

bool MeasureRecordReaderFormatB8::is_end_of_record() {
    return position >= bits_per_record;
}

void MeasureRecordReaderFormatB8::maybe_update_payload() {
    if (bits_available > 0) {
        return;
    }
    payload = getc(in);
    if (payload != EOF) {
        bits_available = 8;
    }
}

/// Hits format

MeasureRecordReaderFormatHits::MeasureRecordReaderFormatHits(FILE *in, size_t bits_per_record)
    : in(in), bits_per_record(bits_per_record), buffer(bits_per_record), position_in_buffer(bits_per_record) {
}

bool MeasureRecordReaderFormatHits::read_bit() {
    if (position_in_buffer >= bits_per_record) {
        throw std::invalid_argument("Read past end of buffer.");
    }
    return buffer[position_in_buffer++];
}

bool MeasureRecordReaderFormatHits::next_record() {
    return start_record();
}

bool MeasureRecordReaderFormatHits::start_record() {
    int c = getc(in);
    if (c == EOF) {
        return false;
    }
    buffer.clear();
    position_in_buffer = 0;
    bool is_first = true;
    while (c != '\n') {
        uint64_t value;
        if (!read_uint64(in, value, c, is_first)) {
            throw std::runtime_error(
                "Integer didn't start immediately at start of line or after comma in 'hits' format.");
        }
        if (c != ',' && c != '\n') {
            throw std::runtime_error(
                "'hits' format requires  integers to be followed by a comma or newline, but got a '" +
                std::to_string(c) + "'.");
        }
        if (value >= bits_per_record) {
            throw std::runtime_error(
                "Bits per record is " + std::to_string(bits_per_record) + " but got a hit value " +
                std::to_string(value) + ".");
        }
        buffer[value] ^= true;
        is_first = false;
    }
    return true;
}

bool MeasureRecordReaderFormatHits::is_end_of_record() {
    return position_in_buffer >= bits_per_record;
}

/// R8 format

MeasureRecordReaderFormatR8::MeasureRecordReaderFormatR8(FILE *in, size_t bits_per_record)
    : in(in), bits_per_record(bits_per_record) {
}

size_t MeasureRecordReaderFormatR8::read_bits_into_bytes(PointerRange<uint8_t> out_buffer) {
    size_t n = 0;
    for (uint8_t &b : out_buffer) {
        b = 0;
        if (buffered_0s >= 8) {
            position += 8;
            buffered_0s -= 8;
            n += 8;
            continue;
        }
        for (size_t k = 0; k < 8; k++) {
            if (!buffered_0s && !buffered_1s && !have_seen_terminal_1) {
                assert(maybe_buffer_data());
            }
            if (is_end_of_record()) {
                return n;
            }
            b |= uint8_t(read_bit()) << k;
            ++n;
        }
    }
    return n;
}

bool MeasureRecordReaderFormatR8::read_bit() {
    if (!buffered_0s && !buffered_1s) {
        assert(maybe_buffer_data());
    }
    if (buffered_0s) {
        buffered_0s--;
        position++;
        return false;
    } else if (buffered_1s) {
        buffered_1s--;
        position++;
        return true;
    } else {
        throw std::invalid_argument("Read past end-of-record.");
    }
}

bool MeasureRecordReaderFormatR8::next_record() {
    while (!is_end_of_record()) {
        read_bit();
    }
    return start_record();
}

bool MeasureRecordReaderFormatR8::start_record() {
    position = 0;
    have_seen_terminal_1 = false;
    return maybe_buffer_data();
}

bool MeasureRecordReaderFormatR8::is_end_of_record() {
    return position == bits_per_record && have_seen_terminal_1;
}

bool MeasureRecordReaderFormatR8::maybe_buffer_data() {
    assert(buffered_0s == 0);
    assert(buffered_1s == 0);
    if (is_end_of_record()) {
        throw std::invalid_argument("Attempted to read past end-of-record.");
    }

    // Count zeroes until a one is found.
    int r;
    do {
        r = getc(in);
        if (r == EOF) {
            if (buffered_0s == 0 && position == 0) {
                return false;  // No next record.
            }
            throw std::invalid_argument("r8 data ended on a continuation (a 0xFF byte) which is not allowed.");
        }
        buffered_0s += r;
    } while (r == 0xFF);
    buffered_1s = 1;

    // Check if the 1 is the one at end of data.
    size_t total_data = position + buffered_0s + buffered_1s;
    if (total_data == bits_per_record) {
        int t = getc(in);
        if (t == EOF) {
            throw std::invalid_argument(
                "r8 data ended too early. "
                "The extracted data ended in a 1, but there was no corresponding 0x00 terminator byte for the expected "
                "'fake encoded 1 just after the end of the data' before the input ended.");
        } else if (t != 0) {
            throw std::invalid_argument(
                "r8 data ended too early. "
                "The extracted data ended in a 1, but there was no corresponding 0x00 terminator byte for the expected "
                "'fake encoded 1 just after the end of the data' before any additional data.");
        }
        have_seen_terminal_1 = true;
    } else if (total_data == bits_per_record + 1) {
        have_seen_terminal_1 = true;
        buffered_1s = 0;
    } else if (total_data > bits_per_record + 1) {
        throw std::invalid_argument("r8 data encoded a jump past the expected end of encoded data.");
    }
    return true;
}

/// DETS format

MeasureRecordReaderFormatDets::MeasureRecordReaderFormatDets(
    FILE *in, size_t n_measurements, size_t n_detection_events, size_t n_logical_observables)
    : in(in),
      buffer(n_measurements + n_detection_events + n_logical_observables),
      position_in_buffer(n_measurements + n_detection_events + n_logical_observables),
      m_bits_per_record(n_measurements),
      d_bits_per_record(n_detection_events),
      l_bits_per_record(n_logical_observables) {
}

bool MeasureRecordReaderFormatDets::read_bit() {
    if (position_in_buffer >= m_bits_per_record + d_bits_per_record + l_bits_per_record) {
        throw std::invalid_argument("Read past end of buffer.");
    }
    return buffer[position_in_buffer++];
}

bool MeasureRecordReaderFormatDets::next_record() {
    return start_record();
}

bool MeasureRecordReaderFormatDets::start_record() {
    int c;
    if (!maybe_consume_keyword(in, "shot", c)) {
        return false;
    }
    buffer.clear();
    position_in_buffer = 0;
    while (true) {
        bool had_spacing = c == ' ';
        while (c == ' ') {
            c = getc(in);
        }
        if (c == '\n' || c == EOF) {
            break;
        }
        if (!had_spacing) {
            throw std::invalid_argument("DETS values must be separated by spaces.");
        }
        size_t offset;
        size_t size;
        char prefix = c;
        if (prefix == 'M') {
            offset = 0;
            size = m_bits_per_record;
        } else if (prefix == 'D') {
            offset = m_bits_per_record;
            size = d_bits_per_record;
        } else if (prefix == 'L') {
            offset = m_bits_per_record + d_bits_per_record;
            size = l_bits_per_record;
        } else {
            throw std::invalid_argument("Unrecognized DETS prefix: '" + std::to_string(c) + "'");
        }
        uint64_t number;
        if (!read_uint64(in, number, c)) {
            throw std::invalid_argument("DETS prefix '" + std::to_string(c) + "' wasn't not followed by an integer.");
        }
        if (number >= size) {
            throw std::invalid_argument(
                "Got '" + std::to_string(c) + std::to_string(number) + "' but expected num values of that type is " +
                std::to_string(size) + ".");
        }
        buffer[offset + number] ^= true;
    }
    return true;
}

bool MeasureRecordReaderFormatDets::is_end_of_record() {
    return position_in_buffer == m_bits_per_record + d_bits_per_record + l_bits_per_record;
}

char MeasureRecordReaderFormatDets::current_result_type() {
    if (position_in_buffer < m_bits_per_record && m_bits_per_record) {
        return 'M';
    }
    if (position_in_buffer < m_bits_per_record + d_bits_per_record && d_bits_per_record) {
        return 'D';
    }
    if (l_bits_per_record) {
        return 'L';
    }
    if (d_bits_per_record) {
        return 'D';
    }
    return 'M';
}