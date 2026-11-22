"""Use the project's own PBR road textures; retain material paths and all gameplay geometry."""
from pathlib import Path
import unreal,shutil
root=Path(unreal.Paths.project_dir());backup=root/'SourceAssets/ReferenceHD_20261123/OriginalRoadMaterials';backup.mkdir(parents=True,exist_ok=True)
lib=unreal.MaterialEditingLibrary
names=['M_Street','M_UpperRoad','M_MainRoad','M_LowerRoad','M_LinkAlley']
for name in names:
 src=root/'Content/KTW/Materials'/f'{name}.uasset'
 if not (backup/src.name).exists():shutil.copy2(src,backup/src.name)
 mat=unreal.load_asset('/Game/KTW/Materials/'+name);assert mat,name
 lib.delete_all_material_expressions(mat);mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
 position=lib.create_material_expression(mat,unreal.MaterialExpressionWorldPosition,-700,-200)
 # WorldPosition exposes named XYZ/XY/Z outputs in UE 5.8.
 scale=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,-360,-200);scale.set_editor_property('const_b',.001)
 assert lib.connect_material_expressions(position,'XY',scale,'A'), 'World XY connection failed'
 texture=unreal.load_asset('/Game/Abandoned_Hong-Kong/Textures/T_Concrete');assert texture
 node=lib.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-150,0);node.set_editor_property('texture',texture)
 assert lib.connect_material_expressions(scale,'',node,'UVs')
 tint=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,0,180);tint.set_editor_property('constant',unreal.LinearColor(.10,.125,.15,1))
 color=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply,180,0)
 assert lib.connect_material_expressions(node,'RGB',color,'A');assert lib.connect_material_expressions(tint,'',color,'B')
 assert lib.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant,180,220);rough.set_editor_property('r',.84)
 assert lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('KTW HD WORLD MATERIALS PASS 5 physically lit roads, verified concrete texture and physically lit roughness')

