Import("env")
import os
import shutil

def move_binary(target, source, env):
    # 'target': the file that was just built (.pio/build/simulator/program)
    # 'source': the files used to build it (unused here)
    
    # copy the newly built binary into our simulator root folder
    dest_dir = os.path.join(env.get("PROJECT_DIR"), "simulator")
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
        
    dest_path = os.path.join(dest_dir, "program")
    
    # target[0] is the primary output file built by SCons
    shutil.copy2(str(target[0]), dest_path)
    print(f"Copied simulator binary to {dest_path}")

# Add a post action to the program creation
env.AddPostAction("$BUILD_DIR/program${PROGSUFFIX}", move_binary)
