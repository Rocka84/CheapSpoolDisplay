Import("env")
import os
import shutil

def move_binary(target, source, env):
    # target is the built binary (e.g. .pio/build/simulator/program)
    # target_dir is the target path
    
    # We want to move it to simulator/program
    target_dir = os.path.join(env.get("PROJECT_DIR"), "simulator")
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
        
    target_path = os.path.join(target_dir, "program")
    shutil.copy2(str(target[0]), target_path)
    print(f"Copied simulator binary to {target_path}")

# Add a post action to the program creation
env.AddPostAction("$BUILD_DIR/program${PROGSUFFIX}", move_binary)
