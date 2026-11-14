"""Join source-versioned owner references; preserve parameter factory and science gaps."""
import argparse, collections, copy, gzip, hashlib, io, json, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require
import NasaGuiCoverage as nasa

DIRECT = {'DryMass','Element1','RMAG','C3Energy','ElapsedSecs','A1ModJulian'}
CONFIG = {'Cd','Cr','DragArea','SRPArea','CdSigma','UTCGregorian'}
WRONG_OWNER = {'FuelMass','DutyCycle','Area','Latitude','Longitude','ThrustScaleFactor'}


def digest(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def validate(rows, baseline):
    require(len(rows) == len(baseline) == 566, 'Factory row omitted')
    added = {'retained_owner_parameter_contexts','retained_owner_configuration_contexts','owner_parameter_search_disposition'}
    require([{k:v for k,v in row.items() if k not in added} for row in rows] == baseline, 'Prior role/metadata/context changed')
    joined = {row['factory_type'] for row in rows if row['retained_owner_parameter_contexts']}
    require(joined == DIRECT, 'Direct owner-reference set differs')
    for row in rows:
        require(not row['qualified'] and not row['whole_function_scope_qualified'], 'Unsupported whole-row admission')
        contexts = row['retained_owner_parameter_contexts']
        if not contexts: continue
        require(row['role'] == 'owner-bound-system-parameter' and len(contexts) == 2 and {v['mode'] for v in contexts} == {'headless','visible'}, 'Owner parameter mode/role differs')
        for context in contexts:
            require(context['parameter_type'] == row['factory_type'] and context['metadata_owner'] == row['native_parameter_metadata']['owner_type_name'] and not context['factory_instance_identity_recorded'] and not context['whole_obligation_qualified'] and not context['strict_time_qualified'], 'Unsupported parameter-instance/whole/time claim')
            require(context['actual_owner_class'] == context['metadata_owner'] or (context['actual_owner_class'],context['metadata_owner']) == ('Spacecraft','SpacePoint'), 'Wrong parameter owner')


def review(repo, stage):
    inputs = {}; frozen_sources = {}
    def read(p):
        p = Path(p); p = p if p.is_absolute() else repo/p
        require(p.is_file() and not p.is_symlink(),'Missing regular input '+str(p))
        inputs[str(p)] = digest(p); return p
    def load(p): return json.loads(read(p).read_text())
    previous = repo/'build/nasa-coverage/coverage95-20261103-function-owned-scope'
    require(digest(read(previous/'stage-receipt-v1.json')) == '2e2b11ff7d99bd14059762a2c639c706d39fa6c9b7121ba9a6cf628cb7d5a458', 'Previous owned-scope seal changed')
    prior = load(previous/'stage-receipt-v1.json'); baseline_path = read(previous/'owned-scope-matrix-v1.json'); baseline = load(baseline_path)['rows']
    require(digest(read(previous/'evidence-inputs-v1.json')) == prior['manifest_sha256'] and digest(read(previous/'evidence-audit-v1.json')) == prior['audit_sha256'], 'Prior manifest/audit binding changed')
    for path,value in load(previous/'evidence-inputs-v1.json')['artifact_sha256'].items():
        require(digest(read(path)) == value, 'Prior regular evidence binding changed '+path)
    for suffix in ['function-resource-universe','function-global-resources','function-body-scope','function-navigation']:
        sealed = repo/('build/nasa-coverage/coverage95-20261103-'+suffix)
        receipt = load(sealed/'stage-receipt-v1.json')
        require(digest(read(sealed/'evidence-inputs-v1.json')) == receipt['manifest_sha256'] and digest(read(sealed/'evidence-audit-v1.json')) == receipt['audit_sha256'], 'Retained family seal differs '+suffix)
        for path,value in load(sealed/'evidence-inputs-v1.json')['artifact_sha256'].items():
            require(digest(read(path)) == value, 'Retained family artifact changed '+path)
    current = load(prior['canonical_ledger']); require(digest(read(prior['canonical_ledger'])) == prior['canonical_ledger_sha256'] and current['requirements_still_needed'] == 82 and len(prior['unchanged_prerequisite_holds']) == 59, 'Canonical score/holds changed')
    original = nasa.inventory(repo)
    for n,d in original['sources'].items(): require(digest(read(n)) == d, 'Original authority changed')
    require(original['sources'] == current['authority_sha256'] and len(original['requirements']) == 1158 and len(original['historical_cases']) == 2367, 'Original universe changed')
    rows = copy.deepcopy(baseline); by_name = {v['factory_type']:v for v in rows}
    system = [v for v in rows if v['role'] == 'owner-bound-system-parameter']; unjoined = {v['factory_type'] for v in system if not v['conjunction']['contexts']}
    require(len(system) == 321 and len(unjoined) == 280 and DIRECT <= unjoined, 'Original owner parameter partition differs')
    for row in rows:
        row['retained_owner_parameter_contexts'] = []; row['retained_owner_configuration_contexts'] = []; row['owner_parameter_search_disposition'] = None
    candidates = load(stage/'lexical-search-v2.json')
    require(candidates['status'].startswith('top-level-regular-testcase-gmf') and set(candidates['candidate_parameters']) == DIRECT|CONFIG|WRONG_OWNER|{'OrbitErrorCovariance'} and len(candidates['files']) == 212, 'Bounded lexical candidate set differs')
    for item in candidates['files']:
        p = read(item['path']); relative = p.relative_to(repo/'build/gui-acceptance')
        require(len(relative.parts) == 3 and relative.parts[1].startswith('QtGui.') and digest(p) == item['sha256'], 'Testcase artifact identity differs')
    # Installation/runtime aliases in the broad bootstrap search are not executed
    # testcase witnesses. The refined search retains this limitation explicitly.
    read(stage/'lexical-search-v1.json')

    def native(invocation, test, source, archive, function_file):
        receipt_path = read(repo/'build/gui-acceptance'/invocation/'results.json'); receipt = load(receipt_path)
        require(receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Retained native invocation nonterminal')
        selected = [v for v in receipt['results'] if v['name'] == test]
        require(len(selected) == 1 and selected[0]['status'] == 'passed' and selected[0]['returncode'] == 0,'Retained case did not pass')
        result = selected[0]; log = read(result['log']); case = log.parent
        tests = list(ET.parse(read(case/'ctest.xml')).getroot().iter('testcase'))
        require(len(tests) == 1 and tests[0].get('name') == test and all(tests[0].find(v) is None for v in ['failure','error','skipped']), 'Retained JUnit differs')
        emitted = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
        require(len(emitted) == 1 and emitted[0]['outcome'] == 'passed','Retained native proof absent')
        proof = emitted[0]; proof_path = case/'proof.json'
        if proof_path.is_file(): require(load(proof_path) == proof,'Stored native proof differs')
        else: proof_path = log
        archive = read(archive)
        with tarfile.open(archive) as t:
            member = t.getmember(source); require(member.isfile(),'Nonregular assertion source'); code = t.extractfile(member).read()
        require(hashlib.sha256(code).hexdigest() == receipt['source_hashes'][source], 'Assertion source not bound to original execution')
        frozen_sources[invocation+'/'+Path(source).name] = code
        function = read(case/function_file); text = function.read_text()
        mode = 'visible' if '-visible-' in invocation else 'headless'
        if mode == 'visible':
            require(result['trace']['native_exposure_events'] > 0 and receipt['visible_display']['resizable'] and receipt['visible_display']['start_minimized'] and receipt['max_screenshots'] == 0, 'Retained visible exposure policy differs')
        item = dict(mode=mode,invocation=invocation,test=test,receipt=str(receipt_path),receipt_sha256=digest(receipt_path),proof=str(proof_path),proof_sha256=digest(proof_path),assertion_source=source,assertion_source_sha256=hashlib.sha256(code).hexdigest(),source_capsule=str(archive),source_capsule_sha256=digest(archive),function_file=str(function),function_sha256=digest(function),original_binary_only=True,factory_instance_identity_recorded=False,whole_obligation_qualified=False,strict_time_qualified=False)
        return item,proof,text,case,code.decode()

    def add(parameter,item,owner,reference,kind):
        row = by_name[parameter]; context = dict(item,parameter_type=parameter,metadata_owner=row['native_parameter_metadata']['owner_type_name'],actual_owner_class=owner,reference=reference,kind=kind,limit='Owner-qualified syntax plus passed assertion/raw context;no direct native ParameterFactory-instance identity or whole science/scope qualification.')
        row['retained_owner_parameter_contexts'].append(context)

    universe = repo/'build/nasa-coverage/coverage95-20261103-function-resource-universe'
    for invocation,archive in [('function-resource-universe-pilot-headless-v2','pilot-executed-sources-v2.tar.gz'),('function-resource-universe-visible-v1','visible-executed-sources.tar.gz')]:
        item,proof,text,case,code = native(invocation,'QtGui.NasaFunctionRemainingResourceSpacecraft','src/qtgui/tests/NasaFunctionRemainingResourceTests.cpp',universe/archive,'edited.gmf')
        require(proof['type'] == proof['native_type'] == 'Spacecraft' and proof['field'] == 'DryMass' and proof['missions'] == 3 and proof['function_calls'] == 12 and 'Create Spacecraft PrivateOnly;' in text and 'Write Shared.DryMass ' in text and 'Write PrivateOnly.DryMass ' in text,'Spacecraft owner/property route differs')
        for phase in proof['phases']:
            require(phase['shared_writes'] == [150]*4 and phase['private_writes'] == [275]*3 and phase['shared_value'] == 150 and phase['shadow_value'] == 275 and phase['private_leaked'] is False,'DryMass owner values/scope differ')
        caller = read(case/'caller.script').read_text(); require('Create Spacecraft Shared Shadow;' in caller,'Global owner class differs')
        add('DryMass',item,'Spacecraft','Shared.DryMass / PrivateOnly.DryMass','retained-main-private-global-owner-property-Write')

    global_dir = repo/'build/nasa-coverage/coverage95-20261103-function-global-resources'
    for invocation,archive in [('function-global-recovery-headless-v11','recovery-executed-sources-v11.tar.gz'),('function-global-visible-v1','visible-executed-sources.tar.gz')]:
        item,proof,text,case,code = native(invocation,'QtGui.NasaFunctionGlobalResourceImpulsiveBurn','src/qtgui/tests/NasaFunctionGlobalResourceTests.cpp',global_dir/archive,'edited.gmf')
        require(proof['type'] == 'ImpulsiveBurn' and proof['field'] == 'Element1' and proof['missions'] == 3 and 'Write Shared.Element1 ' in text and 'Create ImpulsiveBurn Shared Decoy;' in read(case/'caller.script').read_text(),'Impulsive owner/reference route differs')
        for phase in proof['phases']:
            require(len(phase['field_writes']) == 4 and all(float(v) == float(proof['first']) for v in phase['field_writes']),'Element1 actual Write values differ')
        add('Element1',item,'ImpulsiveBurn','Shared.Element1','retained-global-owner-property-Write')

    body_dir = repo/'build/nasa-coverage/coverage95-20261103-function-body-scope'
    for invocation,archive in [('function-body-scope-headless-v2','headless-executed-sources-v2.tar.gz'),('function-body-scope-visible-v1','visible-executed-sources-v1.tar.gz')]:
        item,proof,text,case,code = native(invocation,'QtGui.NasaFunctionFrameAutomatic','src/qtgui/tests/NasaFunctionFrameTests.cpp',body_dir/archive,'accept.gmf')
        require(proof['checkpoint'] == 'function-frame-Automatic' and proof['mode'] == 'Automatic' and 'Create Spacecraft local;' in text and 'out = local.Earth.RMAG;' in text and 'energy = local.Earth.C3Energy;' in text and '800000/r' in code,'Recursive private orbit-parameter route differs')
        read(case/'Summary.txt')
        for parameter in ['RMAG','C3Energy']: add(parameter,item,'Spacecraft','local.Earth.'+parameter,'retained-recursive-private-owner-calculation')

    for invocation,archive in [('function-owned-scope-headless-v5','headless-executed-sources-v5.tar.gz'),('function-owned-scope-visible-v1','visible-executed-sources-v1.tar.gz')]:
        item,proof,text,case,code = native(invocation,'QtGui.NasaFunctionOwnedPropSetupStoppedTime','src/qtgui/tests/NasaFunctionOwnedScopeTests.cpp',previous/archive,'edited.gmf')
        require(proof['case'] == 'PropSetupStoppedTime' and proof['stop_time_policy'] == 'measured-endpoint-and-default-one-microsecond-duration-bound' and 'Create Spacecraft FunctionSC;' in text and 'Propagate FunctionOwned(FunctionSC) {FunctionSC.ElapsedSecs = 30};' in text and proof['missions'] == 3,'Elapsed-time local-owner use differs')
        for phase in proof['phases']:
            require(abs(phase['oracle_function_interval_seconds']-30) <= 1e-6 and phase['stopping_time_absolute_bound_seconds'] == 1e-6,'Bounded duration context differs')
        add('ElapsedSecs',item,'Spacecraft','FunctionSC.ElapsedSecs','retained-private-owner-stopping-parameter;strict-time-open')

    navigation = repo/'build/nasa-coverage/coverage95-20261103-function-navigation'
    for invocation,archive in [('function-navigation-headless-v6','executed-sources-v6.tar.gz'),('function-navigation-visible-v1','visible-executed-sources-v7.tar.gz')]:
        item,proof,text,case,code = native(invocation,'QtGui.NasaFunctionNavigationScope','src/qtgui/tests/NasaFunctionNavigationScopeTests.cpp',navigation/archive,'edited.gmf')
        require(proof['native_missions'] == 2 and len(proof['phases']) == 2,'Navigation repeat context missing')
        for index,name in enumerate(['Sat','SimSat','EstSat']): require('Create Spacecraft '+name+';' in text and 'out('+str(index+1)+',1) = '+name+'.A1ModJulian;' in text,'SpacePoint-subclass owner/calculation route differs')
        for phase in proof['phases']:
            require(len(phase['returned']) == 25 and phase['returned'][0] > 21545 and phase['returned'][8] == phase['returned'][16] and set(phase['caller_prototypes']) == {'Sat','SimSat','EstSat'},'Returned owner epochs or caller shadows differ')
        for phase in proof['phases']: read(case/(str(phase['phase'])+'-Caller results Ω.csv'))
        add('A1ModJulian',item,'Spacecraft','Sat.A1ModJulian / SimSat.A1ModJulian / EstSat.A1ModJulian','retained-private-SpacePoint-subclass-epoch-evaluation')
        for parameter in sorted(CONFIG|{'OrbitErrorCovariance'}):
            kind = 'executed-matrix-assignment;no-direct-parameter-value-or-factory-identity-check' if parameter == 'OrbitErrorCovariance' else 'declaration-property-or-disabled-subscriber-reference;not-evaluation-join'
            require(re.search(r'\b(?:SimSat|EstSat)\.'+parameter+r'\b',text),'Recorded weak property context absent')
            if parameter in {'Cd','UTCGregorian'}: require('FilterCdReport.WriteReport = False' in text,'Disabled sink disposition differs')
            by_name[parameter]['retained_owner_configuration_contexts'].append(dict(item,parameter_type=parameter,kind=kind,whole_obligation_qualified=False))
    inheritance = ['src/base/spacecraft/Spacecraft.hpp','src/base/spacecraft/SpaceObject.hpp']
    require('public SpaceObject' in read(inheritance[0]).read_text() and 'public SpacePoint' in read(inheritance[1]).read_text(),'Spacecraft/SpacePoint inheritance differs')
    for context in by_name['A1ModJulian']['retained_owner_parameter_contexts']:
        context['inheritance_evidence'] = 'Current static header chain inspected;these core hashes were not recorded in the selected old navigation source receipts. No old native core/header equivalence or runtime factory-instance identity is inferred.'
    io_source = read('src/qtgui/tests/NasaFunctionIoTests.cpp').read_text()
    require('"Sat.X","Sat.UTCGregorian"' in io_source and 'out = input + 7;' in io_source and 'stamp = epoch;' in io_source, 'Main-property input/formal-value distinction changed')

    dispositions = []
    for name in candidates['candidate_parameters']:
        kind = 'direct-owner-reference-context-joined;factory-instance-gap-retained' if name in DIRECT else 'wrong-owner-name-collision;not-joined' if name in WRONG_OWNER else 'executed-assignment-only;value/factory-gap-retained' if name == 'OrbitErrorCovariance' else 'configuration-or-disabled-output-only;not-joined'
        row = by_name[name]; row['owner_parameter_search_disposition'] = kind
        dispositions.append(dict(parameter=name,metadata_owner=row['native_parameter_metadata']['owner_type_name'],disposition=kind,new_whole_credit=0))
    validate(rows,baseline)
    negatives = []
    for label,change in [('missing-factory-row',lambda x:x.pop()),('false-whole-admission',lambda x:x[0].__setitem__('qualified',True)),('changed-prior-owner-role',lambda x:next(v for v in x if v['factory_type']=='FuelMass').__setitem__('role','excluded')),('missing-parameter-mode',lambda x:next(v for v in x if v['factory_type']=='DryMass')['retained_owner_parameter_contexts'].pop()),('tank-field-as-spacecraft-parameter',lambda x:next(v for v in x if v['factory_type']=='DryMass')['retained_owner_parameter_contexts'][0].__setitem__('actual_owner_class','ChemicalTank')),('false-factory-instance-identity',lambda x:next(v for v in x if v['factory_type']=='DryMass')['retained_owner_parameter_contexts'][0].__setitem__('factory_instance_identity_recorded',True)),('false-strict-time-credit',lambda x:next(v for v in x if v['factory_type']=='ElapsedSecs')['retained_owner_parameter_contexts'][0].__setitem__('strict_time_qualified',True))]:
        bad = copy.deepcopy(rows); change(bad)
        try: validate(bad,baseline)
        except (ValueError,AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt owner join '+label)
    remaining = [v for v in system if v['factory_type'] not in DIRECT and not v['conjunction']['contexts']]
    require(len(remaining) == 274,'Remaining owner parameter set changed')
    with (stage/'retained-assertion-sources-v1.tar.gz').open('wb') as raw:
        with gzip.GzipFile(filename='',fileobj=raw,mode='wb',mtime=0) as compressed:
            with tarfile.open(fileobj=compressed,mode='w') as t:
                for n,b in sorted(frozen_sources.items()):
                    info = tarfile.TarInfo(n);info.size=len(b);t.addfile(info,io.BytesIO(b))
    source_ids = {n:hashlib.sha256(b).hexdigest() for n,b in sorted(frozen_sources.items())}
    (stage/'retained-assertion-identities-v1.json').write_text(json.dumps(source_ids,indent=2)+'\n')
    matrix = dict(factory_entries=566,qualified_keys=[],baseline=str(baseline_path),baseline_sha256=digest(baseline_path),rows=rows,limits=['All prior fields,321 parameters and metadata remain.','47 named types have bounded owner-reference contexts;no native ParameterFactory-instance identity or whole science/scope qualification is inferred.','274 need stronger function-use context search or new tests;seven weak configuration/assignment cases remain among them.'])
    (stage/'parameter-owner-matrix-v1.json').write_text(json.dumps(matrix,indent=2)+'\n')
    worklist = dict(status='remaining-context-work;not-exclusion',remaining_count=274,all_system_parameter_count=321,all_system_parameters_whole_scope_open=True,additional_gaps='The 47 bounded reference-context names remain in the complete matrix with factory-instance,owner/domain and whole-scope/science limitations. This 274-entry search queue does not close or exclude them.',by_owner=dict(collections.Counter(v['native_parameter_metadata']['owner_type_name'] for v in remaining)),rows=[dict(factory_type=v['factory_type'],metadata=v['native_parameter_metadata'],retained_weak_contexts=v['retained_owner_configuration_contexts'],original_qualified=False) for v in remaining])
    (stage/'remaining-parameter-owner-worklist-v1.json').write_text(json.dumps(worklist,indent=2)+'\n')
    (stage/'candidate-dispositions-v1.json').write_text(json.dumps(dict(candidates=dispositions,qualified_keys=[]),indent=2)+'\n')
    (stage/'retained-inputs-v1.json').write_text(json.dumps(dict(artifact_sha256=dict(sorted(inputs.items()))),indent=2)+'\n')
    return dict(status='passed-retained-owner-reference-join;factory-instance-and-whole-scope-open',qualified_keys=[],factory_entries=566,system_parameters=321,previous_vector_component_types=41,new_owner_reference_types=sorted(DIRECT),bounded_owner_reference_types=47,remaining_without_joined_function_use_context=274,retained_GUI_passes_reused=10,new_GUI_runs=0,parameter_mode_contexts=12,configuration_or_assignment_only_types=7,wrong_owner_name_collisions=6,negative_controls=negatives,original_binary_only=True,old_numerical_oracles_recomputed=False,old_navigation_core_header_hashes_recorded=False,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    result = review(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
