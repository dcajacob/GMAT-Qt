"""Exercise the complete menu catalogue with the real MATLAB plugin loaded."""
import hashlib,json,os,re,subprocess,sys,tempfile
from pathlib import Path
startup,plugin,driver,mode=sys.argv[1:]
original=Path(startup).resolve();plugin=Path(plugin).resolve()
assert mode in ['Target','OptimizeAddObjective','OptimizeCapacity'] and plugin.is_file()
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
original_sha=sha(original);plugin_sha=sha(plugin)
with tempfile.TemporaryDirectory(prefix='MatlabCatalogue-') as temp:
    directory=Path(temp);(directory/'output').mkdir()
    settings={'OUTPUT_PATH':str(directory/'output')+'/', 'LOG_FILE':str(directory/'output/GmatLog.txt'),
              'PERSONALIZATION_FILE':str(directory/'MyGmat.ini'), 'MATLAB_MODE':'SHARED'}
    lines=original.read_text().splitlines();changed=set()
    for index,line in enumerate(lines):
        match=re.match(r'^(\w+)\s*=\s*(.*)$',line)
        if not match:continue
        key,value=match.groups();value=value.strip()
        if key in settings:lines[index]=key+' = '+settings[key];changed.add(key)
        elif value.startswith(('../','./')):
            resolved=str((original.parent/value).resolve())
            lines[index]=key+' = '+resolved+('/' if value.endswith('/') else '')
    for key in settings:
        if key not in changed:lines.append(key+' = '+settings[key])
    if not any(re.match(r'^PLUGIN\s*=',line) and 'libMatlabInterface' in line for line in lines):
        lines.append('PLUGIN = '+str(plugin))
    private=directory/'startup.txt';assert not private.exists();private.write_text('\n'.join(lines)+'\n')
    output=Path(os.environ['GMAT_QT_ACCEPTANCE_DIR'])
    (output/'selected-startup.txt').write_bytes(private.read_bytes())
    identity=dict(mode=mode,plugin=str(plugin),plugin_sha256=plugin_sha,original_startup=str(original),
                  original_startup_sha256=original_sha,selected_startup_sha256=sha(private),private_startup_is_symlink=False)
    (output/'plugin-identity.json').write_text(json.dumps(identity,indent=2)+'\n')
    result=subprocess.run([driver,str(private),mode])
    unchanged=sha(original)==original_sha and sha(plugin)==plugin_sha
    (output/'launcher-terminal.json').write_text(json.dumps(dict(returncode=result.returncode,inputs_unchanged=unchanged,original_startup_sha256=sha(original),plugin_sha256=sha(plugin)),indent=2)+'\n')
    assert unchanged
    raise SystemExit(result.returncode)
