function GmatQtMatlabJob(requestPath, resultPath)
% Execute work originating in a GMAT command while its optimizer waits.
job = jsondecode(readUtf8(requestPath));
assert(isstruct(job) && isscalar(job) && numel(fieldnames(job)) == 5 && ...
    isfield(job,'session') && ischar(job.session) && isfield(job,'operation') && isfield(job,'name') && isfield(job,'expression') && isfield(job,'value'), ...
    'GMAT:MatlabJob', 'Invalid nested MATLAB request');
reply = struct('found',false,'reserved',false,'value',emptyValue(),'output','');
operation = job.operation;
assert(isscalar(operation) && ismember(operation,0:8), 'GMAT:MatlabJob', 'Unknown nested MATLAB operation');
if operation == 5
    identity = GmatQtOptimizerWorkspace('push',job.session);
    reply.output = matlab.net.base64encode(unicode2native(identity,'UTF-8'));
elseif operation == 6
    GmatQtOptimizerWorkspace('pop',job.session,job.name);
elseif operation == 7 || operation == 8
    writeMatFile(job,operation == 7);
elseif operation == 0
    try
        output = evalc('evalin(''base'',job.expression);');
    catch failure
        % The C Engine reports communication success for a MATLAB language
        % error. RunMatlabString keeps its own status-variable convention.
        output = failure.getReport('extended','hyperlinks','off');
    end
    reply.output = matlab.net.base64encode(unicode2native(output,'UTF-8'));
else
    assert(ischar(job.name) && isvarname(job.name), 'GMAT:MatlabJob', 'Invalid MATLAB workspace name');
    switch operation
        case {1,4}
            reply.found = logical(evalin('base',sprintf('exist(''%s'',''var'')',job.name)));
            if reply.found, reply.value = encodeValue(evalin('base',job.name),0,operation == 4); end
        case 2
            assignin('base',job.name,decodeValue(job.value,0));
        case 3
            if ~evalin('base',sprintf('exist(''%s'',''var'')',job.name))
                assignin('base',job.name,[]);
                reply.reserved = true;
            end
    end
end
writeUtf8(resultPath,jsonencode(reply));
end

function packet = emptyValue()
packet = struct('type',0,'className','unset','rows',0,'columns',0, ...
    'numbers','','text','','fields',{{}},'children',{{}});
end

function packet = encodeValue(value,depth,forceCharacter)
if nargin < 3, forceCharacter = false; end
assert(depth <= 128,'GMAT:MatlabValue','MATLAB value nesting exceeds the callback transport limit');
packet = emptyValue();
packet.className = class(value);
sizes = builtin('size',value); packet.rows = sizes(1); packet.columns = prod(sizes(2:end));
checkDimensions(packet);
if builtin('isobject',value) || (numel(sizes) ~= 2 && ~(forceCharacter && ischar(value))), return; end
if (isnumeric(value) || islogical(value)) && (issparse(value) || ~isreal(value)), return; end
if ischar(value)
    packet.type = 3;
    units = uint16(value(:).');
    index = 1;
    while index <= numel(units)
        if units(index) >= 55296 && units(index) <= 56319
            assert(index < numel(units) && units(index+1) >= 56320 && units(index+1) <= 57343, ...
                'GMAT:MatlabValue','Invalid UTF-16 character data');
            index = index+1;
        else
            assert(units(index) < 56320 || units(index) > 57343, 'GMAT:MatlabValue','Invalid UTF-16 character data');
        end
        index = index+1;
    end
    packet.text = matlab.net.base64encode(unicode2native(value(:).','UTF-8'));
elseif isstruct(value) && isscalar(value)
    packet.type = 4; packet.fields = fieldnames(value).';
    packet.children = cell(1,numel(packet.fields));
    for index = 1:numel(packet.fields)
        packet.children{index} = encodeValue(value.(packet.fields{index}),depth+1);
    end
elseif iscell(value)
    packet.type = 5; packet.children = cell(1,numel(value));
    for index = 1:numel(value), packet.children{index} = encodeValue(value{index},depth+1); end
elseif isnumeric(value) || islogical(value)
    packet.type = 1+double(islogical(value));
    values = double(value(:));
    if computer('endian') == 'B', values = swapbytes(values); end
    packet.numbers = matlab.net.base64encode(typecast(values,'uint8'));
end
end

function value = decodeValue(packet,depth)
assert(depth <= 128 && isstruct(packet) && isscalar(packet) && numel(fieldnames(packet)) == 8, ...
    'GMAT:MatlabValue','Invalid MATLAB value packet');
count = checkDimensions(packet);
children = asCells(packet.children); fields = asCells(packet.fields);
switch packet.type
    case {1,2}
        bytes = matlab.net.base64decode(packet.numbers);
        assert(numel(bytes) == count*8,'GMAT:MatlabValue','Numeric payload does not match dimensions');
        values = typecast(uint8(bytes),'double');
        if computer('endian') == 'B', values = swapbytes(values); end
        if packet.type == 2
            assert(all(values == 0 | values == 1),'GMAT:MatlabValue','Invalid logical payload');
            values = logical(values);
        end
        value = reshape(values,packet.rows,packet.columns);
    case 3
        bytes = matlab.net.base64decode(packet.text);
        value = native2unicode(bytes,'UTF-8'); value = value(:).';
        encoded = unicode2native(value,'UTF-8');
        assert(isequal(uint8(encoded(:)),uint8(bytes(:))) && numel(value) <= intmax('int32'), ...
            'GMAT:MatlabValue','Invalid or oversized UTF-8 character input');
    case 4
        assert(count == 1 && numel(fields) == numel(children) && numel(unique(fields)) == numel(fields), ...
            'GMAT:MatlabValue','Invalid scalar structure fields');
        value = struct();
        for index = 1:numel(fields)
            assert(ischar(fields{index}) && isvarname(fields{index}),'GMAT:MatlabValue','Invalid structure field');
            value.(fields{index}) = decodeValue(children{index},depth+1);
        end
    case 5
        assert(count == numel(children),'GMAT:MatlabValue','Cell payload does not match dimensions');
        value = cell(packet.rows,packet.columns);
        for index = 1:numel(children), value{index} = decodeValue(children{index},depth+1); end
    otherwise
        error('GMAT:MatlabValue','Unsupported typed MATLAB input value');
end
end

function count = checkDimensions(packet)
assert(isscalar(packet.rows) && isscalar(packet.columns) && ...
    isfinite(packet.rows) && isfinite(packet.columns) && ...
    packet.rows >= 0 && packet.columns >= 0 && ...
    packet.rows == fix(packet.rows) && packet.columns == fix(packet.columns), ...
    'GMAT:MatlabValue','Invalid MATLAB value dimensions');
count = packet.rows*packet.columns;
assert(packet.rows <= intmax('int32') && packet.columns <= intmax('int32') && count <= intmax('int32'), ...
    'GMAT:MatlabValue','MATLAB value exceeds native element capacity');
end

function values = asCells(input)
if iscell(input)
    values = input(:).';
elseif isempty(input)
    values = {};
elseif isstruct(input)
    values = arrayfun(@(entry)entry,input(:).','UniformOutput',false);
elseif ischar(input)
    values = {input};
else
    error('GMAT:MatlabValue','Invalid MATLAB value list');
end
end

function text = readUtf8(path)
file = fopen(path,'rb'); assert(file >= 0,'GMAT:MatlabJob','Cannot open nested MATLAB request');
cleanup = onCleanup(@()fclose(file)); %#ok<NASGU>
text = native2unicode(fread(file,Inf,'*uint8').','UTF-8');
end

function writeUtf8(path,text)
file = fopen(path,'wb'); assert(file >= 0,'GMAT:MatlabJob','Cannot create nested MATLAB result');
cleanup = onCleanup(@()fclose(file)); %#ok<NASGU>
bytes = unicode2native(text,'UTF-8');
assert(fwrite(file,bytes,'uint8') == numel(bytes),'GMAT:MatlabJob','Cannot finish nested MATLAB result');
end

function writeMatFile(job,create)
assert(ischar(job.name) && isrow(job.name) && ~isempty(job.name) && ~any(job.name == char(0)), ...
    'GMAT:MatWriter','Invalid MAT-file path');
switch job.expression
    case 'w4', version = '-v4';
    case 'w6', version = '-v6';
    case 'w7', version = '-v7';
    case 'w7.3', version = '-v7.3';
    case 'w', version = '-v7';
    otherwise, error('GMAT:MatWriter','Invalid MAT-file format');
end
options = {version};
if strcmp(job.expression,'w'), options{end+1} = '-nocompression'; end
variables = struct();
if ~create
    assert(~strcmp(version,'-v4'),'GMAT:MatWriter','MAT-file version 4 cannot store structures');
    packet = decodeValue(job.value,0);
    assert(isstruct(packet) && isscalar(packet) && numel(fieldnames(packet)) == 3 && ...
        all(isfield(packet,{'name','prototype','records'})) && ischar(packet.name) && isvarname(packet.name) && ...
        isstruct(packet.prototype) && isscalar(packet.prototype) && iscell(packet.records) && size(packet.records,2) == 1, ...
        'GMAT:MatWriter','Invalid MAT-file record payload');
    fields = fieldnames(packet.prototype);
    values = repmat(packet.prototype,size(packet.records));
    for index = 1:numel(packet.records)
        record = packet.records{index};
        assert(isstruct(record) && isscalar(record) && isequal(fieldnames(record),fields), ...
            'GMAT:MatWriter','MAT-file record fields do not match');
        values(index) = record;
    end
    variables.(packet.name) = values;
    assert(exist(job.name,'file') == 2,'GMAT:MatWriter','MAT output file is no longer present');
    options{end+1} = '-append';
end
% Explicit -mat preserves an exact filename, including one without a suffix.
% Version 6 appends require -v6 as well as -append.
[priorWarning,priorWarningID] = lastwarn;
warningCleanup = onCleanup(@()lastwarn(priorWarning,priorWarningID)); %#ok<NASGU>
lastwarn('','');
save(job.name,'-mat','-struct','variables',options{:});
message = lastwarn;
if ~create && ~isempty(message)
    error('GMAT:MatWriter','MAT-file write reported a warning: %s',message);
end
end
