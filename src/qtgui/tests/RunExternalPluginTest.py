"""Launch a test-only external plug-in through a private startup file."""
import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
from AcceptanceRuntime import isolate_startup

startup,plugin,driver,mode=sys.argv[1:]
plugin=Path(plugin).resolve();original=Path(startup).resolve()
assert mode in ('Enabled','Absent','Missing') and plugin.is_file()
with tempfile.TemporaryDirectory(prefix='ExternalAcceptance-') as temporary:
    private=isolate_startup(original,Path(temporary)/'runtime')
    # GMAT adds the platform suffix itself; preserve any lib prefix.
    stem=plugin.with_suffix('')
    selected=str(stem) if mode=='Enabled' else str(stem.parent/'MissingAcceptanceFixture')
    text=private.read_text()
    assert 'AcceptanceProbe' not in text and str(stem) not in text
    if mode!='Absent':text+='\nPLUGIN = '+selected+'\n'
    private.write_text(text)
    output=Path(os.environ['GMAT_QT_ACCEPTANCE_DIR']) if os.environ.get('GMAT_QT_ACCEPTANCE_DIR') else Path(temporary)/'evidence'
    output.mkdir(exist_ok=True,parents=True)
    (output/'selected-startup.txt').write_text(text)
    identity=dict(mode=mode,plugin=str(plugin),plugin_sha256=hashlib.sha256(plugin.read_bytes()).hexdigest(),plugin_bytes=plugin.stat().st_size,selected_plugin_stem=selected if mode!='Absent' else None,original_startup_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),selected_startup_sha256=hashlib.sha256(private.read_bytes()).hexdigest(),plugin_linked_into_gui=False)
    (output/'plugin-identity.json').write_text(json.dumps(identity,indent=2)+'\n')
    result=subprocess.run([driver,str(private),mode],env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(output)))
    assert hashlib.sha256(plugin.read_bytes()).hexdigest()==identity['plugin_sha256']
    assert hashlib.sha256(original.read_bytes()).hexdigest()==identity['original_startup_sha256']
    sys.exit(result.returncode)
