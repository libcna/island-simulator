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
# coredumpctl initially opens the recorded executable path, which may now be rebuilt.
# Caller must select the preserved candidate with `file`; reloading the core then
# restores shared-library symbols and relocation using that selected executable.
candidate = gdb.current_progspace().filename
if not candidate or not candidate.endswith('/island-simulator.candidate'):
    raise RuntimeError('Select the preserved island-simulator.candidate before this helper')
gdb.execute('core-file '+match.group(1), to_string=True)
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

# CPU upload shadows are retained separately from GPU memory. Check them without
# invoking GetData or any method on the dead process; this cannot verify GPU residency.
def object_from_unique(value, typename):
    address = re.search(r'_M_head_impl = (0x[0-9a-f]+)', str(value))
    if not address:
        raise RuntimeError('Missing retained buffer pointer')
    return gdb.parse_and_eval('*(('+typename+'*)'+address.group(1)+')')
def shadow_bytes(buffer):
    impl = buffer['cpuShadow_']['_M_impl']
    start, finish = int(impl['_M_start']), int(impl['_M_finish'])
    if not 0 <= finish-start <= 16*1024*1024:
        raise RuntimeError('Invalid or excessive CPU buffer shadow size')
    return retained(start,finish-start) if finish>start else b''
try:
    grass = pointee('game.grassRenderer_', 'island::GrassRenderer')
    mesh_reports = []
    for name in ('nearMesh_','middleMesh_','farMesh_'):
        mesh = grass[name]
        vb = object_from_unique(mesh['vertices'], 'Microsoft::Xna::Framework::Graphics::VertexBuffer')
        ib = object_from_unique(mesh['indices'], 'Microsoft::Xna::Framework::Graphics::IndexBuffer')
        vertices, indices = shadow_bytes(vb), shadow_bytes(ib)
        count, stride = int(vb['vertexCount_']), int(vb['vertexDeclaration_']['vertexStride_'])
        index_count = int(ib['indexCount_'])
        if int(ib['indexElementSize_']) != 0:
            raise RuntimeError('Expected 16-bit grass mesh indices')
        decoded = struct.unpack('<'+'H'*(len(indices)//2), indices)
        positions_finite = all(math.isfinite(v) for i in range(count)
                               for v in struct.unpack_from('<fff',vertices,i*stride))
        valid = (len(vertices)==count*stride and len(indices)==index_count*2
                 and count==int(mesh['vertexCount']) and index_count==int(mesh['primitiveCount'])*3
                 and max(decoded)<count and positions_finite)
        mesh_reports.append({'mesh':name,'vertices':count,'stride':stride,'indices':index_count,
                             'max_index':max(decoded),'valid':valid})
    cells, patches, invalid = 0,0,[]
    cell_array = grass['cells_']['_M_elems']
    first_cell,last_cell = cell_array.type.strip_typedefs().range()
    for i in range(first_cell,last_cell+1):
        cell = cell_array[i]
        if not bool(cell['occupied']):
            continue
        vb = object_from_unique(cell['instances'], 'Microsoft::Xna::Framework::Graphics::VertexBuffer')
        data = shadow_bytes(vb)
        count, stride = int(vb['vertexCount_']), int(vb['vertexDeclaration_']['vertexStride_'])
        patch_count = int(cell['patchCount'])
        valid = (count==patch_count and stride==44 and len(data)==count*stride
                 and all(math.isfinite(v) for (v,) in struct.iter_unpack('<f',data)))
        if not valid:
            invalid.append(i)
        cells += 1
        patches += patch_count
    print('CORE_GRASS_UPLOAD_AUDIT',json.dumps({'meshes':mesh_reports,'array_cells':last_cell-first_cell+1,
          'occupied_cells':cells,'patches':patches,'reported_patches':int(grass['totalPatchCount_']),
          'total_matches':patches==int(grass['totalPatchCount_']),
          'invalid_cells':invalid,'scope':'CPU upload shadows, not GPU descriptors'}))
except Exception as error:
    print('CORE_GRASS_UPLOAD_AUDIT_UNAVAILABLE',str(error))
core.close()
