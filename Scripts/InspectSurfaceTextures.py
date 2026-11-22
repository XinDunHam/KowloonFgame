import unreal
from pathlib import Path
out=Path(unreal.Paths.project_dir())/'SourceAssets/ReferenceHD_20261123/SurfaceInspection';out.mkdir(parents=True,exist_ok=True)
for name in ['T_Concrete','T_floor__PBR_StoA_Diffuse_001','T_tile_2__PBR_StoA_Diffuse','T_road1__PBR_StoA_Diffuse']:
 asset=unreal.load_asset('/Game/Abandoned_Hong-Kong/Textures/'+name)
 if not asset:continue
 task=unreal.AssetExportTask();task.object=asset;task.filename=str(out/(name+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterPNG();assert unreal.Exporter.run_asset_export_task(task),name
