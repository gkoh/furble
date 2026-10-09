from string import Template

import os

BOARDS = {
    "m5stack-core": ("ESP32", 4096),
    "m5stack-core2": ("ESP32", 4096),
    "m5stick-c": ("ESP32", 4096),
    "m5stick-c-plus": ("ESP32", 4096),
    "m5stick-s3": ("ESP32-S3", 0),
    "xteink-x3": ("ESP32-C3", 0),
}

def generate(template: str, platform: str, version: str):
    chip_family, boot_offset = BOARDS[platform]
    with open(template, "r") as f:
        t = Template(f.read())
        print(t.substitute({"PLATFORM": platform, "VERSION": version,
                            "CHIP_FAMILY": chip_family, "BOOT_OFFSET": boot_offset}))

if __name__ == "__main__":
    template = "manifest.tmpl"
    platform = os.environ["PLATFORM"]
    version = os.environ["VERSION"]
    generate(template, platform, version)
