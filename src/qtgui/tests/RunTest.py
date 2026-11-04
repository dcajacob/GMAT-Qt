"""CTest launcher: runtime directories followed by the test command."""
import os
import subprocess
import sys
from AcceptanceRuntime import arguments, environment as acceptance_environment

environment = acceptance_environment()
if sys.platform.startswith("linux") and environment.get("GMAT_QT_ALLOW_CORE_DUMPS") != "1":
    # Test crashes retain their exit status and logs without large native core
    # files. Debuggers still work; explicit diagnostic runs can opt in.
    import resource
    _, hard = resource.getrlimit(resource.RLIMIT_CORE)
    resource.setrlimit(resource.RLIMIT_CORE, (0, hard))
if os.name == "nt":
    # Avoid CMake list escaping of Windows PATH and keep the machine unchanged.
    environment["PATH"] = os.pathsep.join([*sys.argv[1:3], environment.get("PATH", "")])
result = subprocess.run(arguments(sys.argv[3:]), env=environment)
if result.returncode:
    print(f"Test process exit: {result.returncode} (0x{result.returncode & 0xffffffff:08x})", file=sys.stderr, flush=True)
sys.exit(result.returncode)
