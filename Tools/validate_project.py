import json, pathlib, re, sys
root=pathlib.Path(__file__).resolve().parents[1]
themes=json.loads((root/'Resources/themes/themes.json').read_text(encoding='utf-8'))['themes']
assert len(themes)==42, len(themes)
assert len(list((root/'Resources/themes').glob('*.html')))==42
registry=(root/'Source/ModuleRegistry.cpp').read_text(encoding='utf-8')
assert registry.count('{"dream') == 30, registry.count('{"dream')
features=(root/'Source/FeatureNames.h').read_text(encoding='utf-8')
assert features.count('"') >= 400
worker=(root/'WORKER_DREAMSHARE.js').read_text(encoding='utf-8')
for needle in ['community_publish','community-instrument:','community-instruments-v1','DREAMSHARE_KV']:
    assert needle in worker, needle
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
assert 'KyotoSpxritFX' in cmake and 'BinaryData' in cmake
print('OK: 42 themes / 30 modules / native synth+FX targets / community API')
