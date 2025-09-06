from platformio.project.helpers import get_project_dir
from os.path import join, relpath

Import("env")

def apply_ble_gap_patch(source, target, env):
  env.Execute("{} --verbose -N -p1 -i {} {}".format("patch", "patches/ble_gap.patch", source[0]))

def apply_ble_gap_disconnect_event_patch(source, target, env):
  env.Execute("{} --verbose -N -p1 -i {} {}".format("patch", "patches/ble_gap_handle_disconnect_event.patch", source[0]))

for lib in env.GetLibBuilders():
  if lib.name == "NimBLE-Arduino":
    build_dir = relpath(env.subst(lib.build_dir), get_project_dir())
    tgt_path = join(build_dir, "nimble", "nimble", "host", "src", "ble_gap.c.o")
    env.AddPreAction(tgt_path, apply_ble_gap_patch)
    tgt_path = join(build_dir, "NimBLEClient.cpp.o")
    env.AddPreAction(tgt_path, apply_ble_gap_disconnect_event_patch)
