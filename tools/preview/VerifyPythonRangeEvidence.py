"""Replay saved alpha.5 Python compatibility receipts without launching native code."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent/'releases/0.1.0-alpha.5'


def main():
    release = json.loads((ROOT/'release-receipt.json').read_text())
    versions = ['3.12','3.13','3.14','3.15']
    assert release['status'] == 'passed' and release['python_versions'] == versions
    assert len(release['packages']) == 4
    for package in release['packages']:
        receipt_path = ROOT/package['receipt']
        assert hashlib.sha256(receipt_path.read_bytes()).hexdigest() == package['receipt_sha256']
        receipt = json.loads(receipt_path.read_text())
        assert receipt['status'] == 'passed'
        assert receipt['platform'] == package['platform']
        assert receipt['archive_sha256'] == package['sha256']
        assert receipt['archive_files_unchanged_after_checks']
        assert receipt['gui_without_python']['status'] == 'passed'
        assert list(receipt['python_versions']) == versions
        assert all(receipt[key]['status'] == 'passed' for key in ['package_checks','hermite_checks','event_checks'])
        assert receipt['hermite_checks']['max_position_error_km'] < 2e-5
        assert receipt['hermite_checks']['max_velocity_error_km_s'] < 2e-8
        for version in versions:
            result = receipt['python_versions'][version]
            assert result['status'] == 'passed' and result['mcp']['one_profile']
            assert all(profile['ok'] for profile in result['mcp']['doctor']['profiles'])
            for backend in ['worker','desktop']:
                run = result['mcp'][backend]
                assert run['status'] == 'succeeded'
                assert run['max_position_error_km'] < 2e-5
                assert run['max_velocity_error_km_s'] < 3e-8
            assert result['mcp']['desktop']['mission_edit_committed']
        assert not receipt['full_regression_run']
    sums = {name:digest for digest,name in (line.split() for line in (ROOT/'SHA256SUMS').read_text().splitlines())}
    assert sums == {package['archive']:package['sha256'] for package in release['packages']}
    print('Saved alpha.5 evidence passed for all 16 platform/Python combinations.')


if __name__ == '__main__':
    main()
