"""Shared opt-in runtime isolation for CTest and the acceptance runner."""
import os
from pathlib import Path
import re


def environment():
    env = dict(os.environ)
    if env.get('GMAT_QT_ACCEPTANCE_MODE') == 'visible':
        env['QT_QPA_PLATFORM'] = 'windows' if os.name == 'nt' else 'xcb'
    else:
        env['QT_QPA_PLATFORM'] = 'offscreen'
    return env


def arguments(command):
    """Replace only the configured startup argument, leaving test fixtures alone."""
    original = os.environ.get('GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL')
    private = os.environ.get('GMAT_QT_ACCEPTANCE_STARTUP')
    if not original or not private:
        return command
    return [private if os.path.normcase(os.path.abspath(x)) == os.path.normcase(original) else x
            for x in command]


def isolate_startup(original, directory, path_mapping=None):
    original, directory = Path(original).resolve(), Path(directory).resolve()
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'output'
    output.mkdir(exist_ok=True)
    # Existing scenarios also locate shipped fixtures as startup/../data,
    # startup/../samples, etc. Preserve that installation layout without copying
    # large kernels or redirecting their ../output into the user's installation.
    installation = original.parent.parent
    for child in installation.iterdir():
        if not child.is_dir() or child.name in ('bin', 'output'):
            continue
        alias = directory.parent / child.name
        if not alias.exists():
            alias.symlink_to(child, target_is_directory=True)
    private_output = directory.parent / 'output'
    if not private_output.exists():
        private_output.symlink_to(output, target_is_directory=True)
    # Path-settings acceptance launches GmatQt beside its startup file. Keep
    # launchers/binaries/libraries available there, while retaining our private
    # startup and personalization files.
    for child in original.parent.iterdir():
        if not child.is_file() or child.name.startswith('gmat_startup') or child.name in ('MyGmat.ini', 'GmatLog.txt'):
            continue
        alias = directory / child.name
        if not alias.exists():
            alias.symlink_to(child)
    text = original.read_text(encoding='utf-8')
    if path_mapping:
        # This changes only the per-case startup, never the archived input.
        text = text.replace(str(path_mapping[0]), str(path_mapping[1]))
    replacements = {'ROOT_PATH': str(installation) + '/',
                    'OUTPUT_PATH': str(output) + '/',
                    'LOG_FILE': str(output / 'GmatLog.txt'),
                    'PERSONALIZATION_FILE': str(directory / 'MyGmat.ini')}
    for key, value in replacements.items():
        pattern = r'^' + key + r'\s*=.*$'
        assignment = key + ' = ' + value
        if re.search(pattern, text, re.M):
            text = re.sub(pattern, lambda _: assignment, text, flags=re.M)
        else:
            text += '\n' + assignment + '\n'

    def absolute_literal(match):
        prefix, value = match.group(1), match.group(2).strip()
        if value.startswith(('../', './')):
            value = str((original.parent / value).resolve())
        return prefix + value

    text = re.sub(r'^(\w+\s*=\s*)([^\n]*)$', absolute_literal, text, flags=re.M)
    selected = directory / 'startup.txt'
    selected.write_text(text, encoding='utf-8')
    return selected
