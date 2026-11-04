"""Inventory compiled Qt input-construction templates, without qualification.

This is a fail-closed source worklist. A construction site is not an exposed
field, and a table/editor template may instantiate many fields or only outputs.
"""
import argparse,hashlib,json,re
from collections import Counter
from pathlib import Path

CONTROL_TYPES=set('QLineEdit QComboBox QSpinBox QDoubleSpinBox QCheckBox QRadioButton QPlainTextEdit QTextEdit QTextBrowser QTableWidget QTreeWidget QListWidget QTableView QTreeView QListView QColumnView QDateEdit QDateTimeEdit QTimeEdit QSlider QDial QKeySequenceEdit'.split())
DIALOG_TYPES={'QInputDialog','QFileDialog','QColorDialog','QFontDialog'}
# Containers, actions, output labels and item records are not independent input
# fields. Editable table/list items are represented by their containing template.
NON_FIELD_TYPES=set('QAction QDialog QDialogButtonBox QDockWidget QFile QFormLayout QGridLayout QGroupBox QHBoxLayout QLabel QListWidgetItem QMdiArea QOpenGLWidget QPushButton QRubberBand QScrollArea QShortcut QSplitter QStackedWidget QTabBar QTabWidget QTableWidgetItem QTimer QToolBar QToolButton QTreeWidgetItem QVBoxLayout QWidget'.split())
NONCODE=re.compile(r'R"(?P<delimiter>[^()\s]*)\(.*?\)(?P=delimiter)"|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
def code_only(text):
 return NONCODE.sub(lambda m:re.sub(r'[^\n]',' ',m.group()),text)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def sources(repo):
 directory=repo/'src/qtgui';cmake=directory/'CMakeLists.txt';raw=cmake.read_text()
 if re.search(r'\btarget_sources\s*\(\s*GmatQtFrontend\b',raw):raise ValueError('Review additional frontend target_sources before inventory')
 match=re.search(r'add_library\(GmatQtFrontend\s+STATIC\s+(.*?)\)\s*\n',raw,re.S)
 if not match:raise ValueError('Expected explicit static frontend source list')
 tokens=match.group(1).split()
 if any('$' in token or not re.fullmatch(r'[A-Za-z0-9_./-]+\.(cpp|hpp|h)',token) for token in tokens):raise ValueError('Review nonliteral frontend source list')
 pending=[directory/token for token in tokens]+[directory/'main.cpp'];result=set()
 while pending:
  path=pending.pop().resolve()
  if path in result:continue
  if not path.is_file():raise ValueError('Missing compiled/local source: '+str(path))
  result.add(path)
  for name in re.findall(r'^\s*#include\s+"([^"\n]+)"',path.read_text(),re.M):
   target=(path.parent/name).resolve()
   if target.is_relative_to(directory.resolve()) and target.is_file():pending.append(target)
 return sorted(result),cmake

def inventory(repo):
 files,cmake=sources(repo);texts={p:p.read_text() for p in files};code={p:code_only(s) for p,s in texts.items()};types=set(CONTROL_TYPES);inheritance={}
 classes=[(m.group(1),m.group(2),p) for p,s in code.items() for m in re.finditer(r'\bclass\s+(\w+)(?:\s+final)?\s*:\s*public\s+(\w+)',s)]
 changed=True
 while changed:
  changed=False
  for child,parent,p in classes:
   if parent in types and child not in types:types.add(child);inheritance[child]=dict(base=parent,source=str(p.relative_to(repo)));changed=True
 unknown={name for text in code.values() for name in re.findall(r'\bnew\s+(Q[A-Z]\w*)\b',text)}-types-DIALOG_TYPES-NON_FIELD_TYPES
 if unknown:raise ValueError('Review unclassified Qt allocation types: '+str(sorted(unknown)))
 pattern=re.compile(r'\bnew\s+(?P<heap>'+ '|'.join(sorted(types|DIALOG_TYPES,key=len,reverse=True))+r')\b|\b(?P<static>Q(?:Input|File|Color|Font)Dialog::get\w+)\s*\(|\b(?P<stack>'+ '|'.join(sorted(types|DIALOG_TYPES,key=len,reverse=True))+r')\s+\w+\s*[({;]')
 records=[]
 for path in files:
  raw=texts[path];clean=code[path];ordinals=Counter()
  for m in pattern.finditer(clean):
   name=m.group('heap') or m.group('static') or m.group('stack');kind='heap' if m.group('heap') else 'static-dialog' if m.group('static') else 'stack-dialog';line=clean.count('\n',0,m.start())+1;source_line=raw.splitlines()[line-1].strip();ordinals[(kind,name)]+=1
   records.append(dict(key=str(path.relative_to(repo))+':'+kind+':'+name+':'+str(ordinals[(kind,name)]),source=str(path.relative_to(repo)),line=line,constructor=name,allocation=kind,constructor_ordinal=ordinals[(kind,name)],source_line=source_line,source_line_sha256=hashlib.sha256(source_line.encode()).hexdigest(),source_sha256=sha(path)))
 return dict(scope='Compiled source input/control construction templates, including locally derived text editors and direct standard dialogs. No field-exposure, semantic-unit or all-input qualification.',summary=dict(compiled_and_local_source_files=len(files),cpp_files=sum(p.suffix=='.cpp' for p in files),header_files=sum(p.suffix!='.cpp' for p in files),construction_templates=len(records),source_files_with_templates=len({r['source'] for r in records}),local_control_subclasses=len(inheritance)),control_subclasses=inheritance,records=records,source_sha256={str(p.relative_to(repo)):sha(p) for p in files+[cmake]},qualified_keys=[],all_input_documentation_qualified=False)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=inventory(a.repo.resolve());a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r['summary']))
