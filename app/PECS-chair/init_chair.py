#!/usr/bin/python
import subprocess

fs = None
while fs not in ("firestorm", "chair"):
    fs = raw_input('Program a firestorm or a chair? Enter "firestorm" or "chair": ')
    
if fs == "chair":
    reset = None
    while reset not in ('reset', 'flash', 'name'):
        reset = raw_input('Reset the log, flash the chair, or set the bluetooth name? Enter "reset", "flash", or "name": ')
    if reset == "name":
        bl_name = raw_input('Enter the new bluetooth name: ')
    serial = None
    while serial not in ('yes', 'no'):
        serial = raw_input('Change the serial number? Enter "yes" or "no": ')
    if serial == "yes":
        new_serial = raw_input('Enter the new serial number: ')
        subprocess.call(["sload", "sattr", "0", "serial", "-x", new_serial])
    with open("bl_name.lua", "w") as init:
        init.write("BL_NAME = {}\n")
        init.write("function set_name()\n")
        if reset == "name":
            init.write('    bl_name = "{0}"\n'.format(bl_name))
        init.write("end\n")
        init.write("BL_NAME.set_name = set_name\n")
        init.write("return BL_NAME")
    
cd = "cd /home/oski/workspace/"
build_dir = "PECS-chair/app/PECS-chair/build"
if fs == "firestorm":
    subprocess.call(["{0} && ./{1}/build_fs.lua".format(cd, build_dir)], shell=True)
elif reset == "reset":
    subprocess.call(["{0} && ./{1}/build_reset_log.lua".format(cd, build_dir)], shell=True)
elif reset == "name":
    subprocess.call(["{0} && ./{1}/build_bl_name.lua".format(cd, build_dir)], shell=True)
else:
    subprocess.call(["{0} && ./{1}/build_chair.lua".format(cd, build_dir)], shell=True)
