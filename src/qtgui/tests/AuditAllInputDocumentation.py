"""Audit the complete bounded Linux Qt All Others!E12 documentation contract.

This checks source-versioned field evidence, reviewed parametric input rules,
active documentation and the installed rendered guide. It neither executes GUI
scenarios nor qualifies numerical truth, other platforms or a complete binary.
"""
import argparse
import gzip
import hashlib
import json
import re
import tarfile
from pathlib import Path
from lxml import etree, html
import NasaGuiCoverage as nasa
from AuditShippedInputDocumentation import active_sources, enabled, audit as guide_audit
from AuditConfiguredInputDocumentation import audit as configured_audit
from AuditNestedInputDocumentation import audit as nested_audit
from AuditControlFieldDomains import audit as domains_audit
from AuditInputUnits import inventory as guide_inventory
from InputConstructionInventory import sources as construction_sources

KEY = 'All Others!E12'
FINGERPRINT = '73808d8c00d2e8717e7f567559b3fd1a6e722478fa03f55e0ff559268e4523ec'
TEXT = 'The system shall have user-documentation that describes the units of all input fields.'
NS = {'d': 'http://docbook.org/ns/docbook'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def identity(row):
    return row['type'], row['field'], row.get('original_observed_GUI_unit', row.get('observed_GUI_unit'))


def canonical_unit(value):
    aliases = {'meters':'m','Kilometers':'km','Seconds':'s','sec':'s','sec.':'s','deg.':'deg','Deg.':'deg',
               'GMAT A1 modified Julian days':'A1ModJulian','Range Units (RU)':'RU','Percent/Year':'percent/year','Percent':'percent'}
    value = aliases.get(value.strip(), value.strip()).replace('Kg','kg').replace('kw','kW').replace('/sec','/s').replace('kg-m','kg*m')
    value = re.sub(r'kPa\^?([CK]\d+)', r'kPa^\1', value)
    return re.sub(r'(km|kPa|m|s|AU)([23])(?=$|[/ *])', r'\1^\2', value)


def rendered_guide(repo, active):
    flat = lambda e: re.sub(r'\s+', '', ''.join(e.itertext()))
    generated = repo / 'doc/help/html'
    installed = repo / 'application/docs/help/html'
    cache, checks, policies, conventions = {}, [], [], []
    def page(topic):
        if topic not in cache:
            require((generated / topic).is_file(), 'Missing generated topic: ' + topic)
            cache[topic] = html.parse(str(generated / topic))
        return cache[topic]
    for path, root in active.items():
        topic = root.get('{http://www.w3.org/XML/1998/namespace}id')
        for index, row in enumerate(root.findall('.//d:row', NS)):
            if not all(enabled(n) for n in [row, *row.iterancestors()]):
                continue
            unit = row.find('.//d:varlistentry[d:term="Units"]/d:listitem', NS)
            entry = row.find('d:entry', NS)
            if unit is None or entry is None:
                continue
            entry = etree.fromstring(etree.tostring(entry))
            for term in entry.findall('.//d:indexterm', NS):
                term.getparent().remove(term)
            require(bool(topic), 'Unit row lacks a rendered topic')
            found = False
            for tr in page(topic + '.html').xpath('//tr[td]'):
                if flat(tr[0]) != flat(entry):
                    continue
                units = [u for u in tr.xpath('.//tr[td]') if len(u) > 1 and flat(u[0]) == 'Units']
                found = found or any(flat(u[1]) == flat(unit) for u in units)
            require(found, 'Unit row missing from rendered help: ' + str(path) + ':' + str(index))
            checks.append(dict(source=str(path.relative_to(repo)), row_index=index, field=flat(entry), units=flat(unit), topic=topic))
            if path.name == 'Resource_QtInputConventions.xml':
                conventions.append(flat(entry))
        # Qt-specific controls and scalar conventions use unit-policy sections,
        # not native field-table rows. Check every such policy in rendered help.
        if path.name in {'Resource_QtControlUnits.xml', 'Resource_QtInputConventions.xml'}:
            for section in root.findall('.//d:refsection', NS):
                unit = section.find('./d:variablelist/d:varlistentry[d:term="Units"]/d:listitem/d:para', NS)
                if unit is None:
                    continue
                require(any(flat(p) == flat(unit) for p in page(topic + '.html').xpath('//p')), 'Unit policy absent from rendered guide')
                policies.append(section.get('{http://www.w3.org/XML/1998/namespace}id'))
    from collections import deque
    from urllib.parse import urlsplit, unquote
    pending, reachable, links = deque(['index.html']), set(), []
    while pending:
        name = pending.popleft()
        if name in reachable:
            continue
        reachable.add(name)
        for href in page(name).xpath('//a/@href|//head/link/@href'):
            url = urlsplit(href)
            if url.scheme or url.netloc:
                continue
            target = unquote(url.path) or name
            if '/' in target or not target.endswith('.html'):
                continue
            links.append((name, target, unquote(url.fragment)))
            if target not in reachable:
                pending.append(target)
    anchors = {name: set(page(name).xpath('//@id|//@name')) for name in reachable}
    require(not [(a,b,c) for a,b,c in links if b not in anchors or (c and c not in anchors[b])], 'Broken local guide link')
    for name in reachable:
        require(sha(generated / name) == sha(installed / name), 'Installed help differs: ' + name)
    require(sha(repo / 'doc/help/help.html') == sha(repo / 'application/docs/help/help.html'), 'Installed single-page help differs')
    return dict(unit_rows=len(checks), unit_policy_sections=len(policies), supplemental_convention_rows=len(conventions), rows=checks, policies=policies, convention_fields=conventions,
                reachable_pages=len(reachable), local_links_checked=len(links), installed_matches_generated=True,
                installed_page_sha256={name:sha(installed/name) for name in sorted(reachable)},
                installed_single_page_sha256=sha(repo/'application/docs/help/help.html'))


def source_snapshot(repo, root):
    before = load(root / 'fourteenth-increment-before.json')
    files = {repo/p for p in before if p.startswith(('src/base/','src/gmatutil/','plugins/')) and Path(p).suffix in ('.cpp','.hpp','.h')}
    compiled, cmake = construction_sources(repo)
    files.update(compiled + [cmake])
    files.update(active_sources(repo))
    files.update(p for p in (repo/'src/qtgui/tests').iterdir() if p.suffix in ('.py','.json'))
    for directory in ('doc/help/html','application/docs/help/html'):
        files.update((repo/directory).glob('*.html'))
    files.update(repo/p for p in ('doc/help/help.html','application/docs/help/help.html'))
    return {str(p.relative_to(repo)):sha(p) for p in sorted(files)}


def audit(repo, root):
    inputs_before = source_snapshot(repo, root)
    authorities = nasa.inventory(repo)
    authority = next(row for row in authorities['requirements'] if row['key'] == KEY)
    require(authority['fingerprint'] == FINGERPRINT and authority['text'] == TEXT, 'Changed E12 authority')
    sealed = {}
    for phase in ['fourteenth','fifteenth','sixteenth','seventeenth','eighteenth','nineteenth','twentieth','twenty-first']:
        report_path = root / (phase + '-increment-audit.json')
        report = load(report_path)
        require(report['status'] == 'passed', 'Unpassed evidence increment: ' + phase)
        archive_path = root / (phase + '-increment-sources.tar.gz')
        require(sha(archive_path) == report['source_archive_sha256'], 'Changed frozen source archive: ' + phase)
        with tarfile.open(archive_path) as archive:
            for name, digest in report['source_sha256'].items():
                require(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest, 'Changed frozen text source: ' + name)
        for name, digest in report.get('report_sha256', {}).items():
            require(sha(root / name) == digest, 'Changed sealed evidence report: ' + name)
        sealed[phase] = dict(audit_sha256=sha(report_path), source_archive_sha256=sha(archive_path))
    receipt = load(root / 'input-field-census/receipt.json')
    require(receipt['inputs_unchanged'] and len(receipt['results']) == 84, 'Incomplete original census')
    reconstructed, catalogue = set(), None
    for result in receipt['results']:
        path = Path(result['compressed_output'])
        raw = gzip.decompress(path.read_bytes())
        require(result['status'] == 'completed' and sha(path) == result['compressed_sha256'] and hashlib.sha256(raw).hexdigest() == result['output_sha256'], 'Changed original raw census')
        require(sha(result['log']) == result['log_sha256'], 'Changed original census log')
        data = json.loads(raw)
        if data['requested'] == '--catalog':
            catalogue = data
            continue
        states = [data['default_GUI_fields']] + [v['GUI_fields'] for v in data['single_selector_variants'] if 'GUI_fields' in v]
        reconstructed.update((data['requested'], f['field'], f['unit']) for fields in states for f in fields)
    crosswalk = load(root / 'input-documentation-crosswalk.json')
    require(len(reconstructed) == 987 and reconstructed == {identity(f) for f in crosswalk['fields']}, 'Original field identities were lost')
    require(catalogue is not None and len(catalogue['resource_types']) == 83, 'Missing original factory universe')
    current = load(root / 'current-factory-scope/receipt.json')
    require(current['returncode'] == 0 and current['inputs_unchanged'] and current['resource_editor_constructions'] == 0, 'Unverified current factory scope')
    for path, digest in current['inputs_sha256'].items():
        require(sha(path) == digest, 'Current factory-query input drift')
    raw = gzip.decompress((root / 'current-factory-scope/catalogue.json.gz').read_bytes())
    require(sha(root/'current-factory-scope/catalogue.json.gz') == current['compressed_sha256'] and hashlib.sha256(raw).hexdigest() == current['uncompressed_sha256'], 'Changed current catalogue')
    now = json.loads(raw)
    require(now['resource_types'] == catalogue['resource_types'] and now['nested_factory_types'] == catalogue['nested_factory_types'], 'Uninventoried active factory type')
    # Source-schema lineage: these two native changes are the separately tested
    # Precision correction and normalized solar unit strings, not new factories.
    before = load(root/'fourteenth-increment-before.json')
    native = {p:h for p,h in before.items() if p.startswith(('src/base/','src/gmatutil/','plugins/')) and Path(p).suffix in ('.cpp','.hpp','.h')}
    require(len(native) == 1703, 'Native source snapshot incomplete')
    changes = {p for p,h in native.items() if sha(repo/p) != h}
    require(changes == {'src/base/subscriber/MessageWindow.cpp','src/base/hardware/SolarPowerSystem.cpp'}, 'Unreviewed native schema/source change')
    for phase in ['fifteenth','seventeenth']:
        phase_report = load(root/(phase+'-increment-audit.json'))
        for name in changes & set(phase_report['source_sha256']):
            require(sha(repo/name) == phase_report['source_sha256'][name], 'Metadata repair changed after review')
    active = active_sources(repo)
    routes = guide_audit(repo, crosswalk)
    missing = {(f['type'], f['field']) for f in routes['fields'] if not f['shipped_documentation_candidates']}
    require(routes['summary']['active_guide_route_candidates'] == 984 and missing == {('MessageWindow','Dec'),('String','Object'),('Variable','Object')}, 'Unreviewed original field route gap')
    semantic = load(root/'nineteenth-unit-semantic-dispositions.json')
    require(len(semantic['records']) == 987 and {identity(f) for f in semantic['records']} == reconstructed and not any(f['review_status'] == 'field-meaning-review-open' for f in semantic['records']), 'Open original field meanings')
    meanings = load(root/'remaining-field-meaning-review.json')
    require(len(meanings['records']) == 634 and len(meanings['policies']) == 54, 'Incomplete remaining-field review')
    for policy in meanings['policies'].values():
        for path,digest in policy['source_sha256'].items():
            require(sha(repo/path) == digest, 'Changed field-meaning source: '+path)
    by_identity = {identity(f):f for f in routes['fields']}
    explicit = load(root/'explicit-native-guide-unit-agreement.json')
    require(len(explicit['records']) == 156, 'Incomplete explicit-unit agreement review')
    for row in explicit['records']:
        docs = by_identity[(row['type'],row['field'],row['native_unit'])]['shipped_documentation_candidates']
        require(any(canonical_unit(d['units']) == row['canonical_unit'] for d in docs), 'Current guide unit disagreement')
    require(canonical_unit('s/kPa^K7') != canonical_unit('s/kPaC7'), 'Exponent-symbol distinction lost')
    # Three preserved anomalous records have concrete, source-backed dispositions.
    for typ in ['String','Variable']:
        raw = json.loads(gzip.decompress((root/'input-field-census'/(typ+'.json.gz')).read_bytes()))
        names = {c['object_name'] for c in raw['default_GUI_controls']}
        require('parameterValue' in names and 'resource_Object' not in names, 'Scalar Object incorrectly treated as an input')
    message = load(root/'input-field-census-message-window/receipt.json')
    data = gzip.decompress((root/'input-field-census-message-window/MessageWindow.json.gz').read_bytes())
    require(message['returncode'] == 0 and message['inputs_unchanged'] and message['Precision']['parameter_type'] == 'Integer' and 'Precision' in message['GUI_fields'], 'Precision replacement lacks evidence')
    require(sha(root/'input-field-census-message-window/MessageWindow.json.gz') == message['compressed_sha256'] and hashlib.sha256(data).hexdigest() == message['output_sha256'], 'Precision evidence drift')
    precision = guide_audit(repo,dict(fields=[dict(type='MessageWindow',field='Precision')]))
    require(precision['summary']['active_guide_route_candidates'] == 1, 'Precision has no user unit description')
    solar = load(root/'solar-unit-metadata/receipt.json')
    require(solar['returncode'] == 0 and solar['inputs_unchanged'] and len(solar['changes']) == 5 and solar['all_other_default_field_metadata_preserved'], 'Missing solar unit replacement evidence')
    require(sha(root/'solar-unit-metadata/SolarPowerSystem.json.gz') == solar['current_compressed_output_sha256'], 'Solar raw evidence drift')
    configured = configured_audit(repo,root/'configured-input-census')
    nested = nested_audit(repo,root/'nested-input-census')
    domains = domains_audit(repo,root/'control-field-domain-review.json')
    require(configured['states_reviewed'] == 315 and configured['field_unit_pairs_reviewed'] == 72 and nested['scenes'] == 11 and nested['actual_controls'] == 177, 'Incomplete configured/nested input evidence')
    require(domains['families'] == 34 and domains['construction_templates'] == 343, 'Incomplete concrete/parametric source domains')
    metadata = guide_inventory(repo)
    active_names = {str(p.relative_to(repo)) for p in active}
    require(not metadata['parse_errors'] and all(f['source'] == 'doc/help/src/Command_Template.xml' and f['source'] not in active_names for f in metadata['gui_unit_review_queue']), 'Active GUI field lacks a Units description')
    rendered = rendered_guide(repo,active)
    require(rendered['unit_rows'] == 1243 and rendered['unit_policy_sections'] == 34 and rendered['supplemental_convention_rows'] == 9 and rendered['reachable_pages'] == 275 and rendered['local_links_checked'] == 5643, 'Incomplete rendered documentation universe')
    source_hashes = source_snapshot(repo, root)
    require(inputs_before == source_hashes, 'Documentation audit source/rendered-input drift')
    require(nasa.inventory(repo)['sources'] == authorities['sources'], 'Authority changed during audit')
    return dict(status='passed',contract_kind='input-documentation',key=KEY,fingerprint=FINGERPRINT,requirement_text=TEXT,
                scope='All applicable input-field unit documentation in the audited Linux Qt source/runtime/guide versions, using explicit native, configured and parametric field domains. This documentary contract does not assert scientific truth or full current-binary/platform/NASA qualification.',
                checks=dict(authority_exact=True,frozen_evidence_verified=True,active_factory_scope_equal=True,native_schema_lineage_reviewed=True,original_987_identities_retained=True,original_semantic_reviews_closed=True,three_corrupt_or_nonrendered_records_explicitly_disposed=True,solar_and_Precision_current_replacements_verified=True,configured_modes_complete_within_declared_domains=True,all_34_source_domains_bound=True,active_GUI_field_metadata_complete=True,all_1243_rendered_unit_rows_verified=True,all_34_non_table_policies_and_9_convention_rows_rendered=True,all_reachable_installed_pages_match=True),
                frozen_increments=sealed,original_field_records=987,original_active_routes=984,original_explicit_metadata_dispositions=3,active_factory_types=83,configured_states=315,configured_field_unit_pairs=72,nested_scenes=11,nested_controls=177,native_field_rows_additionally_reviewed=33,source_families=34,construction_templates=343,source_schema_changes=sorted(changes),guide_metadata_summary=metadata['summary'],inactive_template_rows=metadata['gui_unit_review_queue'],rendered=rendered,
                authority_source_sha256=authorities['sources'],source_sha256=source_hashes,qualified_keys=[KEY],all_input_documentation_qualified=True,source_inputs_unchanged=True,new_GUI_acceptance_replays=0,new_mission_executions=0,full_current_build_qualified=False,full_NASA_qualified=False,technical_core_approval=False,
                limits=['Requirement credit is documentary and explicitly source-versioned; it is not a newly executed full GUI/binary suite.','The old exploratory census remains a selected-input/available-library evidence version; newer loaded-image receipts do not retroactively strengthen it.','Arbitrary new source/plugins or changed field domains require a new audit.','User-defined script/function/variable/array expressions use their documented receiving-field/function or user-assigned numerical conventions.','The separate scientific, optional-interface, authority, OS/GPU and core-approval gates remain open.'])


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--evidence-root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    result=audit(args.repo.resolve(),args.evidence_root.resolve())
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',KEY+':',result['rendered']['unit_rows'],'rendered unit rows,34 unit-policy sections,9 supplemental convention rows,34 source domains; documentary scope only.')
