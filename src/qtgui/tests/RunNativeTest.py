"""Keep native CTest tests on private software X11 unless visibility is requested."""
import os
import shutil
import subprocess
import sys
import tempfile
from AcceptanceRuntime import arguments


def main():
    xvfb, *command = sys.argv[1:]
    command = arguments(command)
    if os.environ.get('GMAT_QT_ACCEPTANCE_MODE') != 'visible':
        command = [xvfb, '-a', *command]
    elif not os.environ.get('DISPLAY'):
        raise SystemExit('Visible native tests require your existing desktop DISPLAY')
    # Keep the execution mode truthful: private X11 is native, not visible.
    # Only this child opts in; ordinary widget tests still default offscreen.
    child_environment = dict(os.environ, GMAT_QT_NATIVE_TEST_LAUNCH='1')
    if os.environ.get('GMAT_QT_CAPTURE_CHILD_OUTPUT') == '1':
        # Some external runtimes spawn detached services that inherit stdio.
        # A regular file lets CTest observe the native client's actual exit
        # without waiting for an unrelated service to close its output pipe.
        with tempfile.TemporaryFile() as output:
            code = subprocess.call(command, env=child_environment,
                                   stdout=output, stderr=subprocess.STDOUT)
            output.seek(0)
            shutil.copyfileobj(output, sys.stdout.buffer)
            sys.stdout.buffer.flush()
            return code
    return subprocess.call(command, env=child_environment)


if __name__ == '__main__':
    sys.exit(main())
