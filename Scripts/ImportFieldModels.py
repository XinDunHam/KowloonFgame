import unreal,json,traceback
from pathlib import Path
R=Path(unreal.Paths.project_dir())/'SourceAssets/FieldModels_20261121'
DEST='/Game/KTW/GameplayModels'
result={'status':'FAIL','assets':[]}
try:
 tools=unreal.AssetToolsHelpers.get_asset_tools()
 skeleton=unreal.load_asset('/Game/Mannequin/Character/Mesh/UE4_Mannequin_Skeleton')
 rows=json.loads((R/'static_report.json').read_text())
 for x in rows:x['skeletal']=False
 guards=json.loads((R/'Guard/build_report.json').read_text())
 for x in guards:x['skeletal']=True
 for row in rows+guards:
  Source=R/'Guard' if row['skeletal'] else R
  mats=[]
  for entry in row['materials']:
   path=DEST+'/Materials/'+entry['name'];mat=unreal.load_asset(path)
   if not mat:mat=tools.create_asset(entry['name'],DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
   unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
   for semantic,name in entry['textures'].items():
    task=unreal.AssetImportTask();task.filename=str(Source/'Textures'/f'{name}.png');task.destination_path=DEST+'/Textures';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;tools.import_asset_tasks([task])
    tex=unreal.load_asset(DEST+'/Textures/'+name);assert tex,name
    tex.set_editor_property('srgb',semantic=='BaseColor')
    if semantic=='Normal':tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-400,0);node.texture=tex
    node.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if semantic=='Normal' else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR if semantic=='ORM' else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
    if semantic=='BaseColor':unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    elif semantic=='Normal':unreal.MaterialEditingLibrary.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_NORMAL)
    else:
     unreal.MaterialEditingLibrary.connect_material_property(node,'G',unreal.MaterialProperty.MP_ROUGHNESS);unreal.MaterialEditingLibrary.connect_material_property(node,'B',unreal.MaterialProperty.MP_METALLIC)
   mat.set_editor_property('two_sided',True)
   mat.set_editor_property('used_with_skeletal_mesh',True)
   unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat);mats.append(mat)
  name=row['mesh'];task=unreal.AssetImportTask();task.filename=str(Source/'FBX'/f'{name}.fbx');task.destination_path=DEST;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
  ui=unreal.FbxImportUI();ui.automated_import_should_detect_type=False;ui.import_mesh=True;ui.import_as_skeletal=row['skeletal'];ui.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if row['skeletal'] else unreal.FBXImportType.FBXIT_STATIC_MESH;ui.import_materials=False;ui.import_textures=False;ui.import_animations=False;ui.create_physics_asset=False;ui.skeleton=skeleton
  ui.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False);ui.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False)
  task.options=ui;task.factory=unreal.FbxFactory();tools.import_asset_tasks([task])
  mesh=unreal.load_asset(DEST+'/'+name);assert mesh,name
  if row['skeletal']:
   assert mesh.get_editor_property('skeleton')==skeleton,name+' skeleton mismatch'
   slots=mesh.get_editor_property('materials')
   for i,slot in enumerate(slots):slot.material_interface=mats[min(i,len(mats)-1)]
   mesh.set_editor_property('materials',slots);mesh.set_editor_property('physics_asset',unreal.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin_PhysicsAsset'))
  else:
   for i,mat in enumerate(mats):mesh.set_material(i,mat)
  unreal.EditorAssetLibrary.save_loaded_asset(mesh);result['assets'].append(mesh.get_path_name())
 result['status']='PASS'
except Exception:result['error']=traceback.format_exc()
(R/'import_report.json').write_text(json.dumps(result,indent=2))
unreal.log('NPC_IMPORT '+json.dumps(result))
