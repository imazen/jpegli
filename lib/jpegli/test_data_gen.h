// DO NOT put any code in test_data_gen.c, this is a HEADER ONLY IMPLEMENTATION
// JUST DON'T DO IT
#ifndef LIB_JPEGLI_TEST_DATA_GEN_H_
#define LIB_JPEGLI_TEST_DATA_GEN_H_

// Default definition: enable instrumentation code compilation.
// Can be overridden by build system later (e.g., -DENABLE_RUST_TEST_INSTRUMENTATION=0).
#ifndef ENABLE_RUST_TEST_INSTRUMENTATION
#define ENABLE_RUST_TEST_INSTRUMENTATION 1
#endif

#if ENABLE_RUST_TEST_INSTRUMENTATION

#include <atomic>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <cstdio> // For fprintf

// #include "lib/base/common.h" // Removed include
#include "lib/jpegli/common.h" // For j_compress_ptr, JQUANT_TBL etc.
#include "lib/jpegli/types.h"
#include "lib/jpegli/common_internal.h" // Added for RowBuffer
#include "lib/jpegli/encode_internal.h" // Added for Token

namespace jpegli {

// Thread-safe mutex getter (inline with static ensures single instance)
inline std::mutex& get_testdata_mutex() {
    static std::mutex mtx;
    return mtx;
}

// Runtime check function
inline bool IsRustTestDataEnabled() {
    static std::atomic<int> checked = {-1}; // -1: unchecked, 0: false, 1: true
    int current_value = checked.load(std::memory_order_relaxed);
    if (current_value == -1) {
        const char* env_p = std::getenv("GENERATE_RUST_TEST_DATA");
        bool enabled = (env_p != nullptr && std::string(env_p) == "1");
        current_value = enabled ? 1 : 0;
        int expected = -1;
        checked.compare_exchange_strong(expected, current_value,
                                      std::memory_order_relaxed);
        return enabled;
    }
    return current_value == 1;
}

// JSON formatting helpers (defined inline in header)
inline std::string format_json_string(const std::string& s) {
    std::stringstream ss;
    ss << '"';
    for (char c : s) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    ss << c;
                }
        }
    }
    ss << '"';
    return ss.str();
}

inline std::string format_json_bool(bool b) {
    return b ? "true" : "false";
}

inline std::string format_json_float(float f) {
    std::stringstream ss;
    ss << std::scientific << std::setprecision(9) << f;
    return ss.str();
}

inline std::string format_json_quant_tables(const JQUANT_TBL* const tables[], size_t num_tables) {
    std::stringstream ss;
    ss << "[";
    bool first_table = true;
    for (size_t i = 0; i < num_tables; ++i) {
        if (!first_table) ss << ",";
        first_table = false;
        if (tables[i] == nullptr) {
            ss << "null";
        } else {
            ss << "[";
            bool first_val = true;
            for (size_t k = 0; k < DCTSIZE2; ++k) {
                if (!first_val) ss << ",";
                first_val = false;
                ss << tables[i]->quantval[k];
            }
            ss << "]";
        }
    }
    ss << "]";
    return ss.str();
}

inline std::string format_json_component_info_vector(const jpeg_component_info* components,
                                                     int num_components)
{
    std::stringstream ss;
    ss << "[";
    bool first_comp = true;
    for (int i = 0; i < num_components; ++i) {
        if (!first_comp) ss << ",";
        first_comp = false;
        const jpeg_component_info& comp = components[i];
        ss << "{";
        ss << "\"component_index\": " << comp.component_index << ",";
        ss << "\"h_samp_factor\": " << comp.h_samp_factor << ",";
        ss << "\"v_samp_factor\": " << comp.v_samp_factor << ",";
        ss << "\"quant_tbl_no\": " << comp.quant_tbl_no << ",";
        ss << "\"width_in_blocks\": " << comp.width_in_blocks << ",";
        ss << "\"height_in_blocks\": " << comp.height_in_blocks;
        ss << "}";
    }
    ss << "]";
    return ss.str();
}

// Add declarations for other formatters here...
std::string format_json_2d_float_vec(const std::vector<std::vector<float>>& vec);

// Implementation for format_json_2d_float_vec
inline std::string format_json_2d_float_vec(const std::vector<std::vector<float>>& vec) {
    std::stringstream ss;
    ss << "[";
    bool first_row = true;
    for (const auto& row : vec) {
        if (!first_row) ss << ",";
        first_row = false;
        ss << "[";
        bool first_col = true;
        for (float val : row) {
            if (!first_col) ss << ",";
            first_col = false;
            ss << format_json_float(val);
        }
        ss << "]";
    }
    ss << "]";
    return ss.str();
}

// Helper Struct (mirror Rust struct)
struct RustRowBufferSliceF32_Data {
    size_t component_index = 0;
    ssize_t start_row = 0;
    size_t num_rows = 0;
    ssize_t start_col = 0;
    size_t num_cols = 0;
    size_t stride = 0;
    std::vector<std::vector<float>> data;
};

// Helper to populate the data struct from RowBuffer
template <typename T = float>
inline RustRowBufferSliceF32_Data CaptureRowBufferSlice(
    const RowBuffer<T>& buf,
    size_t component_idx,
    ssize_t y0, size_t ylen,
    ssize_t x0 = -1, // Default captures border
    size_t xlen_override = 0) // Default uses buf.xsize()
{
    RustRowBufferSliceF32_Data slice;
    slice.component_index = component_idx;
    slice.start_row = y0;
    slice.num_rows = ylen;
    slice.start_col = x0;
    slice.stride = buf.stride();

    size_t buffer_xsize = buf.xsize(); // Original allocated width
    size_t capture_xlen = (xlen_override > 0) ? xlen_override : buffer_xsize;
    // Adjust num_cols based on start_col and capture_xlen, usually including borders
    slice.num_cols = capture_xlen + (x0 < 0 ? -x0 : 0) + (x0 + capture_xlen > buffer_xsize ? (x0 + capture_xlen - buffer_xsize) : 0);
    // If x0=-1, num_cols = capture_xlen + 1 + potential right border
    // Correct calculation if border is assumed 1:
    if (x0 == -1) { 
        slice.num_cols = capture_xlen + 2; // Capture -1 and 0..xlen-1 and xlen 
    }
    // TODO: Refine num_cols calculation if borders other than 1 are needed


    slice.data.resize(ylen);
    for (size_t i = 0; i < ylen; ++i) {
        ssize_t current_y = y0 + i;
        const T* row_ptr = buf.Row(current_y);
        slice.data[i].assign(row_ptr + x0, row_ptr + x0 + slice.num_cols);
    }
    return slice;
}

// Helper to format the slice struct
inline std::string format_json_rowbufferslice(const RustRowBufferSliceF32_Data& slice) {
    std::stringstream ss;
    ss << "{";
    ss << "\"component_index\": " << slice.component_index << ",";
    ss << "\"start_row\": " << slice.start_row << ",";
    ss << "\"num_rows\": " << slice.num_rows << ",";
    ss << "\"start_col\": " << slice.start_col << ",";
    ss << "\"num_cols\": " << slice.num_cols << ",";
    ss << "\"stride\": " << slice.stride << ",";
    ss << "\"data\": [";
    bool first_row = true;
    for(const auto& row : slice.data) {
        if (!first_row) ss << ",";
        first_row = false;
        ss << "[";
        bool first_col = true;
        for(const auto& val : row) {
            if (!first_col) ss << ",";
            first_col = false;
            ss << format_json_float(val); // Assumes T=float
        }
        ss << "]";
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

// Helper for 1D float block (64)
inline std::string format_json_blockf32(const float* block) {
    std::stringstream ss;
    ss << "{\"data\": [";
    for (size_t k = 0; k < DCTSIZE2; ++k) {
        if (k > 0) ss << ",";
        ss << format_json_float(block[k]);
    }
    ss << "]}";
    return ss.str();
}

// Helper for 1D int32 block (64)
inline std::string format_json_blocki32(const int32_t* block) {
    std::stringstream ss;
    ss << "{\"data\": [";
    for (size_t k = 0; k < DCTSIZE2; ++k) {
        if (k > 0) ss << ",";
        ss << block[k];
    }
    ss << "]}";
    return ss.str();
}

// Helper for Vec<Token>
inline std::string format_json_token_vector(const std::vector<Token>& tokens) {
     std::stringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& token : tokens) {
        if (!first) ss << ",";
        first = false;
        ss << "{";
        ss << "\"context\": " << token.context << ",";
        ss << "\"symbol\": " << token.symbol << ",";
        ss << "\"bits\": " << token.bits;
        ss << "}";
    }
    ss << "]";
    return ss.str();
}

// Synchronized writing function (defined inline in header)
inline void WriteTestDataJsonLine(const std::string& filename_base,
                                  const std::stringstream& ss) {
    if (!IsRustTestDataEnabled()) {
        return;
    }
    // Use the mutex getter function
    std::lock_guard<std::mutex> lock(get_testdata_mutex());
    std::string filename = filename_base + ".testdata";
    std::ofstream outfile(filename, std::ios::app);
    if (outfile.is_open()) {
        outfile << ss.str() << "," << std::endl;
    } else {
        fprintf(stderr, "Error opening testdata file %s\n", filename.c_str());
    }
}

} // namespace jpegli

#endif // ENABLE_RUST_TEST_INSTRUMENTATION

#endif // LIB_JPEGLI_TEST_DATA_GEN_H_ 