"""Run new GmatFunction/Barycenter/LibrationPoint automatic identities."""
import argparse, hashlib, json, subprocess, sys, uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible', action='store_true')
    parser.add_argument('--filter')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    identity = uuid.uuid4().hex[:12]
    stage = repo / 'build/gui-acceptance'
    stage.mkdir(exist_ok=True)
    refs = {'src/qtgui/CMakeLists.txt', 'src/qtgui/tests/NasaFunctionBuiltinTests.cpp', 'src/qtgui/tests/NasaFunctionBuiltinCases.json', 'src/qtgui/tests/NasaFunctionBuiltinScopeTests.cpp', 'src/qtgui/tests/NasaFunctionBuiltinScopeCases.json.in', 'build/linux-gui/src/qtgui/NasaFunctionBuiltinScopeCases.json', 'src/qtgui/tests/function-builtin-scope/private-endpoints.oem', 'src/qtgui/tests/function-builtin-scope/caller-endpoints.oem'}
    refs.update({'src/qtgui/tests/GetLastStateBindingTests.cpp', 'src/qtgui/tests/AuditFunctionBuiltinScope.py', 'src/qtgui/tests/NasaFunctionBuiltinAutomaticGlobalTests.cpp', 'src/qtgui/tests/PrepareFunctionBuiltinAutomaticGlobal.py', 'src/qtgui/tests/NasaFunctionBuiltinAutomaticGlobalCases.json.in', 'build/linux-gui/src/qtgui/NasaFunctionBuiltinAutomaticGlobalCases.json'})
    for base in ['src/base', 'src/gmatutil', 'plugins']:
        refs.update(str(p.relative_to(repo)) for p in (repo / base).rglob('*') if p.is_file() and p.suffix in {'.cpp', '.hpp'})
    refs.update({'src/qtgui/tests/NasaFunctionAutomaticGmatHelperTests.cpp','src/qtgui/tests/NasaFunctionAutomaticReferenceTests.cpp','src/qtgui/tests/NasaFunctionAutomaticGmatHelperCases.json','src/qtgui/tests/NasaFunctionAutomaticReferenceCases.json','src/qtgui/tests/NasaFunctionGlobalResourceTests.cpp','src/qtgui/tests/NasaFunctionReferenceCases.json','src/qtgui/tests/FormationAcceptance.hpp'})
    review = stage / ('function-automatic-resources-' + identity + '-review.json')
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'], reviews=[], reference_sources={n: hashlib.sha256((repo / n).read_bytes()).hexdigest() for n in sorted(refs)}), indent=2) + '\n')
    command = [sys.executable, str(repo / 'src/qtgui/tests/RunAcceptance.py'), '--filter', args.filter or '^QtGui.NasaFunctionAutoResource', '--max-screenshots', '0', '--max-checkpoints', '400', '--pause-ms', '0', '--nasa-review', str(review.relative_to(repo)), '--output', str(args.output or stage / ('function-automatic-resources-' + identity))]
    if args.visible:
        command += ['--visible', '--start-minimized']
    return subprocess.call(command, cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
