"""Terminal observation contract; no display, GI, app or compositor is launched."""
import subprocess
import io
import time
import unittest
from unittest.mock import Mock, patch
from IsolatedWaylandSession import PrivateWorker


class ExitObservationTests(unittest.TestCase):
    def worker(self, *, windows=None, code=0):
        worker = PrivateWorker.__new__(PrivateWorker)
        worker.deadline = time.monotonic() + 30
        app = Mock(pid=123); app.wait.return_value = code
        compositor = Mock(); compositor.poll.return_value = None
        worker.processes = [('compositor', compositor), ('application', app)]
        worker.windows = Mock(return_value=windows if windows is not None else [
            {'pid': 123, 'mapped': True, 'focus': True, 'title': 'Untitled — GMAT Qt 6'}])
        worker.key = Mock()
        return worker, app, compositor

    def test_normal_exit_is_observed_before_cleanup(self):
        worker, app, _ = self.worker()
        result = worker.exit_application()
        worker.key.assert_called_once_with('ALT+F4')
        self.assertEqual(result['application_exit'], 0)
        self.assertEqual(result['other_process_exits_before_cleanup'], {'compositor': None})
        self.assertLessEqual(app.wait.call_args.kwargs['timeout'], 5)

    def test_unfocused_foreign_or_modal_window_cannot_receive_close(self):
        for windows in ([],
            [{'pid': 124, 'mapped': True, 'focus': True, 'title': 'GMAT Qt 6'}],
            [{'pid': 123, 'mapped': True, 'focus': False, 'title': 'GMAT Qt 6'}],
            [{'pid': 123, 'mapped': True, 'focus': True, 'title': 'Unapplied changes'}],
            [{'pid': 123, 'mapped': True, 'focus': True, 'title': 'GMAT Qt 6'}] * 2):
            worker, _, _ = self.worker(windows=windows)
            with self.subTest(windows=windows), self.assertRaises(ValueError): worker.exit_application()
            worker.key.assert_not_called()

    def test_timeout_and_signal_exit_remain_failures(self):
        worker, app, _ = self.worker()
        app.wait.side_effect = subprocess.TimeoutExpired('GmatQt', 5)
        with self.assertRaisesRegex(RuntimeError, 'within bound'): worker.exit_application()
        for code in (-15, -11, 1):
            worker, _, _ = self.worker(code=code)
            with self.subTest(code=code), self.assertRaisesRegex(RuntimeError, 'abnormally'): worker.exit_application()

    def test_compositor_failure_cannot_be_promoted_to_pass(self):
        worker, _, compositor = self.worker()
        compositor.poll.return_value = -11
        with self.assertRaisesRegex(RuntimeError, 'Private service exited'): worker.exit_application()

    def test_only_explicit_successful_terminal_action_ends_protocol(self):
        worker, _, _ = self.worker()
        worker.alive = Mock(); worker.config = {'seconds': 30}
        worker.exit_application = Mock(return_value={'application_exit': 0})
        with patch('builtins.open', return_value=io.StringIO()):
            result = worker.action({'action': 'exit_application'}, 7)
        self.assertTrue(result['quit'])
        self.assertEqual(result['action'], 'exit_application')
        self.assertEqual(result['result']['application_exit'], 0)

    def test_unexpected_application_exit_still_fails_alive_check(self):
        worker, app, _ = self.worker()
        worker.closed = False; app.poll.return_value = 0
        with self.assertRaisesRegex(RuntimeError, 'Owned application exited'):
            PrivateWorker.alive(worker)


if __name__ == '__main__': unittest.main()
