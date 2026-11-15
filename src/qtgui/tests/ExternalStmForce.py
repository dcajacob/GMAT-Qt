"""Deterministic full-state callback for the external-force STM witness."""
import json
import math
from pathlib import Path

ROOT = Path(__file__).parent
EQUATION = json.loads((ROOT/'external-equation.json').read_text())
POSITION = EQUATION['external_position_matrix']
VELOCITY = EQUATION['external_velocity_matrix']
CONSTANT = EQUATION['external_constant']
LABELS = ['X','Y','Z','Vx','Vy','Vz']
JACOBIAN = [[0.0]*6 for _ in range(3)] + [a+b for a,b in zip(POSITION,VELOCITY)]
MATRIX_VALUES = {row+column:JACOBIAN[i][j]
                 for i,row in enumerate(LABELS) for j,column in enumerate(LABELS)}


def GetDerivatives(state, epoch, descriptions, order):
    if len(state) != len(descriptions):
        raise ValueError('State and description counts differ')
    starts = [i for i,name in enumerate(descriptions) if name.endswith('.CartesianState.X')]
    if len(starts) != 2 or len(state) != 84:
        raise ValueError('The full two-spacecraft state is required')
    trace = ROOT/'external-callback-layout.json'
    if not trace.exists():
        trace.write_text(json.dumps(dict(state_count=len(state), description_count=len(descriptions),
            cartesian_starts=starts, descriptions=descriptions), indent=2)+'\n')
    result = [0.0]*len(state)
    for start in starts:
        current = state[start:start+6]
        if len(current) != 6 or not all(math.isfinite(value) for value in current):
            raise ValueError('Missing or invalid assembled Cartesian state')
        radius = math.sqrt(sum(value*value for value in current[:3]))
        if not 100 < radius < 1e6:
            raise ValueError('Missing or invalid assembled Cartesian state')
        acceleration = [sum(POSITION[i][j]*current[j]+VELOCITY[i][j]*current[j+3]
                            for j in range(3))+CONSTANT[i] for i in range(3)]
        result[start+3:start+6] = acceleration
    matrix_indices = {}
    for i,name in enumerate(descriptions):
        if '.STM.' in name:
            result[i] = MATRIX_VALUES[name.rsplit('.',1)[1]]
        elif name.endswith('.AMatrix'):
            owner = name.rsplit('.',1)[0]
            index = matrix_indices.get(owner,0)
            result[i] = JACOBIAN[(index%36)//6][index%6]
            matrix_indices[owner] = index+1
    return result


def BadLength(state, epoch, descriptions, order):
    return [0.0]*(len(state)-1)


def NonFinite(state, epoch, descriptions, order):
    return [float('inf')]+[0.0]*(len(state)-1)


def NonNumeric(state, epoch, descriptions, order):
    return ['invalid']+[0.0]*(len(state)-1)


def Raises(state, epoch, descriptions, order):
    raise ValueError('Deliberate external-force test exception')
