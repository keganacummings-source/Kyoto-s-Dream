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
assert 'KyotoSpxritFX' in cmake and 'BinaryData' in cmake and 'Source/FxChain.cpp' in cmake
fx=(root/'Source/FxChain.cpp').read_text(encoding='utf-8')
assert fx.count('K::') >= 200, fx.count('K::')
processor=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
ids=re.findall(r'AudioParameter(?:Bool|Float|Int)>\(\"([^\"]+)\"', processor)
assert len(ids) == len(set(ids)), 'duplicate APVTS parameter IDs: '+str([x for x in set(ids) if ids.count(x)>1])
print('OK: 42 themes / 30 modules / native synth+FX targets / community API')
