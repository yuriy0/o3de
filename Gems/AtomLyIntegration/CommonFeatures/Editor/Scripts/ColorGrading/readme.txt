# Tools
# OpenColorIO (ocio):
    https://opencolorio.org/
    https://opencolorio.readthedocs.io/en/latest/quick_start/downloads.html
    https://github.com/AcademySoftwareFoundation/OpenColorIO
    Notes: o3de doesn't fully integrate ocio (just builds dlls), doesn't include python bindings or tools
    Try: install this .whl into python: OpenImageIO‑2.2.8.0‑cp37‑cp37m‑win_amd64.whl
    Source: https://www.lfd.uci.edu/~gohlke/pythonlibs/#openimageio
    Why: Includes OpenColorIO
    Why Not: It doesn't include the ocio tools (used below), ociolutimage and ociobakelut
    Build it?: https://github.com/ttddee/ttddee.github.io/blob/master/building-oiio-win.md
        ^ can use a similar process to install/build opencolorio, opencolorio-tools
        Is this the same opencolorio-tools?: https://github.com/timmwagener/OpenColorIO_Tools

# OpenImageIO (oiio): o3de include oiio
    o3de-engine\build\bin\profile\OpenImageIO.dll
    o3de-engine\build\bin\profile\OpenImageIO.pyd
    Note: the oiiotool is currently only built in debug
        o3de-engine\build\bin\debug\oiiotool.exe

# generate a .azasset from a input 32x32x32 exr lut image

python exr_to_3dl_azasset.py --i graded_lut_32.exr --o graded_lut_32

# Step 1 - Capture a screenshot
# from within the o3de editor run this script
"o3de-engine\Gems\AtomLyIntegration\Editor\Scripts\ColorGrading\capture_displaymapperpassthrough.py"

# Step 2 - creating a base lut (with OpenColorIO)
# Note: OpenColorIO is not yet fully integrated with Lumberyard
# Use ociolutimage to create a normalized LUT to the desired size. In this case a 16x16x16 LUT is created.

ociolutimage --generate --cubesize 16 --output linear_lut_16.exr

# Use the LutHelper.py script above to convert this normalized LUT
# using a shaper function appropriate for the display where color grading will be done.
# Assuming a regular sRGB monitor is used here, this will be the 'Log2_48nits' shaper.
# If you are colorgrading on a HDR monitor use the shaper matching your monitors nits.

python lut_helper.py --i linear_lut_16.exr --op pre-grading --shaper Log2-48nits --o base_lut.exr

Step 3: Compositing the Base LUT with the Screenshot

OpenImageIO can be used to combine the Base LUT with the unmodified screenshot to create the input image for the color grading software.

For example: