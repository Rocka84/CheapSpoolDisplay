Import("env")
import os
import json

import glob
proj_dir = env.get("PROJECT_DIR")
version_path = os.path.join(proj_dir, "VERSION")

# Read version from VERSION (the source of truth)
try:
    with open(version_path, "r") as f:
        version = f.read().strip()
except Exception as e:
    print(f"Warning: Could not read VERSION: {e}")
    version = "unknown"

# Inject into C++
print(f"Injecting PROJECT_VERSION={version} from VERSION")
env.Append(CPPDEFINES=[("PROJECT_VERSION", f'\\"{version}\\"')])

# Keep web/manifest*.json automatically synced
manifests = glob.glob(os.path.join(proj_dir, "web", "manifest*.json"))
for m_path in manifests:
    try:
        with open(m_path, "r") as f:
            manifest = json.load(f)
        if manifest.get("version") != version:
            print(f"Updating {os.path.basename(m_path)} version to {version}")
            manifest["version"] = version
            with open(m_path, "w") as f:
                json.dump(manifest, f, indent=2)
    except Exception as e:
        print(f"Warning: Could not sync {os.path.basename(m_path)}: {e}")
