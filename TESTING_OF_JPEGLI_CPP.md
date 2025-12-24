# C++ Testing Documentation for jpegli

This document details all unit and integration tests in the jpegli C++ codebase.

## Testing Framework

- **Primary Framework**: Google Test (GTest)
- **SIMD Tests**: Highway (HWY) test framework for vectorized code
- **Build System**: CMake with automatic test discovery via `gtest_discover_tests()`
- **Test Timeout**: 240 seconds per test

## Test Files Overview

| Category | Files | Lines | Test Cases |
|----------|-------|-------|------------|
| jpegli Core | 8 | ~3,960 | 90+ |
| Color/CMS | 2 | ~269 | 8+ |
| Extras/Codec | 4 | ~1,305 | 20+ |
| Threading | 1 | ~130 | 3 |
| Tools | 1 | ~803 | 5 |
| **Total** | **16** | **~6,500** | **126+** |

---

## JPEGLI Core Tests

### 1. Encode API Tests

**File**: `lib/jpegli/encode_api_test.cc` (871 lines)

**Source files tested**:
- `lib/jpegli/encode.h`
- `lib/jpegli/common.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST_P(EncodeAPITestParam, TestAPI)` | Parametrized encoding with various configurations |
| `TEST(EncodeAPITest, ReuseCinfoSameImageTwice)` | Encoder context reuse |
| `TEST(EncodeAPITest, ReuseCinfoSameMemOutput)` | Multiple images with same output buffer |
| `TEST(EncodeAPITest, ReuseCinfoSameStdOutput)` | Multiple images with stdio output |
| `TEST(EncodeAPITest, ReuseCinfoChangeParams)` | Parameter changes between encodes |
| `TEST(EncodeAPITest, AbbreviatedStreams)` | Abbreviated JPEG streams |
| `TEST(EncodeAPITest, QualitySettings)` | JPEG quality settings consistency |

### 2. Decode API Tests

**File**: `lib/jpegli/decode_api_test.cc` (685 lines)

**Source files tested**:
- `lib/jpegli/decode.h`
- `lib/jpegli/decode.cc`
- `lib/jpegli/decode_marker.cc`
- `lib/jpegli/decode_scan.cc`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(DecodeAPITest, ReuseCinfo)` | Decoder context reuse |
| `TEST(DecodeAPITest, ReuseCinfoSameMemSource)` | Multiple images from memory |
| `TEST(DecodeAPITest, ReuseCinfoSameStdSource)` | Multiple images from stdio |
| `TEST(DecodeAPITest, AbbreviatedStreams)` | Abbreviated JPEG streams |
| `TEST_P(DecodeAPITestParam, TestAPI)` | Parametrized decode tests |
| `TEST_P(DecodeAPITestParamBuffered, TestAPI)` | Buffered decode with custom source manager |

### 3. Error Handling Tests

**File**: `lib/jpegli/error_handling_test.cc` (1,275 lines)

**Source files tested**:
- `lib/jpegli/encode.h`
- `lib/jpegli/decode.h`
- `lib/jpegli/error.cc`

**Encoder error tests** (50+ cases):
- `MinimalSuccess` - Minimal working encoder setup
- `NoDestination`, `NoImageDimensions`, `ImageTooBig` - Dimension validation
- `NoInputComponents`, `TooManyInputComponents` - Component validation
- `NoSetDefaults`, `NoStartCompress`, `NoWriteScanlines` - Initialization errors
- `InvalidQuantValue`, `InvalidQuantTableIndex` - Quantization table errors
- `NumberOfComponentsMismatch1-6` - Component count validation
- `InvalidColorTransform`, `DuplicateComponentIds` - Component configuration
- `InvalidScanScript1-13` - Scan script validation (13 variations)
- `MCUSizeTooBig`, `RestartIntervalTooBig` - Parameter bounds checking
- `SamplingFactorTooBig`, `NonIntegralSamplingRatio` - Sampling validation

**Decoder error tests** (10+ cases):
- `MinimalSuccess` - Minimal working decoder setup
- `NoSource`, `NoReadHeader`, `NoStartDecompress` - Initialization errors
- `NoSOI`, `InvalidDQT`, `InvalidSOF`, `InvalidDHT`, `InvalidSOS` - Marker validation
- `MutateSingleBytes` - Robustness against corrupted data

### 4. Input Suspension Tests

**File**: `lib/jpegli/input_suspension_test.cc` (442 lines)

**Source files tested**:
- `lib/jpegli/decode.h`
- Input buffer management

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST_P(InputSuspensionTestParam, InputOutputLockStepNonBuffered)` | Non-buffered chunked input |
| `TEST_P(InputSuspensionTestParam, InputOutputLockStepBuffered)` | Buffered chunked input |
| `TEST_P(InputSuspensionTestParam, PreConsumeInputBuffered)` | Pre-consuming input buffer |
| `TEST_P(InputSuspensionTestParam, PreConsumeInputNonBuffered)` | Pre-consuming without buffer |

### 5. Output Suspension Tests

**File**: `lib/jpegli/output_suspension_test.cc` (178 lines)

**Source files tested**:
- `lib/jpegli/decode.h`
- Output buffer management

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST_P(OutputSuspensionTestParam, PixelData)` | Output suspension with pixel data |
| `TEST_P(OutputSuspensionTestParam, RawData)` | Output suspension with raw data |

### 6. Source Manager Tests

**File**: `lib/jpegli/source_manager_test.cc` (119 lines)

**Source files tested**:
- `lib/jpegli/source_manager.cc`
- `lib/jpegli/decode.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST_P(SourceManagerTestParam, TestStdioSourceManager)` | Stdio-based source manager |
| `TEST_P(SourceManagerTestParam, TestMemSourceManager)` | Memory-based source manager |

### 7. Streaming Tests

**File**: `lib/jpegli/streaming_test.cc` (247 lines)

**Source files tested**:
- `lib/jpegli/encode.h`
- `lib/jpegli/encode_streaming.cc`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST_P(StreamingTestParam, TestStreaming)` | Streaming encode/decode roundtrips |

### 8. Transcode API Tests

**File**: `lib/jpegli/transcode_api_test.cc` (143 lines)

**Source files tested**:
- `lib/jpegli/encode.h`
- `lib/jpegli/decode.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST_P(TranscodeAPITestParam, TestAPI)` | Transcoding various JPEG images |

---

## Color and Tone Mapping Tests

### 9. Tone Mapping Tests

**File**: `lib/cms/tone_mapping_test.cc` (155 lines)

**Source files tested**:
- `lib/cms/tone_mapping.h`
- `lib/cms/tone_mapping-inl.h`

**Framework**: Highway Test Framework (HWY_EXPORT_AND_TEST_P)

**Test functions**:
| Test | Description |
|------|-------------|
| `TestRec2408ToneMap()` | Rec. 2408 tone mapping validation (8M+ iterations) |
| `TestHlgOotfApply()` | HLG OOTF tone curve application (8M+ iterations) |
| `TestGamutMap()` | Gamut mapping validation |

### 10. Transfer Functions Tests

**File**: `lib/cms/transfer_functions_test.cc` (114 lines)

**Source files tested**:
- `lib/cms/transfer_functions.h`
- `lib/cms/transfer_functions-inl.h`

**Framework**: Highway Test Framework (HWY_EXPORT_AND_TEST_P)

**Test functions**:
| Test | Description |
|------|-------------|
| `TestPqEncodedFromDisplay()` | PQ encoding curve tests |
| `TestHlgEncodedFromDisplay()` | HLG encoding curve tests |
| `TestPqDisplayFromEncoded()` | PQ decoding curve tests |
| `TestHlgDisplayFromEncoded()` | HLG decoding curve tests |

---

## Extras/Codec Tests

### 11. Jpegli Codec Integration Tests

**File**: `lib/extras/jpegli_test.cc` (446 lines)

**Source files tested**:
- `lib/extras/dec/jpegli.h`
- `lib/extras/enc/jpegli.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(JpegliTest, JpegliSRGBDecodeTest)` | SRGB JPEG decoding |
| `TEST(JpegliTest, JpegliGrayscaleDecodeTest)` | Grayscale JPEG decoding |
| `TEST(JpegliTest, JpegliXYBEncodeTest)` | XYB color space encoding |
| `TEST(JpegliTest, JpegliDecodeTestLargeSmoothArea)` | Large smooth area handling |
| `TEST(JpegliTest, JpegliYUVEncodeTest)` | YUV color space encoding |
| `TEST(JpegliTest, JpegliYUVChromaSubsamplingEncodeTest)` | YUV with chroma subsampling |
| `TEST(JpegliTest, JpegliYUVEncodeTestNoAq)` | YUV without adaptive quantization |
| `TEST(JpegliTest, JpegliHDRRoundtripTest)` | HDR encoding/decoding roundtrip |
| `TEST(JpegliTest, JpegliSetAppData)` | Application marker data handling |
| `TEST_P(JpegliColorQuantTestParam, JpegliColorQuantizeTest)` | Color quantization |

### 12. Butteraugli Tests

**File**: `lib/extras/butteraugli_test.cc` (115 lines)

**Source files tested**:
- `lib/extras/butteraugli.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(ButteraugliInPlaceTest, SinglePixel)` | Single-pixel image quality |
| `TEST(ButteraugliInPlaceTest, LargeImage)` | Large image quality computation |

### 13. Color Description Tests

**File**: `lib/extras/dec/color_description_test.cc` (105 lines)

**Source files tested**:
- `lib/extras/dec/color_description.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(ColorDescriptionTest, RoundTripAll)` | All color spaces roundtrip |
| `TEST(ColorDescriptionTest, NanGamma)` | NaN gamma value handling |

### 14. Codec Tests

**File**: `lib/extras/codec_test.cc` (639 lines)

**Source files tested**:
- `lib/extras/dec/decode.h`
- `lib/extras/enc/encode.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(CodecTest, TestRoundTrip)` | Full encode/decode roundtrips |
| `TEST(CodecTest, LosslessPNMRoundtrip)` | Lossless PNM format |
| `TEST(CodecTest, TestPNM)` | PNM-specific tests |
| `TEST(CodecTest, EncodeToPNG)` | PNG encoding tests |

---

## Threading Tests

### 15. Thread Parallel Runner Tests

**File**: `lib/threads/thread_parallel_runner_test.cc` (130 lines)

**Source files tested**:
- `lib/threads/thread_parallel_runner.cc`
- `lib/threads/thread_parallel_runner.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(ThreadParallelRunnerTest, TestPool)` | Thread pool initialization |
| `TEST(ThreadParallelRunnerTest, TestSmallAssignments)` | Small task assignments |
| `TEST(ThreadParallelRunnerTest, TestCounter)` | Thread-safe counters |

---

## Tools Tests

### 16. Gauss Blur Tests

**File**: `tools/gauss_blur_test.cc` (803 lines)

**Source files tested**:
- `lib/jxl/base/data_parallel.h`
- `lib/jxl/simd/jxl_gaussian_blur.h`

**Test cases**:
| Test | Description |
|------|-------------|
| `TEST(GaussBlurTest, ImpulseResponse)` | Impulse response validation |
| `TEST(GaussBlurTest, Test2D)` | 2D blur kernel testing |
| `TEST(GaussBlurTest, DISABLED_SlowTestDirac1D)` | Slow Dirac impulse test (disabled) |
| `TEST(GaussBlurTest, TestRandom)` | Random input testing |
| `TEST(GaussBlurTest, TestSign)` | Sign preservation in blur |

---

## Quality Metrics

### RMS (Root Mean Square) Distance

**Location**: `lib/jpegli/test_utils.cc:814-842`

Computes per-pixel difference between original and decoded images. Returns RMS error scaled to 0-255 range.

```cpp
double DistanceRms(const TestImage& input, const TestImage& output,
                   size_t start_line, size_t num_lines, double* max_diff);
```

**Verification** (`VerifyOutputImage`):
```cpp
Check(rms <= max_rms);      // RMS must be below threshold
Check(max_d <= max_diff);   // No pixel can differ by more than max_diff
```

**Thresholds by test**:
| Test | max_rms |
|------|---------|
| `encode_api_test.cc` | 2.1 - 20.0 (quality dependent) |
| `decode_api_test.cc` | 2.35 |
| `streaming_test.cc` | 3.8 |
| `output_suspension_test.cc` | 2.5 - 3.5 |
| `source_manager_test.cc` | 1.0 |

### Butteraugli (Perceptual Quality Metric)

**Location**: `lib/extras/butteraugli.h`

Psychovisual image quality metric that models human visual perception.

**Score interpretation**:
- **< 1.0** = Images appear identical
- **1.0 - 2.0** = Subtle difference observable
- **> 2.0** = Noticeable difference

**Key functions**:
```cpp
ButteraugliDistance(memory_manager, ppf0, ppf1)  // Full comparison
Butteraugli3Norm(memory_manager, ppf0, ppf1)     // 3-norm variant
```

**Thresholds in jpegli_test.cc**:
| Test | Threshold |
|------|-----------|
| `JpegliXYBEncodeTest` | < 1.84 |
| `JpegliDecodeTestLargeSmoothArea` | < 3.0 |
| `JpegliYUVEncodeTest` | < 1.85 |
| `JpegliYUVChromaSubsamplingEncodeTest` | ≤ 1.82 |
| `JpegliYUVEncodeTestNoAq` | < 2.2 |
| `JpegliHDRRoundtripTest` | < 1.5 |

**Comparative testing**:
```cpp
// Verifies jpegli produces better quality than libjpeg at same settings
EXPECT_LT(ButteraugliDistance(ppf0, ppf_jpegli),
          ButteraugliDistance(ppf0, ppf_libjpeg));
```

---

## Test Data

### Directory Structure

```
testdata/
├── dots/
│   └── ellipses.png
├── external/
│   ├── pngsuite/           (10 PNG test suite images)
│   ├── raw.pixls/          (10 camera raw conversions)
│   ├── wesaturate/
│   │   ├── 500px/          (5 images)
│   │   └── 64px/           (22 small test images)
│   └── wide-gamut-tests/   (11 wide gamut test images)
├── jxl/
│   ├── blending/           (5 animation frames)
│   ├── boxes/              (1 JXL container test)
│   ├── chessboard/         (1 PNG)
│   ├── flower/             (100+ files - primary test images)
│   ├── jpeg_reconstruction/ (4 files)
│   └── color_management/   (ICC profiles)
├── oss-fuzz/               (47 fuzzer test cases)
└── palette/
    └── 358colors.png
```

**Total**: 285 files

### Key Test Images

| Image | Format | Used By |
|-------|--------|---------|
| `jxl/flower/flower.png` | PNG | Base image for JPEG encoding tests |
| `jxl/flower/flower_small.rgb.depth8.ppm` | PPM 8-bit | Most jpegli encode/decode tests |
| `jxl/flower/flower_small.g.depth8.pgm` | PGM 8-bit | Grayscale tests |
| `jxl/hdr_room.png` | PNG 16-bit | HDR/tone mapping tests |
| `external/wesaturate/500px/tmshre_riaphotographs_srgb8.png` | PNG | Alpha channel codec test |

### JPEG Test Variants

The `flower.png.im_q85_*.jpg` files cover:
- **Subsampling**: 4:4:4, 4:2:2, 4:2:0, 4:4:0, asymmetric
- **Scan modes**: Progressive, non-interleaved, partially interleaved
- **Color spaces**: YCbCr, RGB, grayscale, CMYK
- **Special modes**: Luma subsampling, blue channel subsampling

### Images Referenced in Tests

**decode_api_test.cc** (17 JPEG files):
- `jxl/flower/flower.png.im_q85_420_progr.jpg`
- `jxl/flower/flower.png.im_q85_420_R13B.jpg`
- `jxl/flower/flower.png.im_q85_444.jpg`
- `jxl/flower/flower.png.im_q85_422.jpg`
- `jxl/flower/flower.png.im_q85_440.jpg`
- `jxl/flower/flower.png.im_q85_444_1x2.jpg`
- `jxl/flower/flower.png.im_q85_asymmetric.jpg`
- `jxl/flower/flower.png.im_q85_gray.jpg`
- `jxl/flower/flower.png.im_q85_luma_subsample.jpg`
- `jxl/flower/flower.png.im_q85_rgb.jpg`
- `jxl/flower/flower.png.im_q85_rgb_subsample_blue.jpg`
- `jxl/flower/flower_small.q85_444_non_interleaved.jpg`
- `jxl/flower/flower_small.q85_420_non_interleaved.jpg`
- `jxl/flower/flower_small.q85_444_partially_interleaved.jpg`
- `jxl/flower/flower_small.q85_420_partially_interleaved.jpg`
- `jxl/flower/flower_small.cmyk.jpg`

**source_manager_test.cc** (3 JPEG files):
- `jxl/flower/flower.png.im_q85_444.jpg`
- `jxl/flower/flower.png.im_q85_420.jpg`
- `jxl/flower/flower.png.im_q85_420_R13B.jpg`

**input_suspension_test.cc** (3 JPEG files):
- `jxl/flower/flower.png.im_q85_444.jpg`
- `jxl/flower/flower.png.im_q85_420_R13B.jpg`
- `jxl/flower/flower.png.im_q85_420_progr.jpg`

**jpegli_test.cc** (3 image files):
- `jxl/flower/flower_small.rgb.depth8.ppm`
- `jxl/flower/flower_small.g.depth8.pgm`
- `jxl/hdr_room.png`

---

## Test Support Infrastructure

### Test Utilities

**jpegli utilities** (`lib/jpegli/`):
| File | Purpose |
|------|---------|
| `test_utils.h/cc` | JPEG encoding/decoding utilities, image generation, verification |
| `libjpeg_test_util.h/cc` | libjpeg API compatibility helpers |
| `test_params.h` | Test parameter definitions |
| `testing.h` | Testing macros and utilities |
| `fuzztest.h` | Fuzzing infrastructure |

**extras utilities** (`lib/extras/`):
| File | Purpose |
|------|---------|
| `test_utils.h/cc` | General image testing utilities |
| `test_image.h/cc` | Test image generation and manipulation |
| `test_memory_manager.h/cc` | Memory allocation testing |

### Key Functions

```cpp
// Read test data from testdata/ directory
std::string GetTestDataPath(const std::string& filename);
std::vector<uint8_t> ReadTestData(const std::string& filename);

// Verify output quality
void VerifyOutputImage(const TestImage& input, const TestImage& output,
                       double max_rms, double max_diff = 255.0);

// Compute distance metrics
double DistanceRms(const TestImage& input, const TestImage& output,
                   double* max_diff);
```

---

## Building and Running Tests

### CMake Configuration

Tests are configured in:
- `lib/jxl_tests.cmake`
- `lib/jpegli.cmake`
- `lib/jxl_lists.cmake`

### Running Tests

```bash
# Build with tests enabled
cmake -B build -DBUILD_TESTING=ON
cmake --build build

# Run all tests
ctest --test-dir build

# Run specific test
./build/tests/encode_api_test
./build/tests/jpegli_test

# Run with verbose output
ctest --test-dir build --output-on-failure
```

### Test Dependencies

- `GTest::GTest`, `GTest::Main` - Google Test framework
- `jxl_testlib-internal` - Shared test library
- `jxl_extras-internal` - Image codec utilities
- `jpegli-static` - Static JPEG encoder/decoder library
- `hwy` - Highway SIMD library
