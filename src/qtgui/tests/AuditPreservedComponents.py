"""Audit reviewed legacy components without replaying their GUI executions.

Configuration supplies source-row scopes and exact assertion messages reviewed
by a human/agent. This never discovers or grants coverage from test names alone.
It preserves the original source, receipt and logs; output is a separate,
explicitly post-execution mapping with no current-build qualification.
"""
import argparse
import copy
import datetime
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct

import AcceptanceSnapshot as snapshot
import NasaGuiCoverage as nasa
import RunAcceptance as runner
import ScreenshotRetention


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def assertion(source, message):
    """Retain the complete source assertion, including multiline conditions."""
    quoted = '"'+message+'"'
    assert source.count(quoted) == 1, message
    position = source.index(quoted)
    start = source.rfind('require(', 0, position)
    end = source.find(');', position)
    assert start >= 0 and end >= position, message
    return source[start:end+2]


def audit(root, config, output):
    output.mkdir(parents=True, exist_ok=True)
    inventory = nasa.inventory(root)
    items = {row['key']: row for row in inventory['requirements']}
    keys = {row['key'] for row in config['rows']}
    assert len(keys) == len(config['rows']) and keys <= items.keys()
    assert all(row['scope'] and row['components'] for row in config['rows'])
    reports, summaries, mappings = {}, {}, {}
    metrics = dict(preserved_results=0, native_exposure_events=0,
                   screenshot_references=0, unique_png_files=0, pruned_png_files=0,
                   reviewed_assertions=0, new_gui_executions=0)
    for cohort in config['cohorts']:
        mode = cohort['mode']
        assert mode not in reports and mode in ('visible', 'unattended')
        receipt_path = root/cohort['receipt']
        receipt = json.loads(receipt_path.read_text())
        review_receipt = copy.deepcopy(receipt)
        supplemental_inputs = {}
        manifest = json.loads(Path(receipt['snapshot_manifest']).read_text())
        snapshot.verify(manifest)
        frozen = Path(manifest['root'])
        assert receipt['snapshot_unchanged']
        assert nasa.inventory(frozen)['sources'] == inventory['sources']
        mapping = dict(schema_version=1, sources=inventory['sources'], reviews=[])
        templates, details = {}, []
        for name, component in config['components'].items():
            result = next(row for row in receipt['results'] if row['name'] == component['test'])
            assert result['status'] == 'passed'
            assert runner.junit_status(Path(result['junit']), result['returncode']) == 'passed'
            for artifact in ('log', 'junit'):
                assert sha(result[artifact]) == result[artifact+'_sha256']
            source = component['source']
            assert sha(root/source) == sha(frozen/source) == receipt['source_hashes'][source]
            text = (frozen/source).read_text()
            log_text = Path(result['log']).read_text()
            # Older receipts froze file arguments but did not list every data
            # fixture in source_hashes. Bind only explicitly reviewed inputs,
            # using both the verified snapshot and the exact executed command.
            for path in component.get('input_artifacts', []):
                digest = sha(frozen/path)
                assert digest == manifest['original_hashes'][path]
                commands = [shlex.split(line.split('Test command:', 1)[1])
                            for line in log_text.splitlines() if 'Test command:' in line]
                assert len(commands) == 1 and str(frozen/path) in commands[0]
                existing = receipt['source_hashes'].get(path)
                assert existing is None or existing == digest
                review_receipt['source_hashes'][path] = digest
                supplemental_inputs[path] = dict(sha256=digest, test=result['name'],
                    provenance='verified frozen artifact named by exact executed command')
            lines = [re.sub(r'^\s*\d+:\s?', '', line).strip()
                     for line in log_text.splitlines()]
            markers = [line for line in lines if any(line.startswith(prefix)
                       for prefix in component['terminal_prefixes'])]
            assert len(markers) == component['terminal_count'] and len(set(markers)) == len(markers)
            anchors = [assertion(text, message) for message in component['assertion_messages']]
            claim = dict(test=component['test'], source=source, source_sha256=sha(frozen/source),
                         anchor=component['assertion_messages'][0],
                         facets=['reviewed-exact-terminal-assertions', 'source-bound-component-reuse'],
                         legacy_assertions=dict(terminal_markers=markers, assertion_anchors=anchors),
                         reference_sources={path: sha(frozen/path) for path in
                            component['references']+component.get('input_artifacts', [])})
            for path, digest in claim['reference_sources'].items():
                assert digest == manifest['original_hashes'][path] == review_receipt['source_hashes'][path]
            assert nasa._valid_legacy_assertions(claim['legacy_assertions'], text)
            assert nasa.legacy_assertions_verified(frozen, result, claim)
            templates[name] = claim
            metrics['preserved_results'] += 1
            metrics['reviewed_assertions'] += len(anchors)
            if mode == 'visible':
                assert result['trace']['native_exposure_events'] > 0
                assert result['trace']['recorded_platforms'] == ['xcb']
                metrics['native_exposure_events'] += result['trace']['native_exposure_events']
            images = result['trace']['screenshots']
            metrics['screenshot_references'] += len(images)
            for image in set(images):
                if Path(image).is_file():
                    png = Path(image).read_bytes()
                    assert png[:8] == b'\x89PNG\r\n\x1a\n' and all(struct.unpack('>II', png[16:24]))
                    metrics['unique_png_files'] += 1
                else:
                    retained = ScreenshotRetention.retained_capture(image)
                    assert retained['storage_state'] == 'pruned-after-pass' and not retained['pixels_available']
                    metrics['pruned_png_files'] += 1
            details.append(dict(test=result['name'], source=source, source_sha256=sha(frozen/source),
                                log=result['log'], log_sha256=result['log_sha256'],
                                junit=result['junit'], junit_sha256=result['junit_sha256']))
        for specification in config['rows']:
            claims = []
            for component in specification['components']:
                claim = copy.deepcopy(templates[component])
                claim['scope'] = specification['scope']+' '+config['limits']
                claims.append(claim)
            mapping['reviews'].append(dict(key=specification['key'],
                fingerprint=items[specification['key']]['fingerprint'], applicability='gui',
                rationale=specification['scope']+' '+config['limits'], evidence=claims))
        mapping_path = output/(mode+'-review-mapping.json')
        mapping_path.write_text(json.dumps(mapping, indent=2)+'\n')
        review_receipt['review_only_snapshot_input_bindings'] = supplemental_inputs
        review_receipt['original_receipt'] = str(receipt_path)
        review_receipt['original_receipt_sha256'] = sha(receipt_path)
        review_receipt_path = output/(mode+'-review-receipt.json')
        review_receipt_path.write_text(json.dumps(review_receipt, indent=2)+'\n')
        report = nasa.evaluate(frozen, nasa.inventory(frozen), mapping, review_receipt)
        assert {row['key'] for row in report['requirements'] if row['status']=='verified'} == keys
        assert not any(row['status']=='verified' for row in report['historical_cases'])
        report['execution'].update(mode='post-execution-component-reuse', no_new_execution=True,
            full_current_build_qualified=False, raw_numeric_payloads_reaudited=False,
            limit=config['limits'])
        reports[mode], mappings[mode] = report, mapping
        summaries[mode] = dict(receipt=str(receipt_path), receipt_sha256=sha(receipt_path),
            snapshot_id=manifest['snapshot_id'], mapping_manifest=str(mapping_path),
            mapping_manifest_sha256=sha(mapping_path), details=details)
        summaries[mode].update(review_receipt=str(review_receipt_path),
            review_receipt_sha256=sha(review_receipt_path), supplemental_snapshot_inputs=supplemental_inputs)
    assert 'visible' in reports
    (output/'reviewed-coverage.json').write_text(json.dumps(reports['visible'], indent=2)+'\n')
    pending = copy.deepcopy(mappings['visible']['reviews'])
    for row in pending:
        for claim in row['evidence']:
            claim['reference_sources'] = {path: sha(root/path) for path in claim['reference_sources']}
    (output/'pending-current-rows.json').write_text(json.dumps(pending, indent=2)+'\n')
    summary = dict(audited_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        status=config['label'], versioned_collection=True, mapping_added_after_execution=True,
        mapping_manifest=summaries['visible']['mapping_manifest'],
        mapping_manifest_sha256=summaries['visible']['mapping_manifest_sha256'],
        qualification_receipt_sha256=summaries['visible']['receipt_sha256'],
        source_bound_evidence=summaries, additional_requirement_scopes=sorted(keys),
        additional_historical_scopes=[], new_gui_tests_executed=0, metrics=metrics,
        raw_numeric_payloads_reaudited=False, core_changes=False, gui_changed_files=[],
        scope=config['limits'], full_current_build_qualified=False,
        config_sha256=sha(output/'config.json'))
    (output/'validation-summary.json').write_text(json.dumps(summary, indent=2)+'\n')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    config = json.loads(args.config.read_text())
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output/'config.json').write_text(json.dumps(config, indent=2)+'\n')
    result = audit(root, config, args.output)
    print(json.dumps(dict(scopes=len(result['additional_requirement_scopes']), metrics=result['metrics']), indent=2))
