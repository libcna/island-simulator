"""Incident-specific offline GDB helper for PID423089, not a game/reproduction launcher.
Requires symbol files with the documented matching build IDs. Never calls inferior code.
"""
import gdb, struct, re, json, math

# Read only bytes actually retained in PT_LOAD segments of the core; never let GDB
# silently substitute a current file-backed page when comparing loaded build IDs.
info = gdb.execute('info files', to_string=True)
match = re.search(r"Local core dump file:\s*`([^']+)'", info)
if not match:
    raise RuntimeError('Cannot locate offline core file')
core = open(match.group(1), 'rb')
header = core.read(64)
if header[:6] != b'\x7fELF\x02\x01':
    raise RuntimeError('Expected ELF64 little-endian core')
phoff = struct.unpack_from('<Q', header, 32)[0]
phsize, phnum = struct.unpack_from('<HH', header, 54)
segments = []
for i in range(phnum):
    core.seek(phoff+i*phsize)
    fields = struct.unpack('<IIQQQQQQ', core.read(56))
    if fields[0] == 1:
        segments.append((fields[3], fields[5], fields[2]))
def retained(address, size):
    for start, length, offset in segments:
        if start <= address and address+size <= start+length:
            core.seek(offset+address-start)
            data = core.read(size)
            if len(data) == size:
                return data
    raise RuntimeError('Requested memory not retained in core: '+hex(address))
def build_id(base):
    header = retained(base, 64)
    if header[:6] != b'\x7fELF\x02\x01':
        raise RuntimeError('Invalid loaded ELF header')
    phoff = struct.unpack_from('<Q', header, 32)[0]
    phsize, phnum = struct.unpack_from('<HH', header, 54)
    for i in range(phnum):
        fields = struct.unpack('<IIQQQQQQ', retained(base+phoff+i*phsize, 56))
        if fields[0] != 4:
            continue
        notes = retained(base+fields[3], fields[5])
        cursor = 0
        while cursor+12 <= len(notes):
            namesz, descsz, typ = struct.unpack_from('<III', notes, cursor)
            cursor += 12
            name = notes[cursor:cursor+namesz]
            cursor += (namesz+3)&~3
            desc = notes[cursor:cursor+descsz]
            cursor += (descsz+3)&~3
            if name.rstrip(b'\0') == b'GNU' and typ == 3:
                return desc.hex()
    raise RuntimeError('No retained GNU build ID')
expected_ids = {'game':'e37224286aeef8884283b5f2231b5983fc2231cd',
                'Gallium':'f34ab849e878d8ffbbb15533a597ca31ebd583ec',
                'CNA':'f8dc807d0df965e4dfb6881e3cae4b1269f70acf',
                'SDL3':'c25c94cd74bd8f6413a1ab65fa4f5e662ab16689'}
for name, base in [('game',0x562694e5e000),('Gallium',0x7f7044800000),
                   ('CNA',0x7f7056800000),('SDL3',0x7f7056400000)]:
    try:
        actual_id = build_id(base)
        print('CORE_BUILD_ID', name, actual_id)
        if actual_id != expected_ids[name]:
            raise RuntimeError('Wrong core image for this incident helper')
    except Exception as error:
        print('CORE_BUILD_ID_UNAVAILABLE', name, str(error))
        raise
core.close()
gdb.execute('thread 2', to_string=True)
gdb.execute('frame 20', to_string=True)
gdb.execute('set print static-members off')
for expression in ['game.cameraPosition_', 'game.renderCameraPosition_',
                   'game.flightMotion_.position_', 'game.flightMotion_.active_',
                   'game.drawnFrames_', 'game.yaw_', 'game.pitch_',
                   'game.cameraView_', 'game.cameraProjection_']:
    try:
        print('CORE_GAME_STATE', expression, gdb.parse_and_eval(expression))
    except Exception as error:
        print('CORE_GAME_STATE_UNAVAILABLE', expression, str(error))
def pointee(expression, typename):
    value = str(gdb.parse_and_eval(expression))
    found = re.search(r'_M_head_impl = (0x[0-9a-f]+)', value)
    if not found:
        raise RuntimeError('Cannot decode retained unique_ptr: '+expression)
    return gdb.parse_and_eval('*(('+typename+'*)'+found.group(1)+')')
try:
    p = gdb.parse_and_eval('game.flightMotion_.position_')
    x,y,z = (float(p[k]) for k in ('x','y','z'))
    terrain = pointee('game.flightMotion_.surface_', 'island::RenderedTerrainSurface')
    heights = terrain['heights_']['_M_elems']
    gx,gz = (x+340)*240/680,(z+340)*240/680
    col,row = max(0,min(239,math.floor(gx))),max(0,min(239,math.floor(gz)))
    tx,tz = gx-col,gz-row
    tl,tr,bl,br = (float(heights[i]) for i in (row*241+col,row*241+col+1,(row+1)*241+col,(row+1)*241+col+1))
    ground = tl+(tr-tl)*tx+(bl-tl)*tz if tx+tz<=1 else br+(bl-br)*(1-tx)+(tr-br)*(1-tz)
    print('CORE_FLIGHT_ENVELOPE', json.dumps({'radius':math.hypot(x,z),'ground':ground,'above_ground':y-ground}))
except Exception as error:
    print('CORE_FLIGHT_ENVELOPE_UNAVAILABLE', str(error))
for member, typename, fields in [
    ('grassRenderer_', 'GrassRenderer', ['visiblePatchCount_','visibleNearPatchCount_','visibleMiddlePatchCount_','visibleFarPatchCount_','drawCallCount_','customShaderReady_']),
    ('monkeyRenderer_', 'MonkeyRenderer', ['visibleMonkeyCount_','drawCallCount_']),
    ('contactShadowRenderer_', 'ContactShadowRenderer', ['visibleShadowCount_','primitiveCount_','drawCallCount_'])]:
    try:
        obj = pointee('game.'+member, 'island::'+typename)
        print('CORE_RENDER_COUNTS', typename, json.dumps({name:int(obj[name]) for name in fields}))
    except Exception as error:
        print('CORE_RENDER_COUNTS_UNAVAILABLE', typename, str(error))
