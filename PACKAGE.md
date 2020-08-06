# Make APPX file from APPXRECIPE
`makeappx.exe pack /d ".\x64\Release\DistroLauncher-Appx" /l /p ".\Manjaro-Architect-20.0.3.appx"`

Set-Location .\;
# Sign the newly generated APPX file with the signing key
`signtool.exe sign /fd SHA256 /a /f .\DistroLauncher-Appx\DistroLauncher-Appx_TemporaryKey.pfx /p password .\x64\Release\DistroLauncher-Appx\Manjaro-Architect-20.0.3.appx`