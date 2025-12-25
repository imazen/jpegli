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

butteraugli_error_t butteraugli_opsin_dynamics(
    const float* linear_rgb,
    size_t width,
    size_t height,
    float intensity_target,
    float* out_xyb) {
  if (!linear_rgb || !out_xyb || width == 0 || height == 0) {
    return BUTTERAUGLI_ERROR_INVALID_INPUT;
  }

  // Create Image3F for input RGB
  auto result_rgb = jxl::Image3F::Create(&g_default_memory_manager, width, height);
  if (!result_rgb.ok()) {
    return BUTTERAUGLI_ERROR_MEMORY;
  }
  jxl::Image3F rgb = std::move(result_rgb).value_();

  // Copy interleaved linear RGB to planar Image3F
  for (size_t y = 0; y < height; ++y) {
    float* row_r = rgb.PlaneRow(0, y);
    float* row_g = rgb.PlaneRow(1, y);
    float* row_b = rgb.PlaneRow(2, y);
    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      row_r[x] = linear_rgb[idx + 0];
      row_g[x] = linear_rgb[idx + 1];
      row_b[x] = linear_rgb[idx + 2];
    }
  }

  // Create output XYB and temp images
  auto result_xyb = jxl::Image3F::Create(&g_default_memory_manager, width, height);
  auto result_temp = jxl::Image3F::Create(&g_default_memory_manager, width, height);
  if (!result_xyb.ok() || !result_temp.ok()) {
    return BUTTERAUGLI_ERROR_MEMORY;
  }
  jxl::Image3F xyb = std::move(result_xyb).value_();
  jxl::Image3F temp = std::move(result_temp).value_();

  // Set up parameters and blur temp
  jxl::ButteraugliParams params;
  params.intensity_target = intensity_target;
  jxl::BlurTemp blur_temp;

  // Call OpsinDynamicsImage via ButteraugliComparator
  // We'll use the full diffmap path since OpsinDynamicsImage is internal
  auto comparator_result = jxl::ButteraugliComparator::Make(rgb, params);
  if (!comparator_result.ok()) {
    return BUTTERAUGLI_ERROR_INTERNAL;
  }

  // The comparator has already computed OpsinDynamicsImage internally
  // For intermediate testing, we need to compute it separately
  // Use a minimal reimplementation that matches the internal logic

  // OpsinDynamicsImage constants
  const float kSigma = 1.2f;
  const float min_val = 1e-4f;

  // OpsinAbsorbance matrix (with bias for <true> version)
  const double mix0_r = 0.29956550340058319;
  const double mix0_g = 0.63373087833825936;
  const double mix0_b = 0.077705617820981968;
  const double mix0_bias = 1.7557483643287353;

  const double mix1_r = 0.22158691104574774;
  const double mix1_g = 0.69391388044116142;
  const double mix1_b = 0.0987313588422;
  const double mix1_bias = 1.7557483643287353;

  const double mix2_r = 0.02;
  const double mix2_g = 0.02;
  const double mix2_b = 0.20480129041026129;
  const double mix2_bias = 12.226454707163354;

  const float min01 = 1.7557483643287353f;
  const float min2 = 12.226454707163354f;

  // Simple box blur approximation for sigma=1.2 (3x3 kernel)
  // This is a simplified version - the actual C++ uses separable Gaussian blur
  auto blur_plane = [&](const jxl::Plane<float>& in, jxl::Plane<float>& out) {
    for (size_t y = 0; y < height; ++y) {
      for (size_t x = 0; x < width; ++x) {
        float sum = 0.0f;
        float count = 0.0f;
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            int ny = static_cast<int>(y) + dy;
            int nx = static_cast<int>(x) + dx;
            if (ny >= 0 && ny < static_cast<int>(height) &&
                nx >= 0 && nx < static_cast<int>(width)) {
              // Gaussian weights for sigma~1.2
              float w = (dx == 0 && dy == 0) ? 0.25f :
                        (dx == 0 || dy == 0) ? 0.125f : 0.0625f;
              sum += in.ConstRow(ny)[nx] * w;
              count += w;
            }
          }
        }
        out.Row(y)[x] = sum / count * (0.25f + 4*0.125f + 4*0.0625f);
      }
    }
  };

  // Create blurred image
  auto result_blurred = jxl::Image3F::Create(&g_default_memory_manager, width, height);
  if (!result_blurred.ok()) {
    return BUTTERAUGLI_ERROR_MEMORY;
  }
  jxl::Image3F blurred = std::move(result_blurred).value_();

  blur_plane(rgb.Plane(0), blurred.Plane(0));
  blur_plane(rgb.Plane(1), blurred.Plane(1));
  blur_plane(rgb.Plane(2), blurred.Plane(2));

  // Apply OpsinDynamicsImage logic
  for (size_t y = 0; y < height; ++y) {
    const float* row_r = rgb.ConstPlaneRow(0, y);
    const float* row_g = rgb.ConstPlaneRow(1, y);
    const float* row_b = rgb.ConstPlaneRow(2, y);
    const float* blur_r = blurred.ConstPlaneRow(0, y);
    const float* blur_g = blurred.ConstPlaneRow(1, y);
    const float* blur_b = blurred.ConstPlaneRow(2, y);
    float* out_x = xyb.PlaneRow(0, y);
    float* out_y = xyb.PlaneRow(1, y);
    float* out_b = xyb.PlaneRow(2, y);

    for (size_t x = 0; x < width; ++x) {
      float it = intensity_target;

      // Blurred RGB scaled by intensity target
      float br = blur_r[x] * it;
      float bg = blur_g[x] * it;
      float bb = blur_b[x] * it;

      // OpsinAbsorbance with bias (for sensitivity)
      float pre0 = std::max(static_cast<float>(mix0_r * br + mix0_g * bg + mix0_b * bb + mix0_bias), min_val);
      float pre1 = std::max(static_cast<float>(mix1_r * br + mix1_g * bg + mix1_b * bb + mix1_bias), min_val);
      float pre2 = std::max(static_cast<float>(mix2_r * br + mix2_g * bg + mix2_b * bb + mix2_bias), min_val);

      // Sensitivity = Gamma(pre) / pre
      float sens0 = std::max(butteraugli_gamma(pre0) / pre0, min_val);
      float sens1 = std::max(butteraugli_gamma(pre1) / pre1, min_val);
      float sens2 = std::max(butteraugli_gamma(pre2) / pre2, min_val);

      // Current RGB scaled by intensity target
      float cr = row_r[x] * it;
      float cg = row_g[x] * it;
      float cb = row_b[x] * it;

      // OpsinAbsorbance without bias
      float cur0 = mix0_r * cr + mix0_g * cg + mix0_b * cb;
      float cur1 = mix1_r * cr + mix1_g * cg + mix1_b * cb;
      float cur2 = mix2_r * cr + mix2_g * cg + mix2_b * cb;

      // Apply sensitivity
      cur0 = std::max(cur0 * sens0, min01);
      cur1 = std::max(cur1 * sens1, min01);
      cur2 = std::max(cur2 * sens2, min2);

      // Convert to XYB
      out_x[x] = cur0 - cur1;
      out_y[x] = cur0 + cur1;
      out_b[x] = cur2;
    }
  }

  // Copy planar XYB to interleaved output
  for (size_t y = 0; y < height; ++y) {
    const float* row_x = xyb.ConstPlaneRow(0, y);
    const float* row_y = xyb.ConstPlaneRow(1, y);
    const float* row_b = xyb.ConstPlaneRow(2, y);
    for (size_t x = 0; x < width; ++x) {
      size_t idx = (y * width + x) * 3;
      out_xyb[idx + 0] = row_x[x];
      out_xyb[idx + 1] = row_y[x];
      out_xyb[idx + 2] = row_b[x];
    }
  }

  return BUTTERAUGLI_OK;
}

}  // extern "C"
