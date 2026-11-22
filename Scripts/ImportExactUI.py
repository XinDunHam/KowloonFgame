import unreal,json,re
from pathlib import Path
root=Path(unreal.Paths.project_dir());base=root/'SourceAssets/UIExact_20261123/Young-and-Dangerous-Priority-UI-Exact'
tasks=[];names=[]
for path in sorted((base/'screens').glob('*/parts/*.png')):
 name='T_'+re.sub('[^a-zA-Z0-9_]','_',path.parent.parent.name)+'__'+path.stem
 task=unreal.AssetImportTask();task.filename=str(path);task.destination_path='/Game/KTW/UI/Exact';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;tasks.append(task);names.append(name)
for path in sorted((base/'buttons/04-intermission-shop__buy_gunner').glob('*.png')):
 name='T_04_intermission_shop__buy_gunner_'+path.stem
 task=unreal.AssetImportTask();task.filename=str(path);task.destination_path='/Game/KTW/UI/Exact';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;tasks.append(task);names.append(name)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in names:
 asset=unreal.load_asset('/Game/KTW/UI/Exact/'+name);assert asset,name
 asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);asset.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);asset.set_editor_property('srgb',True);unreal.EditorAssetLibrary.save_loaded_asset(asset)
(root/'Docs/ExactUIImport_20261123.json').write_text(json.dumps(dict(status='PASS',textures=len(names),assets=names),indent=2))
unreal.log('KTW EXACT UI IMPORT PASS '+str(len(names)))
