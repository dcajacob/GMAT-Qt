function [F, GradF] = GmatQtFminconCall(X, iteration)
% Evaluate the real GMAT optimizer branch through the Qt callback service.
% This protocol has no facility to submit a MATLAB-to-GMAT script.
global GMAT_QT_CALLBACK
global NonLinearEqCon JacNonLinearEqCon NonLinearIneqCon JacNonLinearIneqCon
assert(isstruct(GMAT_QT_CALLBACK) && isfield(GMAT_QT_CALLBACK,'port') && ...
    isfield(GMAT_QT_CALLBACK,'token'), 'GMAT:QtCallbackSession', 'Missing Qt callback session');
assert(isnumeric(X) && isreal(X) && all(isfinite(X(:))), ...
    'GMAT:QtCallbackVariables', 'Callback variables must be finite real numbers');
client = tcpclient('127.0.0.1', GMAT_QT_CALLBACK.port, 'Timeout', 3600);
cleanup = onCleanup(@()delete(client)); %#ok<NASGU>
configureTerminator(client,'LF');
request = struct;
request.token = GMAT_QT_CALLBACK.token;
request.op = 'evaluate';
request.id = 1;
% A cell array preserves the JSON array shape even for one control variable.
request.x = num2cell(X(:).');
if nargin > 1
    request.op = 'iteration';
    request.state = iteration.state;
    request.iteration = iteration.iteration;
    request.funccount = iteration.funccount;
    request.fval = iteration.fval;
end
writeline(client,jsonencode(request));
raw = readline(client);
assert(~isempty(raw), 'GMAT:QtCallbackTimeout', 'No response from the GMAT callback service');
response = jsondecode(char(raw));
assert(isstruct(response) && isfield(response,'ok') && isfield(response,'id') && response.id == request.id, ...
    'GMAT:QtCallbackResponse', 'Invalid callback response identity');
if ~response.ok
    error('GMAT:QtCallbackEvaluation','%s',response.error);
end
F = response.F;
GradF = response.GradF(:);
NonLinearEqCon = response.NonLinearEqCon(:);
NonLinearIneqCon = response.NonLinearIneqCon(:);
JacNonLinearEqCon = response.JacNonLinearEqCon;
JacNonLinearIneqCon = response.JacNonLinearIneqCon;
assert(isscalar(F) && isfinite(F) && all(isfinite(GradF)) && ...
    all(isfinite(NonLinearEqCon)) && all(isfinite(NonLinearIneqCon)), ...
    'GMAT:QtCallbackResult', 'Nonfinite optimizer callback result');
end
