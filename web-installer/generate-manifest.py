from string import Template

import os

def generate(template: str, platform: str, version: str):
    with open(template, "r") as f:
        t = Template(f.read())
        print(t.substitute({ "PLATFORM": platform, "VERSION" : version }))

if __name__ == "__main__":
    template = "manifest.tmpl"
    platform = os.environ["PLATFORM"]
    version = os.environ["VERSION"]
    generate(template, platform, version)
