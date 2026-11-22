import unreal,json
from pathlib import Path
r=Path(unreal.Paths.project_dir())/'SourceAssets/CoverUI/封面+游戏进入按钮序列帧'
files=[('T_Cover',r/'游戏封面英文01.png')]+[(f'T_Press{i}',r/'frames'/f'frame_{i:02d}.png') for i in range(6)]
for name,p in files:
 t=unreal.AssetImportTask();t.filename=str(p);t.destination_path='/Game/KTW/UI/Cover';t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=True;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);a=unreal.load_asset('/Game/KTW/UI/Cover/'+name);assert a;a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);unreal.EditorAssetLibrary.save_loaded_asset(a)
(Path(unreal.Paths.project_dir())/'Docs/CoverUIImport.json').write_text(json.dumps({'status':'PASS','textures':7}))
