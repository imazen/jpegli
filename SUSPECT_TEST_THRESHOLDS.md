# Suspect Test Thresholds Audit

This document catalogs all Rust test expectations that appear to have loosened thresholds or are otherwise suspect. These need review and tightening.

---

## HIGH SEVERITY

### 1. XYB Color Conversion - 15% Tolerance

**File**: `jpegli-rs/jpegli/tests/xyb_cpp_comparison.rs:166-191`

```rust
assert!((x - exp_x).abs() < 0.15, ...);
assert!((y - exp_y).abs() < 0.15, ...);
assert!((b_out - exp_b).abs() < 0.15, ...);
```

**Problem**: 15% tolerance is massive for XYB color conversion. Comment says "Rough range check (exact values depend on implementation details)" - but a port should match within floating point precision (`1e-6`), not 15%.

---

### 2. Butteraugli Diffmap - 25-35% OR Patterns

**File**: `jpegli-rs/butteraugli/tests/intermediate_values.rs`

| Line | Image Type | Threshold |
|------|------------|-----------|
| 218-222 | Uniform gray | `score_rel < 0.25 \|\| score_diff < 0.3` |
| 272 | Gradient | `score_rel < 0.30 \|\| score_diff < 0.5` |
| 317 | Checkerboard | `score_rel < 0.35 \|\| score_diff < 1.0` |
| 349 | Color gradient | `score_rel < 0.30 \|\| score_diff < 0.5` |
| 409 | Random 32x32 | `score_rel < 0.35 \|\| score_diff < 1.5` |

**Problem**:
- Uses logical OR with two loose thresholds
- 25-35% relative difference is unacceptable for C++ parity
- Thresholds escalate with image complexity = tuned to pass, not validate
- Uniform gray (simplest case) shouldn't need 25% tolerance

---

### 3. Commented Out Assertions

**File**: `jpegli-rs/jpegli/tests/aq_cpp_comparison.rs:399-400`

```rust
// This will fail - we're documenting the gap, not passing yet
// assert!(max_abs_diff < 0.01, "Implementation differs from C++ by {:.4}", max_abs_diff);
```

**Problem**: Disabled assertion creates false confidence. Should be `#[ignore]` or fixed.

---

### 4. AQ Strength Mismatch

**File**: `jpegli-rs/jpegli/tests/aq_cpp_comparison.rs:471`

```rust
max_diff <= 0.3
```

**Problem**: Line 14 comments show C++ uses 0.0-0.2 range, but test allows up to 0.3. This is 50% above expected range.

---

### 5. Pareto Front - 30% Worse Acceptable

**File**: `jpegli-rs/jpegli/tests/pareto_front.rs:385-386`

```rust
dssim_ratio < 1.3,
"At similar sizes, jpegli DSSIM ratio should be < 1.3, got {}",
```

**Problem**: Allows jpegli to be 30% worse than reference. No justification for why not 1.05 or 1.1.

---

## MEDIUM SEVERITY

### 6. Self-Comparison Tests (Not Comparing to C++)

**File**: `jpegli-rs/jpegli/tests/tone_mapping.rs:78-81, 129-132`

```rust
const REC2408_TONE_MAP_ERROR: f64 = 2.75e-5;  // From C++ tests
// But then used to compare Rust against Rust:
assert!((pq - pq_ref).abs() < REC2408_TONE_MAP_ERROR);
```

**Problem**: Uses C++ thresholds but compares Rust implementation against itself. These tests always pass but prove nothing about C++ parity.

**Also in**: `jpegli-rs/jpegli/tests/transfer_functions.rs:14-20, 73-80`

---

### 7. Corpus Comparison - 15% Size Tolerance

**File**: `jpegli-rs/jpegli/tests/corpus_cpp_comparison.rs:339`

```rust
size_diff_pct.abs() < 15.0,
```

**Problem**: 15% file size difference is enormous for a C++ parity test. Rust and C++ encoding same image should produce near-identical sizes.

---

### 8. Inconsistent DSSIM Thresholds

**File**: `jpegli-rs/jpegli/tests/decode_external.rs`

| Line | Threshold | Notes |
|------|-----------|-------|
| 59 | 0.0001 | |
| 83 | 0.0001 | |
| 107 | 0.01 | **100x looser** |

**Problem**: 100x variation with no explanation. Suggests thresholds tuned to specific images.

---

### 9. AQ Range Too Wide

**File**: `jpegli-rs/jpegli/tests/aq_locked_tests.rs:71-86`

```rust
params.mul[k] >= 0.0 && params.mul[k] <= 2.5,
params.offset[k] >= 0.0 && params.offset[k] <= 2.5,
```

**Problem**: Comment on line 70 says C++ values go up to ~2.1, but test allows 2.5 (20% buffer). Uncertainty about actual ranges.

---

### 10. Roundtrip Quality - Loose DSSIM

**File**: `jpegli-rs/jpegli/tests/roundtrip_quality.rs`

| Line | Image | Threshold |
|------|-------|-----------|
| 160 | Gradient | 0.002 |
| 186 | Solid color | 0.0001 |

**Problem**: No justification for values. No C++ reference comparison.

---

### 11. XYB Roundtrip - 5% Error

**File**: `jpegli-rs/jpegli/tests/xyb_roundtrip.rs:56, 67`

```rust
error_ratio < 0.05,  // 5% error allowed
assert!(x.abs() < 0.01, ...);  // 1% tolerance
```

**Problem**: Loose tolerances for roundtrip test. Should be near-lossless.

---

## LOW SEVERITY

### 12. Silent Divergence Acceptance

**File**: `jpegli-rs/butteraugli/tests/step_by_step_comparison.rs:339`

```rust
if max_diff > 0.001 || rel_diff > 1.0 {
    println!(...)
}
```

**Problem**: Prints divergence but doesn't assert. Up to 100% relative difference silently accepted.

---

### 13. Monotonicity Allows Backsliding

**File**: `jpegli-rs/jpegli/tests/metrics_comparison.rs:120-134`

```rust
dssim < prev_dssim + 0.001,  // Allows 0.001 increase
ssim2 > prev_ssim2 - 1.0,    // Allows 1.0 point drop!
```

**Problem**: SSIMULACRA2 monotonicity test allows 1.0 point drop - that's massive.

---

### 14. Quality Mapping No Assertion

**File**: `jpegli-rs/jpegli/tests/quality_mapping.rs:102-124`

```rust
// Binary search runs 20 iterations
// No assertion on final best_diff
// Test passes even if best match is 0.5 DSSIM apart
```

**Problem**: Test documents quality mapping but doesn't validate correctness.

---

### 15. Parity Enforcement Contradictory Baselines

**File**: `jpegli-rs/jpegli/tests/parity_enforcement.rs:33, 42, 45`

```rust
// Line 33: Baseline allows 4% difference for flower
// Line 45: Target is 1%
// Line 42: Regression tolerance is 0.5%
```

**Problem**: Target is 1% but baseline allows 4% - contradictory design. Tests can "regress" while still being "stable".

---

## Summary

| Severity | Count | Key Issues |
|----------|-------|------------|
| HIGH | 5 | XYB 15%, Butteraugli 25-35%, Commented assertions |
| MEDIUM | 6 | Self-comparison tests, 15% corpus tolerance, Inconsistent thresholds |
| LOW | 4 | Silent acceptance, Monotonicity backsliding, No assertions |

## Patterns Identified

1. **Copy-paste from C++ without validation** - C++ thresholds used for Rust-vs-Rust tests
2. **Thresholds scale with complexity** - Easier images should have tighter thresholds, not the same
3. **Logical OR escape hatches** - `a < X || b < Y` lets tests pass via multiple paths
4. **Disabled assertions** - Commented out or `#[ignore]` without tracking
5. **Arbitrary percentages** - 0.15, 0.25, 0.30, 0.35 with no justification
6. **100x variations** - Same metric, wildly different thresholds between tests

## Recommended Actions

1. Replace OR assertions with single strict thresholds
2. Document justification for each threshold with C++ reference
3. Fix or properly ignore disabled assertions
4. Add actual C++ comparison to self-comparison tests
5. Reduce Butteraugli thresholds to <5% for simple images
6. Investigate and justify 100x threshold variations
