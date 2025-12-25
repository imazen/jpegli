// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// C FFI for jpegli internals - enables Rust parity testing

#include "lib/extras/jpegli_test_ffi.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "lib/cms/opsin_params.h"
#include "lib/extras/xyb_transform.h"

namespace {

// sRGB EOTF (gamma decoding)
float SrgbToLinear(float srgb) {
  if (srgb <= 0.04045f) {
    return srgb / 12.92f;
  } else {
    return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
  }
}

// sRGB inverse EOTF (gamma encoding)
float LinearToSrgb(float linear) {
  if (linear <= 0.0031308f) {
    return linear * 12.92f;
  } else {
    return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
  }
}

// PQ constants (ITU-R BT.2100)
constexpr float kPQM1 = 0.1593017578125f;
constexpr float kPQM2 = 78.84375f;
constexpr float kPQC1 = 0.8359375f;
constexpr float kPQC2 = 18.8515625f;
constexpr float kPQC3 = 18.6875f;

// HLG constants (ITU-R BT.2100)
constexpr float kHLGA = 0.17883277f;
constexpr float kHLGB = 0.28466892f;  // 1 - 4*a
constexpr float kHLGC = 0.55991073f;  // 0.5 - a*ln(4*a)

}  // namespace

extern "C" {

void jpegli_srgb_to_linear(
    const uint8_t* srgb,
    size_t width,
    size_t height,
    float* out_linear) {
  size_t num_pixels = width * height;
  for (size_t i = 0; i < num_pixels; ++i) {
    for (int c = 0; c < 3; ++c) {
      float x = srgb[i * 3 + c] / 255.0f;
      out_linear[i * 3 + c] = SrgbToLinear(x);
    }
  }
}

void jpegli_linear_to_xyb(
    const float* linear_rgb,
    size_t width,
    size_t height,
    float intensity_target,
    float* out_xyb) {
  // Compute premul_absorb table
  // Need to allocate enough for SIMD lanes
  constexpr size_t kMaxLanes = 16;  // AVX-512
  // Use aligned allocation for SIMD
  constexpr size_t kAlign = 64;  // AVX-512 alignment
  float* premul_absorb = static_cast<float*>(
      std::aligned_alloc(kAlign, 12 * kMaxLanes * sizeof(float)));
  std::memset(premul_absorb, 0, 12 * kMaxLanes * sizeof(float));
  jxl::ComputePremulAbsorb(intensity_target, premul_absorb);

  // SIMD functions require minimum width for vectorization
  // Round up to next multiple of 16 for AVX-512 safety
  constexpr size_t kMinWidth = 16;
  size_t padded_width = (width + kMinWidth - 1) / kMinWidth * kMinWidth;
  if (padded_width < kMinWidth) padded_width = kMinWidth;

  // Process each row with aligned memory
  float* row_r = static_cast<float*>(
      std::aligned_alloc(kAlign, padded_width * sizeof(float)));
  float* row_g = static_cast<float*>(
      std::aligned_alloc(kAlign, padded_width * sizeof(float)));
  float* row_b = static_cast<float*>(
      std::aligned_alloc(kAlign, padded_width * sizeof(float)));
  std::memset(row_r, 0, padded_width * sizeof(float));
  std::memset(row_g, 0, padded_width * sizeof(float));
  std::memset(row_b, 0, padded_width * sizeof(float));

  for (size_t y = 0; y < height; ++y) {
    // Convert interleaved to planar
    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      row_r[x] = linear_rgb[idx + 0];
      row_g[x] = linear_rgb[idx + 1];
      row_b[x] = linear_rgb[idx + 2];
    }
    // Pad remaining elements with last pixel value for consistency
    for (size_t x = width; x < padded_width; ++x) {
      row_r[x] = width > 0 ? row_r[width - 1] : 0.0f;
      row_g[x] = width > 0 ? row_g[width - 1] : 0.0f;
      row_b[x] = width > 0 ? row_b[width - 1] : 0.0f;
    }

    // Convert to XYB in place
    jxl::LinearRGBRowToXYB(row_r, row_g, row_b, premul_absorb, padded_width);

    // Convert planar back to interleaved XYB (only the actual width)
    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      out_xyb[idx + 0] = row_r[x];  // X
      out_xyb[idx + 1] = row_g[x];  // Y
      out_xyb[idx + 2] = row_b[x];  // B
    }
  }

  std::free(row_r);
  std::free(row_g);
  std::free(row_b);
  std::free(premul_absorb);
}

void jpegli_scale_xyb(
    float* xyb,
    size_t width,
    size_t height) {
  // SIMD functions require minimum width and alignment
  constexpr size_t kMinWidth = 16;
  constexpr size_t kAlign = 64;
  size_t padded_width = (width + kMinWidth - 1) / kMinWidth * kMinWidth;
  if (padded_width < kMinWidth) padded_width = kMinWidth;

  // Need planar format for ScaleXYBRow with aligned memory
  float* row_x = static_cast<float*>(
      std::aligned_alloc(kAlign, padded_width * sizeof(float)));
  float* row_y = static_cast<float*>(
      std::aligned_alloc(kAlign, padded_width * sizeof(float)));
  float* row_b = static_cast<float*>(
      std::aligned_alloc(kAlign, padded_width * sizeof(float)));
  std::memset(row_x, 0, padded_width * sizeof(float));
  std::memset(row_y, 0, padded_width * sizeof(float));
  std::memset(row_b, 0, padded_width * sizeof(float));

  for (size_t y = 0; y < height; ++y) {
    // Convert interleaved to planar
    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      row_x[x] = xyb[idx + 0];
      row_y[x] = xyb[idx + 1];
      row_b[x] = xyb[idx + 2];
    }
    // Pad remaining elements
    for (size_t x = width; x < padded_width; ++x) {
      row_x[x] = width > 0 ? row_x[width - 1] : 0.0f;
      row_y[x] = width > 0 ? row_y[width - 1] : 0.0f;
      row_b[x] = width > 0 ? row_b[width - 1] : 0.0f;
    }

    // Scale
    jxl::ScaleXYBRow(row_x, row_y, row_b, padded_width);

    // Convert back to interleaved (only actual width)
    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      xyb[idx + 0] = row_x[x];
      xyb[idx + 1] = row_y[x];
      xyb[idx + 2] = row_b[x];
    }
  }

  std::free(row_x);
  std::free(row_y);
  std::free(row_b);
}

void jpegli_srgb_to_scaled_xyb(
    const uint8_t* srgb,
    size_t width,
    size_t height,
    float intensity_target,
    float* out_scaled_xyb) {
  size_t num_pixels = width * height;

  // Step 1: sRGB to linear
  std::vector<float> linear(num_pixels * 3);
  jpegli_srgb_to_linear(srgb, width, height, linear.data());

  // Step 2: linear to XYB
  jpegli_linear_to_xyb(linear.data(), width, height, intensity_target,
                        out_scaled_xyb);

  // Step 3: scale XYB
  jpegli_scale_xyb(out_scaled_xyb, width, height);
}

void jpegli_get_xyb_constants(
    float* opsin_matrix,
    float* opsin_bias,
    float* scaled_xyb_offset,
    float* scaled_xyb_scale) {
  if (opsin_matrix) {
    // Row-major 3x3 matrix
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        opsin_matrix[i * 3 + j] = jxl::cms::kOpsinAbsorbanceMatrix[i][j];
      }
    }
  }

  if (opsin_bias) {
    for (int i = 0; i < 3; ++i) {
      opsin_bias[i] = jxl::cms::kOpsinAbsorbanceBias[i];
    }
  }

  if (scaled_xyb_offset) {
    scaled_xyb_offset[0] = jxl::cms::kScaledXYBOffset0;
    scaled_xyb_offset[1] = jxl::cms::kScaledXYBOffset1;
    scaled_xyb_offset[2] = jxl::cms::kScaledXYBOffset2;
  }

  if (scaled_xyb_scale) {
    scaled_xyb_scale[0] = jxl::cms::kScaledXYBScale0;
    scaled_xyb_scale[1] = jxl::cms::kScaledXYBScale1;
    scaled_xyb_scale[2] = jxl::cms::kScaledXYBScale2;
  }
}

// ============================================================================
// Transfer Functions
// ============================================================================

float jpegli_pq_eotf(float encoded) {
  // PQ EOTF (display from encoded)
  if (encoded <= 0.0f) return 0.0f;

  float Ym1 = std::pow(encoded, 1.0f / kPQM2);
  float num = std::max(Ym1 - kPQC1, 0.0f);
  float den = kPQC2 - kPQC3 * Ym1;
  if (den <= 0.0f) return 1.0f;

  return std::pow(num / den, 1.0f / kPQM1);
}

float jpegli_pq_inv_eotf(float display) {
  // PQ inverse EOTF (encoded from display)
  if (display <= 0.0f) return 0.0f;

  float Ym1 = std::pow(display, kPQM1);
  float num = kPQC1 + kPQC2 * Ym1;
  float den = 1.0f + kPQC3 * Ym1;

  return std::pow(num / den, kPQM2);
}

float jpegli_hlg_eotf(float encoded) {
  // HLG EOTF (display from encoded)
  if (encoded <= 0.0f) return 0.0f;

  if (encoded <= 0.5f) {
    return (encoded * encoded) / 3.0f;
  } else {
    return (std::exp((encoded - kHLGC) / kHLGA) + kHLGB) / 12.0f;
  }
}

float jpegli_hlg_inv_eotf(float display) {
  // HLG inverse EOTF (encoded from display)
  if (display <= 0.0f) return 0.0f;

  if (display <= 1.0f / 12.0f) {
    return std::sqrt(3.0f * display);
  } else {
    return kHLGA * std::log(12.0f * display - kHLGB) + kHLGC;
  }
}

float jpegli_srgb_to_linear_single(float srgb) {
  return SrgbToLinear(srgb);
}

float jpegli_linear_to_srgb_single(float linear) {
  return LinearToSrgb(linear);
}

// ============================================================================
// Tone Mapping
// ============================================================================

void jpegli_rec2408_tone_map(
    float source_nits,
    float target_nits,
    const float* luminances,
    float* rgb) {
  // Simplified Rec2408 tone mapping
  // This is a basic S-curve tone mapper
  float Lw = source_nits / 10000.0f;  // Normalize to [0, 1]
  float Ld = target_nits / 10000.0f;

  // Compute luminance
  float Y = luminances[0] * rgb[0] + luminances[1] * rgb[1] + luminances[2] * rgb[2];

  if (Y <= 0.0f || Lw <= Ld) {
    // No tone mapping needed
    return;
  }

  // Knee point and slope
  float knee = Ld * 0.75f;
  float maxY = Lw;

  // Apply tone curve
  float Ynew;
  if (Y <= knee) {
    Ynew = Y;
  } else {
    // Soft roll-off above knee
    float ratio = (Y - knee) / (maxY - knee);
    float compressed = knee + (Ld - knee) * (1.0f - std::exp(-ratio * 2.0f));
    Ynew = compressed;
  }

  // Scale RGB by luminance ratio
  if (Y > 0.0f) {
    float scale = Ynew / Y;
    rgb[0] *= scale;
    rgb[1] *= scale;
    rgb[2] *= scale;
  }
}

void jpegli_hlg_ootf(
    float source_nits,
    float target_nits,
    const float* luminances,
    float* rgb) {
  // HLG OOTF (Opto-Optical Transfer Function)
  // Maps scene light to display light
  float Ys = luminances[0] * rgb[0] + luminances[1] * rgb[1] + luminances[2] * rgb[2];

  if (Ys <= 0.0f) return;

  // HLG system gamma
  float gamma = 1.2f + 0.42f * std::log10(target_nits / 1000.0f);
  gamma = std::max(1.0f, gamma);

  // Apply OOTF
  float Yd = std::pow(Ys, gamma - 1.0f);
  float scale = Yd * (target_nits / source_nits);

  rgb[0] *= scale;
  rgb[1] *= scale;
  rgb[2] *= scale;
}

void jpegli_gamut_map(
    float preserve_saturation,
    const float* luminances,
    float* rgb) {
  // Simple gamut mapping: clip to [0, 1] while preserving hue
  float Y = luminances[0] * rgb[0] + luminances[1] * rgb[1] + luminances[2] * rgb[2];

  // Find the maximum value
  float maxVal = std::max({rgb[0], rgb[1], rgb[2]});
  float minVal = std::min({rgb[0], rgb[1], rgb[2]});

  if (maxVal <= 1.0f && minVal >= 0.0f) {
    // Already in gamut
    return;
  }

  if (maxVal <= 0.0f) {
    // All black
    rgb[0] = rgb[1] = rgb[2] = 0.0f;
    return;
  }

  // Preserve luminance while reducing saturation
  if (maxVal > 1.0f) {
    // Scale down to fit in [0, 1]
    float scale = 1.0f / maxVal;
    float blend = preserve_saturation;

    // Blend between scaled and desaturated
    for (int i = 0; i < 3; ++i) {
      float scaled = rgb[i] * scale;
      float desaturated = Y;
      rgb[i] = blend * scaled + (1.0f - blend) * desaturated;
      rgb[i] = std::min(1.0f, std::max(0.0f, rgb[i]));
    }
  }

  if (minVal < 0.0f) {
    // Clip negatives
    for (int i = 0; i < 3; ++i) {
      rgb[i] = std::max(0.0f, rgb[i]);
    }
  }
}

}  // extern "C"
