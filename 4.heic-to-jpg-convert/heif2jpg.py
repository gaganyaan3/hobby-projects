import os
import sys
import pyheif
from PIL import Image

# Directory containing HEIC files (use "." for current directory)
input_dir = os.environ.get("INPUT_IMAGE_DIR", ".")
if not os.path.isdir(input_dir):
    raise FileNotFoundError(f"Input directory does not exist: {input_dir}")

for filename in os.listdir(input_dir):
    if filename.lower().endswith(".heic"):
        try:
            heif_path = os.path.join(input_dir, filename)
            jpg_filename = os.path.splitext(filename)[0] + ".jpg"
            jpg_path = os.path.join(input_dir, jpg_filename)

            # Read HEIC file
            heif_file = pyheif.read(heif_path)
            
            # Convert to Image
            image = Image.frombytes(
                heif_file.mode,
                heif_file.size,
                heif_file.data,
                "raw",
                heif_file.mode,
                heif_file.stride,
            )

            # Save as JPG
            image.save(jpg_path, "JPEG")
            print(f"Converted: {filename} -> {jpg_filename}")

        except Exception as e:
            print(f"Failed to convert {filename}: {e}")
