# EZ-FLASH  Omega Definitive Edition Kernel

## Author's words

ZaindORp added some features to this version.
Why ZaindORp made this? Who knows. In fact Sterophonick's version seems better and more powerful.
ZaindORp isn't really a good programmer and she just found a old EZ-Omega kernel so ZaindORp tried to renew it and port it to the EZ-Omega Defintive Edition.
But after everything is done she found erophonick's version and found the project have already be made before but decided to keep on developing ZaindORp's own version. ZaindORp has no EZ-Omega so she can't test the kernel on that and won't make a version for that.
By the way ZaindORp always call herself in the third person.

## What's added

- Save backup
- Skip saving when holding L
- Pogoshell plugins support
- Use custom theme place in sd card
- An easter egg(seems broken due to the changes of the devkitpro)
- An transgender flag in help page(Create a file name I_SUPPORT_TRANSGENDER in the folder FLAGS to disable it)

## How to use the pogoshell plugins

1. Rename the plugins as [extname the plugin can open].[plugin's own extname]
2. Move it to /PLUGINS/
3. Enjoy the plugins! Now you can open the file with plugins.

### How to make and use the themes

1. Download the _UniversalSkinner.
2. Create a blank pure text file at the same folder of the _UniversalSkinner.exe as definitions.xml and write these codes in.
```
<?xml version="1.0" encoding="utf-8" ?>
<definitions>
<file chksum="0x3c21e1e6" comment="EZOmega DE_ThemeFile">
    <bitmap offset="0x0" width="70" height="70" color="16">gImage_Chinese_manual</bitmap>
    <bitmap offset="0x2648" width="70" height="70" color="16">gImage_English_manual</bitmap>
    <bitmap offset="0x4C90" width="240" height="160" color="16">gImage_splash</bitmap>
    <bitmap offset="0x17890" width="240" height="160" color="16">gImage_SD</bitmap>
    <bitmap offset="0x2A490" width="240" height="160" color="16">gImage_NOR</bitmap>
    <bitmap offset="0x3D090" width="240" height="160" color="16">gImage_SET</bitmap>
    <bitmap offset="0x4FC90" width="240" height="160" color="16">gImage_SET2</bitmap>
    <bitmap offset="0x62890" width="240" height="160" color="16">gImage_HELP</bitmap>
    <bitmap offset="0x75490" width="240" height="160" color="16">gImage_RECENTLY</bitmap>
    <bitmap offset="0x88090" width="120" height="80" color="16">gImage_NOTFOUND</bitmap>
    <bitmap offset="0x8CB90" width="16" height="14" color="16">gImage_icon_gba</bitmap>
    <bitmap offset="0x8CD50" width="16" height="14" color="16">gImage_icon_folder</bitmap>
    <bitmap offset="0x8CF10" width="16" height="14" color="16">gImage_icon_other</bitmap>
    <bitmap offset="0x8D0D0" width="16" height="14" color="16">gImage_icon_FC</bitmap>
    <bitmap offset="0x8D290" width="16" height="14" color="16">gImage_icon_GB</bitmap>
    <bitmap offset="0x8D450" width="16" height="14" color="16">gImage_icon_nor</bitmap>
    <bitmap offset="0x8D610" width="128" height="110" color="16">gImage_MENU</bitmap>
    <bitmap offset="0x94410" width="1" height="1" color="16">gl_color_text</bitmap>
    <bitmap offset="0x94412" width="1" height="1" color="16">gl_color_selectBG_sd</bitmap>
    <bitmap offset="0x94414" width="1" height="1" color="16">gl_color_selectBG_nor</bitmap>
    <bitmap offset="0x94416" width="1" height="1" color="16">gl_color_cheat_black</bitmap>
    <bitmap offset="0x94418" width="1" height="1" color="16">gl_color_saverr</bitmap>
    <bitmap offset="0x9441A" width="1" height="1" color="16">gl_color_unknown</bitmap>
    <bitmap offset="0x9441C" width="1" height="1" color="16">gl_color_MENU_btn</bitmap>
    <bitmap offset="0x9441E" width="1" height="1" color="16">gl_color_selected</bitmap>
    <bitmap offset="0x94420" width="1" height="1" color="16">gl_color_cheat_count</bitmap>
    <bitmap offset="0x94422" width="1" height="1" color="16">gl_color_NORFULL</bitmap>
    <bitmap offset="0x94424" width="1" height="1" color="16">gl_color_btn_clean</bitmap>
</file>
</definitions>
```
3. Run the _UniversalSkinner.exe and open image.bin which provided with code and replace the picturs with one you like.
```
Images:
	gImage_Chinese_manual: The QR code which direct to the user manual when you use Chinese
	gImage_English_manual: The QR code which direct to the user manual when you use English
	gImage_splash: The splash screen when booting(but the file system always load after the splash screen and the theme loaded after that so this won't change with the theme)
	gImage_SD: The page and the background when in the page SD
	gImage_NOR: The page and the background when in the page NOR
	gImage_SET: The page and the background when in the page SET
	gImage_SET2: The page and the background when in the page SET2
	gImage_HELP: The page and the background when in the page HELP
	gImage_RECENTLY: The page and the background when in the page RECENTLY(Two custom setting pages in this kernel also use this background)
	gImage_NOTFOUND: The image show when the gba file has no thumbnail
	gImage_icon_gba: The icon for the file type gba
	gImage_icon_folder: The icon for the folder
	gImage_icon_other: The icon for the file types which have no icons
	gImage_icon_FC: The icon for the file type nes
	gImage_icon_GB: The icon for the file type gb and gbc
	gImage_icon_nor: The icon for the file in NorFlash
	gImage_MENU: The background of the popup menu windows
Colors: (All the colors are shown as a one-pixel picture and the color of the pixel will be used)
	gl_color_text: The color most text used
	gl_color_selectBG_sd: The color show as the background of the select bar when in SD page
	gl_color_selectBG_nor: The color show as the background of the select bar when in NOR page
	gl_color_cheat_black: The background color when loading the game
	gl_color_saverr: The color show when the save file went wrong(unused in the program)
	gl_color_unknown: Unknown (unused in the program)
	gl_color_MENU_btn: The background of the selection when the course doesn't move on
	gl_color_selected: The color where the selected text used
	gl_color_cheat_count: The color the cheat selection shows when the cheat is on
	gl_color_NORFULL: The color shows when the NorFlash is full
	gl_color_btn_clean: The background of the selection when the course move on
```
3. Click on "save firmware" in the menu and the tool will gen image.bin_edit. You can also share this file so everyone can easily use the theme.
4. Rename it as [theme_name].ezodetheme and put it in your SD card.
5. Enable the custom theme selection and select the theme file on your GBA, press A to apply it.
6. Enjoy the theme.

## How to build 

1. We use devkitARM_r53, you can use the current version or newer.
2. Set the following environment variables in system, based on your installation path
		PATH,DEVKITARM,DEVKITPRO,LIBGBA   
3. Use "make" to build the kernel.
