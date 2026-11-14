"""Closed-form final and trial circles for solver-only OEM acceptance.

The control U defines radius 7000+100U km. Targeting U=2 or minimizing
(U-2)^2 gives the same independently known 7200 km final circular orbit.
No GMAT report or library supplies expected ephemeris states.
"""
import json
import math
from pathlib import Path
import EphemerisSamplingOracle as circle


def state(time, control):
    radius = 7000.+100.*control
    rate = math.sqrt(circle.MU/radius**3)
    speed = math.sqrt(circle.MU/radius)
    angle = rate*time
    return [radius*math.cos(angle),radius*math.sin(angle),0.,
            -speed*math.sin(angle),speed*math.cos(angle),0.]


def fixtures():
    cases=[]
    for key in ['Target','Optimize']:
        target=key=='Target';solver='DC'if target else'Opt'
        mission=(key+" 'Final solution only' "+solver+
                 ' {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n'+
                 'Vary '+solver+'(U = 0, {Perturbation = 0.0001, Lower = -2, Upper = 4, MaxStep = 1});\n'+
                 'Radius = 7000 + 100*U;\n'+
                 'Sat.X = Radius;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\n'+
                 'Sat.VY = sqrt(398600.4415/Radius);\nSat.VZ = 0;\n'+
                 "Propagate 'Trial or final circle' PropA(Sat) {Sat.ElapsedSecs = 120};\n"+
                 'Report Trials U Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y '+
                 'Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n')
        mission+=('Achieve DC(U = 2, {Tolerance = 0.0000000001});\nEndTarget;\n'if target else
                  'Cost = (U-2)^2;\nMinimize Opt(Cost);\nEndOptimize;\n')
        case=circle.fixture('RawSteps')
        case.update(key=key,duration=120,mission=mission,expected_segments=[[[t]+state(t,2.)for t in range(0,121,5)]],
                    expected_final=[120]+state(120,2.)+[9],solver=solver,solver_type='DifferentialCorrector'if target else'Yukon',
                    final_control=2.,final_radius=7200.,control_tolerance=1e-10)
        cases.append(case)
    return dict(cases=cases,scope='Finite Target/DifferentialCorrector and Optimize/Yukon SaveAndContinue OEM final-solution membership. '
                'All 25 epochs and six Cartesian components match the prescribed final 7200 km circle; '
                'raw intermediate/perturbation trials must occur and remain distinguishable. '
                'No estimation/filter/smoother, SPK/Code500/STK, nested/multiple solvers, failure/stop/RunInitialGuess, '
                'arbitrary trajectories/frames/force science, native OS input/hardware/platform/stress/current-private NASA '
                'or full current-build qualification. FRR!E406 remains uncredited until the broader obligation is qualified.')


if __name__=='__main__':
    path=Path(__file__).with_name('NasaSolverEphemerisCases.json')
    path.write_text(json.dumps(fixtures(),indent=2)+'\n')
    print(path)
