import unreal,json,traceback
from pathlib import Path
report={'status':'FAIL'}
try:
 tools=unreal.AssetToolsHelpers.get_asset_tools();dest='/Game/KTW/UI';name='M_StreetFog'
 mat=unreal.load_asset(dest+'/'+name) or tools.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
 mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_POST_PROCESS)
 unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
 def node(cls):return unreal.MaterialEditingLibrary.create_material_expression(mat,cls)
 def link(a,out,b,slot):
  if slot=='Input':slot=''
  assert unreal.MaterialEditingLibrary.connect_material_expressions(a,out,b,slot),(a.get_class().get_name(),out,b.get_class().get_name(),slot)
 def scalar(v):n=node(unreal.MaterialExpressionConstant);n.set_editor_property('r',v);return n
 scene=node(unreal.MaterialExpressionSceneTexture);scene.set_editor_property('scene_texture_id',unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
 pos=node(unreal.MaterialExpressionWorldPosition);xy=node(unreal.MaterialExpressionComponentMask);xy.set_editor_property('r',True);xy.set_editor_property('g',True);link(pos,'',xy,'Input')
 add=node(unreal.MaterialExpressionAdd);link(xy,'',add,'A');link(scalar(8000),'',add,'B')
 div=node(unreal.MaterialExpressionDivide);link(add,'',div,'A');link(scalar(16000),'',div,'B')
 clamp=node(unreal.MaterialExpressionSaturate);link(div,'',clamp,'Input')
 tex=node(unreal.MaterialExpressionTextureSampleParameter2D);tex.set_editor_property('parameter_name','Vision');default=unreal.load_asset(dest+'/T_VisionDefault') or unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineResources/DefaultTexture',dest+'/T_VisionDefault');default.set_editor_property('srgb',False);unreal.EditorAssetLibrary.save_loaded_asset(default);tex.set_editor_property('texture',default);tex.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR);link(clamp,'',tex,'UVs')
 dark=node(unreal.MaterialExpressionMultiply);link(scene,'Color',dark,'A');link(scalar(.07),'',dark,'B')
 mix=node(unreal.MaterialExpressionLinearInterpolate);link(dark,'',mix,'A');link(scene,'Color',mix,'B');link(tex,'R',mix,'Alpha')
 unreal.MaterialEditingLibrary.connect_material_property(mix,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat);report={'status':'PASS','asset':mat.get_path_name()}
except Exception:report['error']=traceback.format_exc()
(Path(unreal.Paths.project_dir())/'Docs/StreetFogMaterial.json').write_text(json.dumps(report,indent=2))
unreal.log('STREET_FOG '+json.dumps(report))
