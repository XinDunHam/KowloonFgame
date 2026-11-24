import unreal
from pathlib import Path
p=Path(unreal.Paths.project_dir())/'SourceAssets/HomeUI_20261124/Reference.png'
t=unreal.AssetImportTask();t.filename=str(p);t.destination_path='/Game/KTW/UI/Home';t.destination_name='T_MenuReference';t.automated=True;t.save=True;t.replace_existing=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);a=unreal.load_asset('/Game/KTW/UI/Home/T_MenuReference');assert a
a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);a.set_editor_property('never_stream',True);unreal.EditorAssetLibrary.save_loaded_asset(a)
unreal.log('KTW HOME IMPORT PASS')
