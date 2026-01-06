from platformio.project.helpers import get_project_dir
from os.path import join, relpath

Import("env")

framework_dir = env.PioPlatform().get_package_dir("framework-espidf")

def apply_ble_gap_patch():
  src_path = join(framework_dir, "components", "bt", "host", "nimble", "nimble", "nimble", "host", "src", "ble_gap.c")
  env.Execute("{} --verbose -N -p1 -i {} {}".format("patch", "patches/ble_gap.patch", src_path))

# patch NimBLE
apply_ble_gap_patch()
