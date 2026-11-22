$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$artRoot=Join-Path (Split-Path $PSScriptRoot -Parent) 'SourceAssets/StreetArt'
New-Item -ItemType Directory -Path $artRoot -Force | Out-Null
$rng=[Random]::new(48181)
function Texture($name,$r,$g,$b,$roof=$false,$facade=$false){
 $bmp=[Drawing.Bitmap]::new(512,512);$gfx=[Drawing.Graphics]::FromImage($bmp)
 for($x=0;$x -lt 512;$x+=4){for($y=0;$y -lt 512;$y+=4){$j=$rng.Next(-13,14);$brush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb([Math]::Clamp($r+$j,0,255),[Math]::Clamp($g+$j,0,255),[Math]::Clamp($b+$j,0,255)));$gfx.FillRectangle($brush,$x,$y,4,4);$brush.Dispose()}}
 $pen=[Drawing.Pen]::new([Drawing.Color]::FromArgb(65,23,25,25),2)
 if($facade){for($y=0;$y -lt 512;$y+=64){$gfx.DrawLine($pen,0,$y,512,$y)};for($x=0;$x -lt 512;$x+=128){$gfx.DrawLine($pen,$x,0,$x,512)}}
 if($roof){$metal=[Drawing.Pen]::new([Drawing.Color]::FromArgb(92,117,114,105),8);$gfx.DrawRectangle($metal,30,30,452,452);$gfx.DrawLine($metal,30,245,480,245);$metal.Dispose()}
 $bmp.Save((Join-Path $artRoot ($name+'.png')),[Drawing.Imaging.ImageFormat]::Png);$pen.Dispose();$gfx.Dispose();$bmp.Dispose()
}
Texture Road 41 47 49
Texture Facade 155 143 119 -facade $true
Texture FacadeWarm 171 143 104 -facade $true
Texture FacadeGreen 103 135 128 -facade $true
Texture Roof 63 70 70 -roof $true
Texture Awning 130 43 32 -facade $true
Texture Shutter 91 103 107 -facade $true
$names=@('九龍茶餐廳','永發五金','廟街冰室','龍記燒臘','旺角押記','金城電器','霓虹酒吧','海景酒家')
for($i=0;$i -lt 8;$i++){
 $bmp=[Drawing.Bitmap]::new(512,256);$gfx=[Drawing.Graphics]::FromImage($bmp);$gfx.Clear([Drawing.Color]::FromArgb(12,24,26));$gfx.TextRenderingHint=[Drawing.Text.TextRenderingHint]::AntiAliasGridFit
 $color=if($i%2 -eq 0){[Drawing.Color]::FromArgb(255,201,83)}else{[Drawing.Color]::FromArgb(137,238,218)}
 $pen=[Drawing.Pen]::new($color,6);$gfx.DrawRectangle($pen,10,10,492,236)
 $brush=[Drawing.SolidBrush]::new($color);$font=[Drawing.Font]::new('Microsoft YaHei',68,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel);$format=[Drawing.StringFormat]::new();$format.Alignment=[Drawing.StringAlignment]::Center;$format.LineAlignment=[Drawing.StringAlignment]::Center
 $gfx.DrawString($names[$i],$font,$brush,[Drawing.RectangleF]::new(18,30,476,135),$format)
 $small=[Drawing.Font]::new('Arial',20,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel);$gfx.DrawString('KOWLOON  /  SINCE 1982',$small,$brush,[Drawing.RectangleF]::new(18,175,476,40),$format)
 $bmp.Save((Join-Path $artRoot ('Sign'+$i+'.png')),[Drawing.Imaging.ImageFormat]::Png)
 $format.Dispose();$small.Dispose();$font.Dispose();$brush.Dispose();$pen.Dispose();$gfx.Dispose();$bmp.Dispose()
}
Write-Output 'KTW ART SOURCE PASS: seven tiled textures and eight original Chinese signs'
