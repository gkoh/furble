from platformio.project.helpers import get_project_dir
from os.path import join, relpath

Import("env")

framework_dir = env.PioPlatform().get_package_dir("framework-espidf")

def apply_ble_gap_patch():
  src_path = join(framework_dir, "components", "bt", "host", "nimble", "nimble", "nimble", "host", "src", "ble_gap.c")
  print("{} --verbose -N -p1 -i {} {}".format("patch", "patches/ble_gap.patch", src_path))
  env.Execute("{} --verbose -N -p1 -i {} {}".format("patch", "patches/ble_gap.patch", src_path))

def apply_ble_gap_disconnect_event_patch(source, target, env):
  env.Execute("{} --verbose -N -p1 -i {} {}".format("patch", "patches/ble_gap_handle_disconnect_event.patch", source[0]))

# patch NimBLE
apply_ble_gap_patch()

# patch esp-nimble-cpp
for lib in env.GetLibBuilders():
  if lib.name == "esp-nimble-cpp":
    build_dir = relpath(env.subst(lib.build_dir), get_project_dir())
    tgt_path = join(build_dir, "NimBLEClient.o")
    env.AddPreAction(tgt_path, apply_ble_gap_disconnect_event_patch)
