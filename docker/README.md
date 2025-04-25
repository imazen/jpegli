# Quick Start: Processing Local Files with Docker

You can use the `imazen/jpegli-tools` Docker image to run `cjpegli` and `djpegli` on files located on your host machine without needing to install dependencies locally. This is done by mounting a local directory into the container using the `-v` flag with `docker run`.

The following examples assume your input and output files are in your current working directory (`.`). We mount this directory to `/work` inside the container.

## Compressing an image (`cjpegli`)

To compress an image (e.g., `input.png`) located in your current directory into a JPEG file (`output.jpg`) in the same directory, you can run:

```bash
# On Linux/macOS
docker run --rm -v "$(pwd):/work" imazen/jpegli-tools input.png output.jpg [OPTIONS...]

# On Windows (Command Prompt)
docker run --rm -v "%cd%:/work" imazen/jpegli-tools input.png output.jpg [OPTIONS...]

# On Windows (PowerShell)
docker run --rm -v "${PWD}:/work" imazen/jpegli-tools input.png output.jpg [OPTIONS...]
```

Since `cjpegli` is the default command, you don't need to specify it. Replace `[OPTIONS...]` with any desired `cjpegli` options (see reference below). For example, to set quality to 85: `... output.jpg -q 85`

To see `cjpegli` help, run the image without arguments:
`docker run --rm imazen/jpegli-tools`

## Decompressing an image (`djpegli`)

To decompress a JPEG image (e.g., `input.jpg`) located in your current directory into a PNG file (`output.png`) in the same directory:

```bash
# On Linux/macOS
docker run --rm -v "$(pwd):/work" imazen/jpegli-tools djpegli /work/input.jpg /work/output.png [OPTIONS...]

# On Windows (Command Prompt)
docker run --rm -v "%cd%:/work" imazen/jpegli-tools djpegli /work/input.jpg /work/output.png [OPTIONS...]

# On Windows (PowerShell)
docker run --rm -v "${PWD}:/work" imazen/jpegli-tools djpegli /work/input.jpg /work/output.png [OPTIONS...]
```

To run `djpegli`, you must specify it as the first argument. Replace `[OPTIONS...]` with any desired `djpegli` options (see reference below).

To run other commands (like getting a shell), specify the command:
`docker run --rm -it -v "$(pwd):/work" imazen/jpegli-tools bash`

# Command-Line Options Reference

# Using cjpegli and djpegli via imazen/jpegli-tools 



## Command line usage
```
Usage: cjpegli INPUT OUTPUT [OPTIONS...]
 INPUT
    the input can be JXL, PPM, PNM, PFM, PAM, PGX, PNG, APNG, GIF, JPEG, EXR
 OUTPUT
    the compressed JPEG output file
 --disable_output
    No output file will be written (for benchmarking)
 -x key=value, --dec-hints=key=value
    color_space indicates the ColorEncoding, see Description();
    icc_pathname refers to a binary file containing an ICC profile.
 -d maxError, --distance=maxError
    Max. butteraugli distance, lower = higher quality.
    1.0 = visually lossless (default).
    Recommended range: 0.5 .. 3.0. Allowed range: 0.0 ... 25.0.
    Mutually exclusive with --quality and --target_size.
 -q QUALITY, --quality=QUALITY
    Quality setting (is remapped to --distance).    Default is quality 90.
    Quality values roughly match libjpeg quality.
    Recommended range: 68 .. 96. Allowed range: 1 .. 100.
    Mutually exclusive with --distance and --target_size.
 --chroma_subsampling=444|440|422|420
    Chroma subsampling setting.
 -p N, --progressive_level=N
    Progressive level setting. Range: 0 .. 2.
    Default: 2. Higher number is more scans, 0 means sequential.
 --xyb
    Convert to XYB colorspace
 --std_quant
    Use quantization tables based on Annex K of the JPEG standard.
 --noadaptive_quantization
    Disable adaptive quantization.
 --fixed_code
    Disable Huffman code optimization. Must be used together with -p 0.
 --target_size=N
    If non-zero, set target size in bytes. This is useful for image
    quality comparisons, but makes encoding speed up to 20x slower.
    Mutually exclusive with --distance and --quality.
 --num_reps=N
    How many times to compress. (For benchmarking).
 --quiet
    Suppress informative output
 -v, --verbose
    Verbose output; can be repeated, also applies to help (!).

 -h, --help
    Prints this help message. All options are shown above.
```
```
Usage: djpegli INPUT OUTPUT [OPTIONS...]
 INPUT
    The JPEG input file.
 OUTPUT
    The output can be PPM, PNM, PFM, PAM, PGX, PNG, APNG, JPEG, EXR
 --disable_output
    No output file will be written (for benchmarking)
 --bitdepth=8|16
    Sets the output bitdepth for integer based formats, can be 8 (default) or 16. Has no impact on PFM output.
 --num_reps=N
    Sets the number of times to decompress the image. Used for benchmarking, the default is 1.
 --quiet
    Silence output (except for errors).

 -h, --help
    Prints this help message. All options are shown above.
```