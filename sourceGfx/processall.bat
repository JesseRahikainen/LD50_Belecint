del /Q .\processed\*.png

for %%f in (.\original\*.png) do (
    ..\tools\SDFImageGenerator %%f .\processed\%%~nf.png -xa 8 -ya 8
)

..\tools\SpriteSheetGenerator ..\bin\data\Images\ui -d ./processed