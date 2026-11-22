import unreal,json,re
from pathlib import Path
root=Path(unreal.Paths.project_dir());base=root/'SourceAssets/UIExact_20261123/Young-and-Dangerous-Priority-UI-Exact'
entries=[]
for p in sorted((base/'buttons').glob('*/*.png')):
 entries.append((p,'T_'+re.sub('[^a-zA-Z0-9_]','_',p.parent.name)+'_'+p.stem))
for p in sorted((base/'animation').glob('*/frame_*.png')):
 entries.append((p,'T_'+re.sub('[^a-zA-Z0-9_]','_',p.parent.name)+'_'+p.stem))
for p in sorted((base/'screens').glob('*/fixed-layout-base.png')):
 entries.append((p,'T_'+re.sub('[^a-zA-Z0-9_]','_',p.parent.name)+'__base'))
tasks=[]
for p,name in entries:
 t=unreal.AssetImportTask();t.filename=str(p);t.destination_path='/Game/KTW/UI/Exact';t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=True;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for _,name in entries:
 a=unreal.load_asset('/Game/KTW/UI/Exact/'+name);assert a,name
 a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);a.set_editor_property('srgb',True);a.set_editor_property('never_stream',True);unreal.EditorAssetLibrary.save_loaded_asset(a)
(root/'Docs/ReferenceUIImport_20261123.json').write_text(json.dumps(dict(status='PASS',additional=len(entries),assets=[n for _,n in entries]),indent=2))
unreal.log('KTW REFERENCE UI IMPORT PASS '+str(len(entries)))
