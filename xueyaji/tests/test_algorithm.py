"""Deterministic signal regressions; these are not medical accuracy tests."""
import ctypes as C
import math
import pathlib
import subprocess
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class Result(C.Structure):
    _fields_ = [('sbp', C.c_float), ('dbp', C.c_float), ('map', C.c_float),
                ('hr', C.c_uint16), ('error', C.c_uint8), ('valid', C.c_bool)]
class AlgorithmTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        target = pathlib.Path(cls.tmp.name) / 'algorithm.so'
        subprocess.run(['gcc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', '-O2',
            *['-I'+str(ROOT/p) for p in ('Core/Inc','Algorithm/Inc','CMSIS')],
            str(ROOT/'Algorithm/Src/bp_algorithm.c'), '-lm', '-o', str(target)], check=True)
        cls.lib = C.CDLL(str(target))
        cls.lib.BP_Algorithm_PushSample.argtypes = [C.c_float, C.c_uint32]
        cls.lib.BP_Algorithm_Calculate.argtypes = [C.POINTER(Result)]
    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()
    def replay(self, hr=72, fixed=None, amp=3, rate=2, duration=60,
               steps=False, blank=False, gap=False, origin=0):
        lib=self.lib
        lib.BP_Algorithm_Reset()
        t=0; last=0; i=0
        while t<duration*1000:
            p=fixed if fixed is not None else 165-rate*(math.floor(t/240)*.24 if steps else t/1000)
            a=amp*math.exp(-((p-100)**2)/(2*22**2))
            signal=p+a*math.sin(2*math.pi*hr/60*t/1000)
            if blank and t-last>=240:
                last=t; lib.BP_Algorithm_NotifyValvePulse(C.c_uint32(round(t+origin)))
            else:
                lib.BP_Algorithm_PushSample(signal, C.c_uint32(round(t+origin)))
            i+=1; t+=130 if gap and i%8==0 else 12.5
        result=Result()
        ok=bool(lib.BP_Algorithm_Calculate(C.byref(result)))
        self.assertEqual(ok, result.valid)
        return result
    def test_heart_rate_range(self):
        for hr in (40,50,60,72,90,120,150,180):
            with self.subTest(hr=hr):
                r=self.replay(hr=hr)
                self.assertTrue(r.valid, r.error)
                self.assertLessEqual(abs(r.hr-hr),2)
                self.assertGreater(r.sbp,r.map)
                self.assertGreater(r.map,r.dbp)
    def test_reject_no_pressure_envelope(self):
        self.assertFalse(self.replay(fixed=100,duration=10).valid)
    def test_reject_no_pulse(self):
        self.assertFalse(self.replay(amp=0).valid)
    def test_reject_incomplete_envelope(self):
        self.assertFalse(self.replay(duration=12).valid)
    def test_valve_step_signal(self):
        r=self.replay(rate=4,duration=30,steps=True,blank=True)
        self.assertTrue(r.valid,r.error)
        self.assertLessEqual(abs(r.hr-72),3)
    def test_valve_steps_without_pulse(self):
        self.assertFalse(self.replay(rate=4,duration=30,steps=True,blank=True,amp=0).valid)
    def test_reject_long_sampling_gaps(self):
        self.assertEqual(self.replay(gap=True).error,9)
    def test_wraparound_timestamp(self):
        r=self.replay(origin=0xFFFFF000)
        self.assertTrue(r.valid,r.error)
        self.assertEqual(r.hr,72)
    def test_reject_nonfinite_input(self):
        self.lib.BP_Algorithm_Reset()
        self.lib.BP_Algorithm_PushSample(float('nan'),0)
        r=Result()
        self.assertFalse(self.lib.BP_Algorithm_Calculate(C.byref(r)))
        self.assertEqual(r.error,9)
    def test_repeated_result_and_reset(self):
        first=self.replay()
        second=Result()
        self.assertTrue(self.lib.BP_Algorithm_Calculate(C.byref(second)))
        self.assertEqual(first.sbp,second.sbp)
        self.lib.BP_Algorithm_Reset()
        self.assertFalse(self.lib.BP_Algorithm_Calculate(C.byref(second)))
        self.assertFalse(second.valid)
if __name__=='__main__':
    unittest.main()
