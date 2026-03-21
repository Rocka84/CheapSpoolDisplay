import json
import os

Import("env")

proj_dir = env.get("PROJECT_DIR")
version_path = os.path.join(proj_dir, "VERSION")
manifest_path = os.path.join(proj_dir, "web", "manifest.json")

# Read version from VERSION (the source of truth)
try:
    with open(version_path, "r") as f:
        version = f.read().strip()
except Exception as e:
    print(f"Warning: Could not read VERSION: {e}")
    version = "unknown"

# Inject into C++
print(f"Injecting FIRMWARE_VERSION={version} from VERSION")
env.Append(CPPDEFINES=[("FIRMWARE_VERSION", f'\\"{version}\\"')])

# Keep web/manifest.json automatically synced
try:
    with open(manifest_path, "r") as f:
        manifest = json.load(f)
    if manifest.get("version") != version:
        print(f"Updating web/manifest.json version to {version}")
        manifest["version"] = version
        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=2)
except Exception as e:
    print(f"Warning: Could not sync web/manifest.json: {e}")
