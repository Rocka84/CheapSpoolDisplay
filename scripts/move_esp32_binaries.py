Import("env")
import os
import shutil

def move_binaries(target, source, env):
    # This script runs after the firmware.bin is created
    # We want to move all relevant binaries to web/bin/
    
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")
    target_dir = os.path.join(project_dir, "web", "bin")
    
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
        print(f"Created directory {target_dir}")

    binaries = [
        "firmware.bin",
        "bootloader.bin",
        "partitions.bin"
    ]

    for bin_file in binaries:
        source_path = os.path.join(build_dir, bin_file)
        if os.path.exists(source_path):
            target_path = os.path.join(target_dir, bin_file)
            shutil.copy2(source_path, target_path)
            print(f"Copied {bin_file} to {target_path}")
        else:
            print(f"Warning: {bin_file} not found at {source_path}")

# Add a post action to the build process
# We hook into the generation of the firmware binary
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", move_binaries)
