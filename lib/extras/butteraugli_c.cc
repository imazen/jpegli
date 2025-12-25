// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// C wrapper for butteraugli - enables FFI bindings for Rust testing

#include "lib/extras/butteraugli_c.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "lib/base/status.h"
#include "lib/extras/butteraugli.h"
#include "lib/extras/image.h"

namespace {

// Default memory manager using malloc/free
void* DefaultAlloc(void* opaque, size_t size) {
  return std::malloc(size);
}

void DefaultFree(void* opaque, void* address) {
  std::free(address);
}

// Static default memory manager
JxlMemoryManager g_default_memory_manager = {
    nullptr,      // opaque
    DefaultAlloc,
    DefaultFree
};

}  // namespace

extern "C" {

butteraugli_error_t butteraugli_compare(
    const float* rgb0,
    const float* rgb1,
    size_t width,
    size_t height,
    float intensity_target,
    double* out_score) {
  return butteraugli_compare_full(
      rgb0, rgb1, width, height,
      1.0f,  // hf_asymmetry
      1.0f,  // xmul
      intensity_target,
      out_score,
      nullptr);  // no diffmap
}

butteraugli_error_t butteraugli_compare_full(
    const float* rgb0,
    const float* rgb1,
    size_t width,
    size_t height,
    float hf_asymmetry,
    float xmul,
    float intensity_target,
    double* out_score,
    float* out_diffmap) {
  if (!rgb0 || !rgb1 || !out_score || width == 0 || height == 0) {
    return BUTTERAUGLI_ERROR_INVALID_INPUT;
  }

  // Create Image3F objects using the default memory manager
  auto result0 = jxl::Image3F::Create(&g_default_memory_manager, width, height);
  auto result1 = jxl::Image3F::Create(&g_default_memory_manager, width, height);

  if (!result0.ok() || !result1.ok()) {
    return BUTTERAUGLI_ERROR_MEMORY;
  }

  jxl::Image3F img0 = std::move(result0).value_();
  jxl::Image3F img1 = std::move(result1).value_();

  // Copy interleaved RGB to planar Image3F
  for (size_t y = 0; y < height; ++y) {
    float* row0_r = img0.PlaneRow(0, y);
    float* row0_g = img0.PlaneRow(1, y);
    float* row0_b = img0.PlaneRow(2, y);
    float* row1_r = img1.PlaneRow(0, y);
    float* row1_g = img1.PlaneRow(1, y);
    float* row1_b = img1.PlaneRow(2, y);

    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      row0_r[x] = rgb0[idx + 0];
      row0_g[x] = rgb0[idx + 1];
      row0_b[x] = rgb0[idx + 2];
      row1_r[x] = rgb1[idx + 0];
      row1_g[x] = rgb1[idx + 1];
      row1_b[x] = rgb1[idx + 2];
    }
  }

  // Set up parameters
  jxl::ButteraugliParams params;
  params.hf_asymmetry = hf_asymmetry;
  params.xmul = xmul;
  params.intensity_target = intensity_target;

  // Compute butteraugli
  jxl::ImageF diffmap;
  jxl::Status status = jxl::ButteraugliDiffmap(img0, img1, params, diffmap);

  if (!status) {
    return BUTTERAUGLI_ERROR_INTERNAL;
  }

  // Compute score (max of diffmap)
  *out_score = jxl::ButteraugliScoreFromDiffmap(diffmap, &params);

  // Copy diffmap if requested
  if (out_diffmap) {
    for (size_t y = 0; y < height; ++y) {
      const float* row = diffmap.ConstRow(y);
      std::memcpy(out_diffmap + y * width, row, width * sizeof(float));
    }
  }

  return BUTTERAUGLI_OK;
}

void butteraugli_srgb_to_linear(
    const uint8_t* srgb,
    size_t width,
    size_t height,
    float* out_linear) {
  size_t num_pixels = width * height;
  for (size_t i = 0; i < num_pixels; ++i) {
    for (int c = 0; c < 3; ++c) {
      float x = srgb[i * 3 + c] / 255.0f;
      if (x <= 0.04045f) {
        out_linear[i * 3 + c] = x / 12.92f;
      } else {
        out_linear[i * 3 + c] = std::pow((x + 0.055f) / 1.055f, 2.4f);
      }
    }
  }
}

double butteraugli_score_to_quality(double score) {
  if (score <= 0.0) return 100.0;
  if (score >= 20.0) return 10.0;
  return 100.0 - score * 6.0;
}

double butteraugli_quality_to_score(double quality) {
  if (quality >= 100.0) return 0.0;
  if (quality <= 10.0) return 20.0;
  return (100.0 - quality) / 6.0;
}

float butteraugli_gamma(float v) {
  const float kInvLog2e = 1.0f / 1.4426950408889634f;
  const float kRetMul = 19.245013259874995f * kInvLog2e;
  const float kRetAdd = -23.16046239805755f;
  const float kBias = 9.9710635769299145f;

  if (v < 0.0f) v = 0.0f;
  float biased = v + kBias;
  float log_val = butteraugli_fast_log2f(biased);
  return kRetMul * log_val + kRetAdd;
}

float butteraugli_fast_log2f(float x) {
  // Exact C++ FastLog2f implementation from fast_math-inl.h
  const float p0 = -1.8503833400518310E-06f;
  const float p1 = 1.4287160470083755E+00f;
  const float p2 = 7.4245873327820566E-01f;

  const float q0 = 9.9032814277590719E-01f;
  const float q1 = 1.0096718572241148E+00f;
  const float q2 = 1.7409343003366853E-01f;

  union { float f; int32_t i; } u;
  u.f = x;
  int32_t x_bits = u.i;

  int32_t exp_bits = x_bits - 0x3f2aaaab;
  int32_t exp_shifted = exp_bits >> 23;
  int32_t mantissa_bits = x_bits - (exp_shifted << 23);
  u.i = mantissa_bits;
  float mantissa = u.f;
  float exp_val = static_cast<float>(exp_shifted);

  float m = mantissa - 1.0f;
  float yp = p2 * m + p1;
  yp = yp * m + p0;
  float yq = q2 * m + q1;
  yq = yq * m + q0;

  return yp / yq + exp_val;
}

}  // extern "C"
